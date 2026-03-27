/**
 * @name  Project: umg/hbridge
 * @note  PWM signal control via CMD console using Nucleo board and H-bridge.
 * @brief Project configuration. These values are used by Forge when loading project.
 *        They should not be removed, but can be edited. Changing requires running Forge again.
 * @date  2025-04-21
 */
#include <xdef.h>

#ifndef PRO_x
#define PRO_x

#define PRO_BOARD_NONE
#define PRO_CHIP_STM32G081
#define PRO_VERSION "0.2.1"
#define PRO_FLASH_kB 120
#define PRO_RAM_kB 36
#define PRO_OPT_LEVEL "Og"

#endif

/**
 * @brief Put here config parameters `#define` that should be overridden.
 * @note  Many libraries include this file, so it must exist even if empty.
 */
#define LOG_LEVEL LOG_LEVEL_INF
#define SYS_CLOCK_FREQ 16000000