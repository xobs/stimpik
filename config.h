#ifndef CONFIG_H_
#define CONFIG_H_

#define UART_TX_PIN 0 // Pico PIN 1
#define UART_RX_PIN 1 // Pico PIN 2
// GND					// Pico PIN 3
#define POWER1_PIN 2 // Pico PIN 4
#define POWER2_PIN 3 // Pico PIN 5
#define RESET_PIN  4 // Pico PIN 6
// unassigned        // Pico PIN 7
// GND					// Pico PIN 8
#define POWER3_PIN 6 // Pico PIN 9
#define SWDIO_PIN  7 // Pico PIN 10
#define SWCLK_PIN  8 // Pico PIN 11
#define BOOT0_PIN  9 // Pico PIN 12

#define UART_BAUD 9600
#define UART_ID   uart0
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART_STALLS_FOR_LED_OFF 10000

static const char DUMP_START_MAGIC[] = {0x10, 0xAD, 0xDA, 0x7A};

#endif /* CONFIG_H_ */
