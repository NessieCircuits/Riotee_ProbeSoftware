#ifndef RIOTEEPROBE_CONFIG_H_
#define RIOTEEPROBE_CONFIG_H_

#ifdef BOARD_RIOTEE_BOARD

#define PROBE_USB_VID 0x1209
#define PROBE_USB_PID 0xC8A0

#define PROBE_PIN_LED 2

#define PROBE_PIN_SWDCLK 16
#define PROBE_PIN_SWDIO 18

#define PROBE_PIN_SBWCLK 17
#define PROBE_PIN_SBWIO 19

/* Controls direction of level translators */
#define PROBE_PIN_PROG_DIR 15

/* Controls target fixed power supply */
#define PROBE_PIN_TARGET_POWER 0

/* Controls power for programming level translators */
#define PROBE_PIN_TRANS_PROG_EN 20

/* Controls power for UART TX level translator */
#define PROBE_PIN_TRANS_UARTTX_EN 5

/* Controls power for UART RX level translator */
#define PROBE_PIN_TRANS_UARTRX_EN 6

/* Uart TX to target */
#define PROBE_UART_TX 8
/* Uart RX from target */
#define PROBE_UART_RX 9

#define PROBE_UART_INTERFACE uart1

#endif

#ifdef BOARD_RIOTEE_PROBE

#define PROBE_USB_VID 0x1209
#define PROBE_USB_PID 0xC8A1

#define PROBE_PIN_LED 2

#define PROBE_PIN_SWDCLK 18
#define PROBE_PIN_SWDIO 16

#define PROBE_PIN_SBWCLK 19
#define PROBE_PIN_SBWIO 17

/* Controls direction of level translators */
#define PROBE_PIN_PROG_DIR 15

/* Controls target fixed power supply */
#define PROBE_PIN_TARGET_POWER 3

/* Controls power for programming level translators */
#define PROBE_PIN_TRANS_PROG_EN 20

/* Controls power for UART TX level translator */
#define PROBE_PIN_TRANS_UARTTX_EN 5

/* Controls power for UART RX level translator */
#define PROBE_PIN_TRANS_UARTRX_EN 6

/* Uart TX to target */
#define PROBE_UART_TX 8
/* Uart RX from target */
#define PROBE_UART_RX 9

#define PROBE_UART_INTERFACE uart1

#endif

#define PROBE_UART_BAUDRATE 115200

#endif /* RIOTEEPROBE_CONFIG_H_ */
