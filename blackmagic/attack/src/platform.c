#include "config.h"
// #include <target/cortexm.h>
#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t platform_max_frequency = 100000000;

uint32_t platform_max_frequency_get(void)
{
	return platform_max_frequency;
}

void platform_max_frequency_set(const uint32_t frequency)
{
	platform_max_frequency = frequency;
}

void platform_nrst_set_val(bool assert)
{
	gpio_set_dir(RESET_PIN, assert ? GPIO_OUT : GPIO_IN);
}

bool platform_nrst_get_val(void)
{
	return !gpio_get(RESET_PIN);
}

void platform_delay(uint32_t ms)
{
	sleep_ms(ms);
}

const char *platform_target_voltage(void)
{
	return "3300";
}

void platform_target_clk_output_enable(bool enable)
{
	(void)enable;
}

uint32_t platform_time_ms(void)
{
	return time_us_64() / 1000;
}

int platform_hwversion(void)
{
	return 2040;
}

int32_t semihosting_request(void *const target, const uint32_t syscall, const uint32_t r1)
{
	return -1;
}

void gdb_outf(const char *const fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void gdb_out(const char *buf)
{
	printf("%s", buf);
}

//  void stimpik_cortexm_pc_write(target_s *target, const uint32_t val)
// {
// 	target_mem_write32(target, CORTEXM_DCRDR, val);
// 	target_mem_write32(target, CORTEXM_DCRSR, CORTEXM_DCRSR_REGWnR | 0x0fU);
// }
