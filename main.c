#include "rtc.h"
#include "sys.h"
#include "vrts.h"
#include "dbg.h"
#include "bash.h"
#include "pwm.h"
#include "main.h"

//------------------------------------------------------------------------------------------------- dbg

UART_t dbg_uart = {
  .reg = USART1,
  .tx_pin = UART1_TX_PC4,
  .rx_pin = UART1_RX_PC5,
  .dma_channel = DMA_Channel_4,
  .int_prioryty = INT_Prioryty_Low,
  .UART_115200
};

//------------------------------------------------------------------------------------------------- pwm

// Struktura parametrów PWM
typedef struct {
  float frequency; // Częstotliwość PWM [Hz]
  float duty; // Wypełnienie PWM [0-1]
  float deadtime; // Czas martwy między sygnałami [s]
} PWM_Control_t;

PWM_Control_t settings = {
  .frequency = 1000,
  .duty = 0,
  .deadtime = 0.0000005
};
PWM_Control_t output;

PWM_t pwm_1 = {
  .reg = TIM1,
  .channel[TIM_CH1] = TIM1_CH1_PA8, .channel[TIM_CH1N] = TIM1_CH1N_PA7,
  .channel[TIM_CH2] = TIM1_CH2_PA9, .channel[TIM_CH2N] = TIM1_CH2N_PB0,
  #if(DRIVER_LIN_NEGATION)
    .invert[TIM_CH1] = true, .invert[TIM_CH1N] = false,
    .invert[TIM_CH2] = false, .invert[TIM_CH2N] = true,
  #else
    .invert[TIM_CH1] = false, .invert[TIM_CH1N] = false,
    .invert[TIM_CH2] = true, .invert[TIM_CH2N] = true,
  #endif
  .center_aligned = true,
  .auto_reload = 1600,
  .deadtime = 10
};

PWM_t pwm_15 = {
  .reg = TIM15,
  .channel[TIM_CH1] = TIM15_CH1_PB14, .channel[TIM_CH1N] = TIM15_CH1N_PB13,
  .invert[TIM_CH1] = true, .invert[TIM_CH1N] = true,
  .center_aligned = false,
  .prescaler = 1000,
  .auto_reload = 1600,
  .deadtime = 10
};

void PWM_Print(PWM_Control_t *ctrl, char *head)
{
  LOG_Info("PWM %s: %fkHz %F%% %fµs", head,
    ctrl->frequency / 1000,
    ctrl->duty * 100,
    ctrl->deadtime * 1000000
  );
}

/**
 * @brief Konfiguruje PWM według parametrów.
 * Faktycznie ustawione wartości zapisuje w strukturze 'output'.
 * @param ctrl Wskaźnik na ustawienia PWM: częstotliwości, wypełnienie, czas martwy.
 * @param pwm Wskaźnik na kontroler PWM.
 */
void PWM_Set(PWM_t *pwm, PWM_Control_t *ctrl)
{
  PWM_Off(pwm);
  uint32_t prescaler = 1;
  uint32_t auto_reload;
  do {
    PWM_SetPrescaler(pwm, prescaler++);
    auto_reload = (float)SystemCoreClock / pwm->prescaler / ctrl->frequency / (pwm->center_aligned + 1);
  } while(auto_reload > 0xFFFF);
  uint32_t deadtime = ctrl->deadtime * SystemCoreClock;
  PWM_SetAutoreload(pwm, auto_reload);
  PWM_SetDeadtime(pwm, deadtime);
  uint32_t value = ctrl->duty * pwm->auto_reload;
  PWM_SetValue(pwm, TIM_CH1, value);
  PWM_SetValue(pwm, TIM_CH2, value);
  output.frequency = (float)SystemCoreClock / pwm->prescaler / pwm->auto_reload / (pwm->center_aligned + 1);
  output.duty = (float)pwm->value[TIM_CH1] / pwm->auto_reload;
  output.deadtime = (float)pwm->deadtime / SystemCoreClock;
  PWM_On(pwm);
}

static void PWM_Bash(char **argv, uint16_t argc)
{
  char *rev;
  for(uint8_t i = 1; i < argc; i++) {
    if(str2unitfloat_fault(argv[i])) continue;
    rev = reverse_string(argv[i]);
    if(rev[1] == 'h' && rev[0] == 'z') { settings.frequency = str2unitfloat(argv[i]); }
    else if(rev[0] == '%') { settings.duty = str2unitfloat(argv[i]); }
    else if(rev[0] == 's') { settings.deadtime = str2unitfloat(argv[i]); }
  }
  PWM_Print(&settings, "set");
  PWM_Set(&pwm_1, &settings);
  // PWM_Set(&pwm_15, &settings);
  PWM_Print(&output, "out");
}

//------------------------------------------------------------------------------------------------- app

GPIO_t led = { // Nucleo LED
  .port = GPIOA,
  .pin = 5,
  .mode = GPIO_Mode_Output
};

void loop(void)
{
  PWM_Init(&pwm_1);
  // PWM_Init(&pwm_15);
  PWM_Set(&pwm_1, &settings);
  // PWM_Set(&pwm_15, &settings);
  PWM_Print(&output, "ini");
  BASH_AddCallback(&PWM_Bash, "pwm");
  while(1) {
    GPIO_Tgl(&led); // Zmiana stanu diody
    delay(1000); // Odczekaj 1s
  }
}

//------------------------------------------------------------------------------------------------- main

stack(stack_dbg, 256); // Stos pamięci dla wątku debug'era (logs + bash)
stack(stack_loop, 256); // Stos pamięci dla funkcji loop

int main(void)
{
  system_clock_init(); // Konfiguracja systemowego sygnału zegarowego
  systick_init(10); // Uruchomienie zegara systemowego z dokładnością do 10ms
  RTC_Init(); // Włączenie zegara czasu rzeczywistego (RTC)
  DBG_Init(&dbg_uart); // Inicjalizacja debuger'a (logs + bash)
  DBG_Enter();
  LOG_Init("UMG HBridge", PRO_VERSION);
  GPIO_Init(&led); // Inicjalizacja diody LED
  thread(DBG_Loop, stack_dbg); // Dodanie wątku debug'era (logs + bash)
  thread(loop, stack_loop); // Dodanie funkcji loop jako wątek
  vrts_init(); // Włączenie systemy przełączania wątków VRTS
  while(1); // W to miejsce program nigdy nie powinien dojść
}
