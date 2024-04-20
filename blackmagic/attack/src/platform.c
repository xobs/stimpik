#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

static uint32_t platform_max_frequency = 100000000;

uint32_t platform_max_frequency_get(void)
{
	return platform_max_frequency;
}

void platform_max_frequency_set(const uint32_t frequency)
{
	platform_max_frequency = frequency;
}

void platform_nrst_set_val(bool nrst)
{
	(void)nrst;
}

bool platform_nrst_get_val(void)
{
	return false;
}

void platform_delay(uint32_t ms)
{
	(void)ms;
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
	static int time;
	return time += 1;
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
