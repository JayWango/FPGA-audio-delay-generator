#include "xil_cache.h"		                /* Cache Drivers */
#include "bsp.h"
#include "stream_grabber.h"

unsigned seqf, seql, seq_old = 0;

//void read_fsl_values(int n) {
//   stream_grabber_start();
//   stream_grabber_wait_enough_samples(n);
//   seql = stream_grabber_read_seq_counter();
//   seqf = stream_grabber_read_seq_counter_latched();
//
//   for(int i = 0; i < n; i++) {
//      int_buffer[i] = stream_grabber_read_sample(i);
//      xil_printf("int_buffer[i]: %d\n", int_buffer[i]);
//   }
//}

int main() {
	Xil_ICacheInvalidate();
	Xil_ICacheEnable();
	Xil_DCacheInvalidate();
	Xil_DCacheEnable();

	BSP_init();

	while (1) {
		// Print roughly once per second (assuming 48kHz interrupt rate)
//		if (sys_tick_counter >= 48000) {
//
//			xil_printf("Buffer: [%ld, %ld, %ld, %ld, %ld]\r\n",
//						tiny_buffer[0], tiny_buffer[1], tiny_buffer[2],
//						tiny_buffer[3], tiny_buffer[4]);
//
//			xil_printf("Current Average Used: %ld\r\n\r\n", curr_sample);
//
//			sys_tick_counter = 0; // Reset counter for next second
//		}
	}

//	while(1); {
//		// read_fsl_values(SAMPLES);
//
//		// Uncomment if you want logging
////		if (sys_tick_counter >= 48000) {  // Print once per second at 48kHz
////			xil_printf("Raw sample:        %ld\r\n", curr_sample);
////			xil_printf("DC bias(static):            %ld\r\n", dc_bias_static);
////			xil_printf("DC bias(drift):            %ld\r\n", dc_bias_drift);
////			xil_printf("Input level:        %ld (threshold: %d)\r\n", input_level, AGC_THRESHOLD);
////			xil_printf("After input limit:  %ld (threshold: %d)\r\n", limited_signal, INPUT_LIMIT_THRESHOLD);
////			xil_printf("After output limit: %ld (threshold: %d)\r\n", output_signal, OUTPUT_LIMIT_THRESHOLD);
////			xil_printf("PWM sample:         %ld\r\n", pwm_sample);
////			xil_printf("Delay: enabled=%d, samples=%lu\r\n", delay_enabled, delay_samples);
////			xil_printf("===============================\r\n\r\n");
////			sys_tick_counter = 0;
////		}
//	}

	return 0;
}
