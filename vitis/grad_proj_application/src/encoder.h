#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include "xgpio.h"
#include "xintc.h"
#include "xparameters.h"

#define ENCODER_GPIO_DEVICE_ID  XPAR_GPIO_0_DEVICE_ID
#define ENCODER_VEC_ID XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR
#define ENCODER_GPIO_CH 1
#define ENCODER_PUSH_DEBOUNCE_TICKS  400u

extern volatile int s_saw_cw;
extern volatile int s_saw_ccw;

void quad_step(uint8_t ab);

#endif // ENCODER_H
