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

#include "hardware/gpio.h"

#define SET_RUN_STATE(state)
#define SET_IDLE_STATE(state)
#define SET_ERROR_STATE(state) gpio_put(CONFIG_LED_GPIO, !state)

#define DEBUG(x, ...)

#define SWDIO_MODE_FLOAT()                \
	do {                                  \
		gpio_set_dir(SWDIO_PIN, GPIO_IN); \
	} while (0)

#define SWDIO_MODE_DRIVE()                 \
	do {                                   \
		gpio_set_dir(SWDIO_PIN, GPIO_OUT); \
	} while (0)

#define SWDIO_PIN 7
#define SWCLK_PIN 6
#define SRST_PIN  4

#define SWCLK_PORT 0
#define SWDIO_PORT 0

#define gpio_set(port, pin) \
	do {                    \
		gpio_put(pin, 1);   \
	} while (0)
#define gpio_clear(port, pin) \
	do {                      \
		gpio_put(pin, 0);     \
	} while (0)
#define gpio_get(port, pin) gpio_get(pin)
#define gpio_set_val(port, pin, value) \
	if (value) {                       \
		gpio_set(port, pin);           \
	} else {                           \
		gpio_clear(port, pin);         \
	}

#define PLATFORM_IDENT "pico"

extern uint32_t swd_delay_cnt;

#endif /* ATTACK_PLATFORM_H_ */
