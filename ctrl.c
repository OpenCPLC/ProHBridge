#include "ctrl.h"

static GPIO_t gpio_enable1 = {
  .port = GPIOA,
  .pin = 6,
  .mode = GPIO_Mode_Output,
  .speed = GPIO_Speed_VeryHigh
};

static GPIO_t gpio_enable2 = {
  .port = GPIOA,
  .pin = 5,
  .mode = GPIO_Mode_Output,
  .speed = GPIO_Speed_VeryHigh
};

static PWM_t pwm_1 = {
  .reg = TIM1,
  .channel[TIM_CH1] = TIM1_CH1_PA8, .channel[TIM_CH1N] = TIM1_CH1N_PA7,
  .channel[TIM_CH2] = TIM1_CH2_PA9, .channel[TIM_CH2N] = TIM1_CH2N_PB0,
  .invert[TIM_CH1] = false, .invert[TIM_CH1N] = false,
  .invert[TIM_CH2] = true, .invert[TIM_CH2N] = true,
  .center_aligned = true,
  .auto_reload = 1600,
  .deadtime = 10
};

// static PWM_t pwm_15 = {
//   .reg = TIM15,
//   .channel[TIM_CH1] = TIM15_CH1_PB14, .channel[TIM_CH1N] = TIM15_CH1N_PB13,
//   .invert[TIM_CH1] = true, .invert[TIM_CH1N] = true,
//   .center_aligned = false,
//   .prescaler = 1000,
//   .auto_reload = 1600,
//   .deadtime = 10
// };

static CTRL_t config = {
  .frequency = 1000,
  .duty = 0,
  .deadtime = 0.000004 // 4μs
};
static CTRL_t state;

/**
 * @brief Print PWM parameters for debug
 * @param ctrl Requested PWM settings (frequency, duty, deadtime, enable)
 * @param head Label prefix
 */
static void PWM_Print(const CTRL_t *ctrl, const char *head)
{
  float freq = ctrl->frequency;
  const char *fu = "Hz";
  if (freq >= 1000.0f) { freq /= 1000.0f; fu = "kHz"; }
  float deadtime = ctrl->deadtime;
  const char *du = "s";
  if (deadtime < 1e-3f) { deadtime *= 1e6f; du = "us"; }
  else if (deadtime < 1.0f) { deadtime *= 1e3f; du = "ms"; }
  LOG_INF("PWM " ANSI_GREY "(%s)" ANSI_END " %.3f%s %.2f%% %.0f%s %s",
    head, freq, fu, ctrl->duty * 100.0f, deadtime, du,
    ctrl->enable ? ANSI_GREEN "ON" ANSI_END : ANSI_RED "OFF" ANSI_END);
}

/**
 * @brief Configure PWM from requested parameters.
 * Applies frequency, duty, and dead-time; derives prescaler/autoreload within 16-bit range;
 * pdates runtime state; always enables PWM. The 'enable' field only drives gpio_enable.
 * @param pwm Target PWM peripheral handle
 * @param ctrl Requested PWM settings (frequency, duty, deadtime, enable)
 */
static void PWM_Set(PWM_t *pwm, const CTRL_t *ctrl)
{
  float freq = maxv(ctrl->frequency, 1.0f);
  float duty = clamp(ctrl->duty, 0.0f, 1.0f);
  // PWM_Off(pwm);
  GPIO_Rst(&gpio_enable1);
  GPIO_Rst(&gpio_enable2);
  uint32_t prescaler = 1, auto_reload;
  do {
    PWM_SetPrescaler(pwm, prescaler++);
    auto_reload = (uint32_t)lrintf((float)SystemCoreClock / (float)pwm->prescaler / freq / (pwm->center_aligned + 1));
  } while(auto_reload > 0xFFFF && prescaler <= 0xFFFF);
  auto_reload = clamp(auto_reload, 1, 0xFFFF);
  PWM_SetAutoreload(pwm, auto_reload);
  uint32_t deadtime = (uint32_t)llroundf(ctrl->deadtime * SystemCoreClock);
  PWM_SetDeadtime(pwm, deadtime);
  uint32_t value = (uint32_t)lrintf(duty * auto_reload);
  if(value > auto_reload) value = auto_reload;
  PWM_SetValue(pwm, TIM_CH1, value);
  PWM_SetValue(pwm, TIM_CH2, value);
  state.frequency = (float)SystemCoreClock / pwm->prescaler / pwm->auto_reload / (pwm->center_aligned + 1);
  state.duty = (float)pwm->value[TIM_CH1] / pwm->auto_reload;
  state.deadtime = (float)pwm->deadtime / SystemCoreClock;
  state.enable = ctrl->enable;
  // PWM_On(pwm);
  if(state.enable) {
    GPIO_Set(&gpio_enable1);
    GPIO_Set(&gpio_enable2);
  }
}

/**
 * @brief Bash callback for PWM configuration.
 * Format: `<freq>Hz <duty>% <dt>s {ON|OFF}`.
 * @param argv Argument vector
 * @param argc Argument count
 */
static void PWM_Bash(char **argv, uint16_t argc)
{
  for(uint16_t i = 0; i < argc; i++) {
    uint32_t arg_hash = hash_djb2(argv[i]);
    switch(arg_hash) {
      case HASH_Start: case HASH_On: config.enable = true; continue;
      case HASH_Stop: case HASH_Off: config.enable = false; continue;
    }
    if(!str_is_uf32(argv[i])) {
      LOG_ErrorParse(argv[i], "unit-float");
      continue;
    }
    uint16_t  len = strlen(argv[i]);
    const char *s = argv[i];
    if(len >= 2 && s[len - 2]=='H' && s[len - 1]=='z') config.frequency = str_to_uf32(s);
    else if(s[len - 1]=='%') config.duty = str_to_uf32(s);
    else if(s[len - 1]=='s') config.deadtime  = str_to_uf32(s);
  }
  PWM_Print(&config, "set");
  PWM_Set(&pwm_1, &config);
  PWM_Print(&state, "out");
}

static GPIO_t led = { // Nucleo LED
  .port = GPIOA,
  .pin = 5,
  .mode = GPIO_Mode_Output
};

/**
 * @brief Main control routine.
 * Initializes PWM, registers CLI handler, and toggles status LED in loop.
 * Intended as top-level runtime entry for PWM control module.
 */
void CTRL_Main(void)
{
  PWM_Init(&pwm_1);
  // PWM_Init(&pwm_15);
  PWM_Set(&pwm_1, &config);
  // PWM_Set(&pwm_15, &config);
  GPIO_Init(&led);
  GPIO_Init(&gpio_enable1);
  GPIO_Init(&gpio_enable2);
  PWM_Print(&state, "ini");
  CMD_AddCallback(&PWM_Bash, NULL);
  while(1) {
    GPIO_Tgl(&led);
    delay(1000);
  }
}
