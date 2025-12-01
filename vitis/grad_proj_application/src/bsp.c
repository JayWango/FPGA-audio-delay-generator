#include "bsp.h"
#include "delay.h"

XIntc sys_intc;
XTmrCtr sampling_tmr; // axi_timer_0
XTmrCtr pwm_tmr; // axi_timer_1

volatile u32 circular_buffer[BUFFER_SIZE] = {0};
volatile u32 read_head = 0;
volatile u32 write_head = 0;

void BSP_init() {
	// interrupt controller
	XIntc_Initialize(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	XIntc_Start(&sys_intc, XIN_REAL_MODE);
	init_pwm_timer();
	init_sampling_timer();
}

// Note: this interrupt triggers at 44.1 kHz (every 22.67 us)
void sampling_ISR() {
	Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 4, 0);
	u32 curr_sample = Xil_In32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 8);

	static int count = 0;
	static int toggle = 0;

	count++;

	if (count > 44100) {
		toggle = !toggle;
		count = 0;
		xil_printf("%lu\r\n", curr_sample);
	}

	if (toggle) {
		XTmrCtr_SetResetValue(&pwm_tmr, 1, 1);
	}
	else {
		XTmrCtr_SetResetValue(&pwm_tmr, 1, RESET_VALUE);
	}

	/*
	 * Max value of curr_sample is 65,535 (16-bit PDM mic output)
	 * Max value of the PWM is the period of the pulse, RESET_VALUE (2267)
	 * To scale the mic sample to fit within the PWM period, divide it by 2^5
	 */
	//u32 pwm_sample = curr_sample >> 5;
	//if (pwm_sample > RESET_VALUE) {pwm_sample = RESET_VALUE;} // clip audio to a max ceiling

	//XTmrCtr_SetResetValue(&pwm_tmr, 1, pwm_sample);

	Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR, 0);

	// Acknowledge the interrupt by clearing the interrupt bit in the timer control status register - referenced lab2b code
	Xuint32 ControlStatusReg = XTmrCtr_ReadReg(sampling_tmr.BaseAddress, 0, XTC_TCSR_OFFSET);
	XTmrCtr_WriteReg(sampling_tmr.BaseAddress, 0, XTC_TCSR_OFFSET,
				ControlStatusReg |XTC_CSR_INT_OCCURED_MASK);
}

int init_sampling_timer() {
	XStatus Status;
	Status = XST_SUCCESS;
	Status = XIntc_Connect(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
			(XInterruptHandler) sampling_ISR, &sampling_tmr);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to connect the application handlers to the interrupt controller...\r\n");
		return XST_FAILURE;
	}
	xil_printf("Connected to Interrupt Controller!\r\n");

	/*
	 * Enable the interrupt for the timer counter
	 */
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
	/*
	 * Initialize the timer counter so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h
	 */
	Status = XTmrCtr_Initialize(&sampling_tmr, XPAR_AXI_TIMER_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Timer initialization failed...\r\n");
		return XST_FAILURE;
	}
	xil_printf("Initialized Timer!\r\n");
	/*
	 * Enable the interrupt of the timer counter so interrupts will occur
	 * and use auto reload mode such that the timer counter will reload
	 * itself automatically and continue repeatedly, without this option
	 * it would expire once only
	 */
	XTmrCtr_SetOptions(&sampling_tmr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	/*
	 * Set a reset value for the timer counter such that it will expire
	 * eariler than letting it roll over from 0, the reset value is loaded
	 * into the timer counter when it is started
	 */
	// clk cycles / 100 Mhz = period
	XTmrCtr_SetResetValue(&sampling_tmr, 0, 0xFFFFFFFF - RESET_VALUE);// 2267 clk cycles @ 100MHz = 22.67 us
	/*
	 * Start the timer counter such that it's incrementing by default,
	 * then wait for it to timeout a number of times
	 */
	XTmrCtr_Start(&sampling_tmr, 0);

	/*
	 * Register the intc device driver’s handler with the Standalone
	 * software platform’s interrupt table
	 */
	microblaze_register_handler(
			(XInterruptHandler) XIntc_DeviceInterruptHandler,
			(void*) XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);

	// refer to stream_grabber.c from lab3a; need to write some value to the base address to reinitialize the stream grabber
	Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR, 0);

	xil_printf("Interrupts enabled!\r\n");
	microblaze_enable_interrupts();

	return XST_SUCCESS;
}

int init_pwm_timer() {
	XStatus Status;

	// Initialize the PWM Timer instance
	Status = XTmrCtr_Initialize(&pwm_tmr, XPAR_AXI_TIMER_1_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Configure the timer for PWM Mode
	// Generate Output, and Down Counting (easier for duty cycle)
	XTmrCtr_SetOptions(&pwm_tmr, 0, XTC_EXT_COMPARE_OPTION | XTC_DOWN_COUNT_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetOptions(&pwm_tmr, 1, XTC_EXT_COMPARE_OPTION | XTC_DOWN_COUNT_OPTION | XTC_AUTO_RELOAD_OPTION);

	// Set the Period (Frequency) in the first register (TLR0)
	// We match the sampling frequency: 2267 ticks
	// think of channel 0 of the pwm_tmr as modifying the "Auto Reload Register (ARR)" of STM32 timers
	XTmrCtr_SetResetValue(&pwm_tmr, 0, RESET_VALUE);

	// Set the Duty Cycle (High Time) in the second register (TLR1)
	// Start with 50% duty cycle (silence)
	// think of channel 1 of the pwm_tmr as modifying the "Capture Compare Register (CCR)" of STM32 timers
	XTmrCtr_SetResetValue(&pwm_tmr, 1, RESET_VALUE / 2);

	// This function sets the specific bits in the Control Status Register to turn on PWM
	XTmrCtr_PwmEnable(&pwm_tmr);

	// Start the PWM generation
	XTmrCtr_Start(&pwm_tmr, 0);

	xil_printf("PWM Timer successfully initialized!\r\n");

	return XST_SUCCESS;
}
