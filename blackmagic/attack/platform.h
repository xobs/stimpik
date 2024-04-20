/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ATTACK_PLATFORM_H_
#define ATTACK_PLATFORM_H_

void platform_buffer_flush(void);
void platform_set_baud(uint32_t baud);

#define SET_RUN_STATE(state)
#define SET_IDLE_STATE(state)
#define SET_ERROR_STATE(state) gpio_set_level(CONFIG_LED_GPIO, !state)

#ifndef NO_LIBOPENCM3
#define NO_LIBOPENCM3
#endif

#ifndef PC_HOSTED
#define PC_HOSTED 0
#endif

#define DEBUG(x, ...)

#define SWDIO_MODE_FLOAT()                                                                \
	do {                                                                                  \
		/*gpio_ll_output_disable(GPIO_HAL_GET_HW(GPIO_PORT_0), CONFIG_TMS_SWDIO_GPIO); */ \
		gpio_set_direction(CONFIG_TMS_SWDIO_GPIO, GPIO_MODE_INPUT);                       \
		if (CONFIG_TMS_SWDIO_DIR_GPIO >= 0)                                               \
			gpio_set_level(CONFIG_TMS_SWDIO_DIR_GPIO, 1);                                 \
	} while (0)

#define SWDIO_MODE_DRIVE()                                                          \
	do {                                                                            \
		if (CONFIG_TMS_SWDIO_DIR_GPIO >= 0)                                         \
			gpio_set_level(CONFIG_TMS_SWDIO_DIR_GPIO, 0);                           \
		gpio_ll_output_enable(GPIO_HAL_GET_HW(GPIO_PORT_0), CONFIG_TMS_SWDIO_GPIO); \
	} while (0)

#define TMS_SET_MODE()                                                       \
	do {                                                                     \
		gpio_reset_pin(CONFIG_TDI_GPIO);                                     \
		gpio_reset_pin(CONFIG_TDO_GPIO);                                     \
		gpio_reset_pin(CONFIG_TMS_SWDIO_GPIO);                               \
		gpio_reset_pin(CONFIG_TCK_SWCLK_GPIO);                               \
		if (CONFIG_TMS_SWDIO_DIR_GPIO >= 0)                                  \
			gpio_reset_pin(CONFIG_TMS_SWDIO_DIR_GPIO);                       \
                                                                             \
		gpio_set_direction(CONFIG_TDI_GPIO, GPIO_MODE_OUTPUT);               \
		gpio_set_direction(CONFIG_TDO_GPIO, GPIO_MODE_INPUT);                \
		gpio_set_direction(CONFIG_TMS_SWDIO_GPIO, GPIO_MODE_INPUT_OUTPUT);   \
		gpio_set_direction(CONFIG_TCK_SWCLK_GPIO, GPIO_MODE_OUTPUT);         \
		if (CONFIG_TMS_SWDIO_DIR_GPIO >= 0)                                  \
			gpio_set_direction(CONFIG_TMS_SWDIO_DIR_GPIO, GPIO_MODE_OUTPUT); \
		if (CONFIG_TMS_SWDIO_DIR_GPIO >= 0)                                  \
			gpio_set_level(CONFIG_TMS_SWDIO_DIR_GPIO, 0);                    \
	} while (0)

#define TMS_PIN CONFIG_TMS_SWDIO_GPIO
#define TCK_PIN CONFIG_TCK_SWCLK_GPIO
#define TDI_PIN CONFIG_TDI_GPIO
#define TDO_PIN CONFIG_TDO_GPIO

#define SWDIO_PIN CONFIG_TMS_SWDIO_GPIO
#define SWCLK_PIN CONFIG_TCK_SWCLK_GPIO
#define SRST_PIN  CONFIG_SRST_GPIO

#define SWCLK_PORT 0
#define SWDIO_PORT 0

#define gpio_set(port, pin)     \
	do {                        \
		gpio_set_level(pin, 1); \
	} while (0)
#define gpio_clear(port, pin)   \
	do {                        \
		gpio_set_level(pin, 0); \
	} while (0)
#define gpio_get(port, pin) gpio_get_level(pin)
#define gpio_set_val(port, pin, value) \
	if (value) {                       \
		gpio_set(port, pin);           \
	} else {                           \
		gpio_clear(port, pin);         \
	}

#define GPIO_INPUT  GPIO_MODE_INPUT
#define GPIO_OUTPUT GPIO_MODE_OUTPUT

#define PLATFORM_IDENT "pico"

extern uint32_t swd_delay_cnt;

#endif /* ATTACK_PLATFORM_H_ */
