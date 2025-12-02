#include "bsp.h"
#include "delay.h"

XIntc sys_intc;
XTmrCtr sampling_tmr; // axi_timer_0
XTmrCtr pwm_tmr; // axi_timer_1

volatile u32 circular_buffer[BUFFER_SIZE] = {0};
volatile u32 read_head = 0;
volatile u32 write_head = 0;

volatile static u32 count = 0;

volatile static u32 s_min = 0xFFFFFFFF;
volatile static u32 s_max = 0;

#define RAW_MIN   4235899812u
#define RAW_MAX   4286508678u
#define AVG_VAL   4250000000u

// 256-point Sine Wave Table (Scaled for PWM Period 2267)
static const u16 sine_table[256] = {
    1133, 1161, 1189, 1216, 1244, 1271, 1299, 1326, 1354, 1381, 1408, 1435, 1462, 1488, 1515, 1541,
    1567, 1593, 1618, 1643, 1668, 1693, 1717, 1741, 1765, 1788, 1811, 1833, 1855, 1877, 1898, 1919,
    1939, 1959, 1978, 1997, 2015, 2033, 2050, 2066, 2082, 2097, 2111, 2125, 2138, 2150, 2162, 2173,
    2183, 2192, 2201, 2209, 2216, 2223, 2229, 2234, 2238, 2242, 2245, 2247, 2248, 2249, 2248, 2247,
    2245, 2242, 2238, 2234, 2229, 2223, 2216, 2209, 2201, 2192, 2183, 2173, 2162, 2150, 2138, 2125,
    2111, 2097, 2082, 2066, 2050, 2033, 2015, 1997, 1978, 1959, 1939, 1919, 1898, 1877, 1855, 1833,
    1811, 1788, 1765, 1741, 1717, 1693, 1668, 1643, 1618, 1593, 1567, 1541, 1515, 1488, 1462, 1435,
    1408, 1381, 1354, 1326, 1299, 1271, 1244, 1216, 1189, 1161, 1133, 1106, 1078, 1051, 1023, 996,
    968, 941, 913, 886, 859, 832, 805, 779, 752, 726, 700, 674, 649, 624, 599, 574,
    550, 526, 502, 479, 456, 434, 412, 390, 369, 348, 328, 308, 289, 270, 252, 234,
    217, 201, 185, 170, 156, 142, 129, 117, 105, 94, 84, 75, 66, 58, 51, 44,
    38, 33, 29, 25, 22, 20, 19, 18, 19, 20, 22, 25, 29, 33, 38, 44,
    51, 58, 66, 75, 84, 94, 105, 117, 129, 142, 156, 170, 185, 201, 217, 234,
    252, 270, 289, 308, 328, 348, 369, 390, 412, 434, 456, 479, 502, 526, 550, 574,
    599, 624, 649, 674, 700, 726, 752, 779, 805, 832, 859, 886, 913, 941, 968, 996,
    1023, 1051, 1078, 1106
};

void BSP_init() {
	// interrupt controller
	XIntc_Initialize(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	XIntc_Start(&sys_intc, XIN_REAL_MODE);
	init_pwm_timer();
	init_sampling_timer();
}

//// Note: this interrupt triggers at 44.1 kHz (every 22.67 us)
//void sampling_ISR() {
//	Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 4, 0);
//	u32 curr_sample = Xil_In32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 8);
//
//	/*
//	 * Max value of curr_sample is [fill in max val here]
//	 * Max value of the PWM is the period of the pulse, RESET_VALUE (2267)
//	 * To scale the mic sample to fit within the PWM period, divide it by 2^n [fill out n]
//	 */
//
////	u32 pwm_sample;
////	if (curr_sample <= RAW_MIN) {
////	    pwm_sample = 0;
////	} else if (curr_sample >= RAW_MAX) {
////	    pwm_sample = RESET_VALUE;
////	} else {
////	    // Linear mapping: [RAW_MIN, RAW_MAX] -> [0, RESET_VALUE]
////	    u32 span = RAW_MAX - RAW_MIN;                   // ~40,000,000
////	    u32 x    = curr_sample - RAW_MIN;               // 0..span
////	    pwm_sample = (u32)(((u64)x * RESET_VALUE) / span);
////	}
//
////	int32_t sample26 = ((int32_t)curr_sample << 6) >> 6;
////	u32 pwm_sample = sample26_to_pwm(sample26);
//
//	u32 raw26 = curr_sample & 0x03FFFFFF;
//	const int32_t DC_OFFSET = (1 << 25);
//	int32_t centered = (int32_t)raw26 - DC_OFFSET;
//	u32 pwm_sample = sample26_to_pwm(centered);
//
//	count++;
//	if (count >= 44100) {
//		xil_printf("curr_sample: %lu\r\n", curr_sample);
//		xil_printf("sample26: %ld\r\n", centered);
//		xil_printf("pwm_sample: %lu\r\n", pwm_sample);
//		count = 0;
//	}
//
//	if (pwm_sample > RESET_VALUE) {pwm_sample = RESET_VALUE;} // clip audio to a max ceiling
//
//	XTmrCtr_SetResetValue(&pwm_tmr, 1, pwm_sample);
//
//	// note: have to write to the baseaddr of the stream grabber to start it for the next sample
//	Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR, 0);
//
//	// Acknowledge the interrupt by clearing the interrupt bit in the timer control status register - referenced lab2b code
//	Xuint32 ControlStatusReg = XTmrCtr_ReadReg(sampling_tmr.BaseAddress, 0, XTC_TCSR_OFFSET);
//	XTmrCtr_WriteReg(sampling_tmr.BaseAddress, 0, XTC_TCSR_OFFSET,
//				ControlStatusReg |XTC_CSR_INT_OCCURED_MASK);
//}

void sampling_ISR() {
    // 1. Hardware Ping-Pong
    Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 4, 0);
    u32 raw_data = Xil_In32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 8);

    // 2. CAST TO SIGNED (Crucial Step)
    // This turns "4.2 Billion" into "-37 Million"
    int32_t curr_sample = (int32_t)raw_data;

    // 3. DC Bias Tracking (Use signed math)
    static int32_t dc_bias = 0;
    static int first_run = 1;

    if (first_run) {
        dc_bias = curr_sample;
        first_run = 0;
    }

    // Exponential Moving Average
    dc_bias += (curr_sample - dc_bias) >> 10;

    // 4. Remove Bias
    // e.g., -37,000,000 - (-37,000,000) = 0 (Centered!)
    int32_t audio_signal = curr_sample - dc_bias;

    // 5. Scale (Arithmetic Shift)
    // Now that we preserve the sign, we can shift safely.
    // Try >> 10 or >> 11 based on volume needs.
    int32_t scaled_signal = audio_signal >> 15;

    // 6. Re-Center for PWM (Unsigned Output)
    // We add the mid-point (1133) to turn the signed AC wave into a positive DC wave.
    int32_t pwm_sample = (RESET_VALUE / 2) + scaled_signal;

    // 7. Clip
    if (pwm_sample < 0) pwm_sample = 0;
    if (pwm_sample > RESET_VALUE) pwm_sample = RESET_VALUE;

    // print tests
    count++;
	if (count >= 44100) {
		xil_printf("raw_sample: %lu\r\n", raw_data);
		xil_printf("curr_sample: %ld\r\n", curr_sample);
		xil_printf("pwm_sample: %lu\r\n", pwm_sample);
		count = 0;
	}

    // 8. Output
    XTmrCtr_SetResetValue(&pwm_tmr, 1, pwm_sample);

    // 9. Restart Hardware
    Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR, 0);

    // 10. Ack Interrupt...
    Xuint32 csr = XTmrCtr_ReadReg(sampling_tmr.BaseAddress, 0, XTC_TCSR_OFFSET);
    XTmrCtr_WriteReg(sampling_tmr.BaseAddress, 0, XTC_TCSR_OFFSET, csr | XTC_CSR_INT_OCCURED_MASK);
}

// Note: This is a test ISR that gives us the min/max value of curr_sample over a 5 second recording interval
//void sampling_ISR() {
//    // Kick the stream grabber as before (if required by your IP)
//    Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 4, 0);
//
//    u32 curr_sample = Xil_In32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 8);
//
//    // ---- Debug: track min/max over one second ----
//    static u32 s_count = 0;
//
//    if (curr_sample < s_min) s_min = curr_sample;
//    if (curr_sample > s_max) s_max = curr_sample;
//
//    s_count++;
//    if (s_count >= 220500) {  // ~1 second at 44.1 kHz
//        xil_printf("RAW mic: min=%lu, max=%lu\r\n", s_min, s_max);
//        s_min = 0xFFFFFFFF;
//        s_max = 0;
//        s_count = 0;
//    }
//
//    // (Optionally still re-init stream grabber)
//    Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR, 0);
//
//    // Clear timer interrupt
//    Xuint32 ControlStatusReg =
//        XTmrCtr_ReadReg(sampling_tmr.BaseAddress, 0, XTC_TCSR_OFFSET);
//    XTmrCtr_WriteReg(sampling_tmr.BaseAddress, 0, XTC_TCSR_OFFSET,
//                     ControlStatusReg | XTC_CSR_INT_OCCURED_MASK);
//}


// Note: this interrupt triggers at 44.1 kHz -- test if a sin wave generates a certain frequency
//void sampling_ISR() {
//    // 1. Maintain the "Ping-Pong" with the input hardware (just to keep it alive)
//    // Even though we ignore the mic data, we keep this so the grabber doesn't hang.
//    Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 4, 0);
//    u32 dummy_sample = Xil_In32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR + 8);
//
//    // 2. SINE WAVE GENERATION
//    static u32 sine_index = 0;
//
//    // We step through the table by 3 indices per interrupt.
//    // Freq = (SampleRate * StepSize) / TableSize
//    // Freq = (44100 * 3) / 256 = ~516 Hz (A nice C5 note)
//    sine_index = (sine_index + 5) % 256;
//
//    u32 pwm_sample = sine_table[sine_index];
//
//    // 3. Write to PWM Output
//    XTmrCtr_SetResetValue(&pwm_tmr, 1, pwm_sample);
//
//    // 4. Restart Input Hardware (Ping-Pong)
//    Xil_Out32(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR, 0);
//
//    // 5. Acknowledge Interrupt
//    u32 csr = XTmrCtr_GetControlStatusReg(sampling_tmr.BaseAddress, 0);
//    XTmrCtr_SetControlStatusReg(sampling_tmr.BaseAddress, 0, csr | XTC_CSR_INT_OCCURED_MASK);
//}

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
	// Side Note: we can decrease 2267 to a smaller number to increase the amount of 'pwm cycles' in one sampling cycle; this leads to a smoother signal because of analog filtering
	// think of channel 0 of the pwm_tmr as modifying the "Auto Reload Register (ARR)" of STM32 timers
	XTmrCtr_SetResetValue(&pwm_tmr, 0, 6000);

	// Set the Duty Cycle (High Time) in the second register (TLR1)
	// Start with 50% duty cycle (silence)
	// think of channel 1 of the pwm_tmr as modifying the "Capture Compare Register (CCR)" of STM32 timers
	XTmrCtr_SetResetValue(&pwm_tmr, 1, 6000 / 2);

	// This function sets the specific bits in the Control Status Register to turn on PWM
	XTmrCtr_PwmEnable(&pwm_tmr);

	// Start the PWM generation
	XTmrCtr_Start(&pwm_tmr, 0);

	xil_printf("PWM Timer successfully initialized!\r\n");

	return XST_SUCCESS;
}
