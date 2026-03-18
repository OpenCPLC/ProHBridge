#ifndef CTRL_H_
#define CTRL_H_

#include <stdbool.h>
#include <pwm.h>
#include <cmd.h>
#include <xdef.h>

/**
 * @brief PWM control structure
 * @param frequency PWM frequency [Hz]
 * @param duty PWM duty ratio [0–1]
 * @param deadtime Dead-time between signals [s]
 * @param enable Output activation control
 */
typedef struct {
  float frequency;
  float duty;
  float deadtime;
  bool enable;
} CTRL_t;

void CTRL_Main(void);

#endif