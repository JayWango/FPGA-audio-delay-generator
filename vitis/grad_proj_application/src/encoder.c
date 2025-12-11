#include "encoder.h"
#include "xil_printf.h"
#include "xil_exception.h"
#include "xgpio.h"

/* Quadrature FSM */
typedef enum {
    Q_IDLE_11 = 0,
    Q_01, Q_00, Q_10,       // CW path
    Q_10b, Q_00b, Q_01b     // CCW path
} qstate_t;

static volatile qstate_t s_qstate = Q_IDLE_11;
volatile int s_saw_cw = 0;
volatile int s_saw_ccw = 0;

/* Step FSM with current AB (A:bit2, B:bit1) */
void quad_step(uint8_t ab) {
    switch (s_qstate) {
    	case Q_IDLE_11:
			if      (ab == 0b01) s_qstate = Q_01;   // CW start
			else if (ab == 0b10) s_qstate = Q_10b;  // CCW start
			break;

		case Q_01:
			if      (ab == 0b00) s_qstate = Q_00;
			else if (ab == 0b11) s_qstate = Q_IDLE_11;
			else if (ab == 0b10) s_qstate = Q_10;
			break;

		case Q_00:
			if      (ab == 0b10) s_qstate = Q_10;
			else if (ab == 0b01) s_qstate = Q_01;
			else if (ab == 0b11) s_qstate = Q_IDLE_11;
			break;

		case Q_10:
			if      (ab == 0b11) { s_qstate = Q_IDLE_11; s_saw_cw = 1; }
			else if (ab == 0b00) s_qstate = Q_00;
			break;

		case Q_10b:
			if      (ab == 0b00) s_qstate = Q_00b;
			else if (ab == 0b11) s_qstate = Q_IDLE_11;
			else if (ab == 0b01) s_qstate = Q_01b;
			break;

		case Q_00b:
			if      (ab == 0b01) s_qstate = Q_01b;
			else if (ab == 0b10) s_qstate = Q_10b;
			else if (ab == 0b11) s_qstate = Q_IDLE_11;
			break;

		case Q_01b:
			if      (ab == 0b11) { s_qstate = Q_IDLE_11; s_saw_ccw = 1; }
			else if (ab == 0b00) s_qstate = Q_00b;
			break;

		default:
			s_qstate = Q_IDLE_11;
			break;
    }
}
