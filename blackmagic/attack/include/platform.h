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

#include <hardware/gpio.h>
#include <stdio.h>

#define SET_RUN_STATE(state)
#define SET_IDLE_STATE(state)
#define SET_ERROR_STATE(state) gpio_put(CONFIG_LED_GPIO, !state)

#define DEBUG(x, ...) printf("bmp: " x, ##__VA_ARGS__)

#define PLATFORM_IDENT "pico"

extern uint32_t swd_delay_cnt;

#endif /* ATTACK_PLATFORM_H_ */
