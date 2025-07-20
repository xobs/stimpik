#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CMD_BITS  1
#define DATA_BITS 32

static volatile uint32_t *IDCODE = (uint32_t *)0xe0042000;

static volatile uint32_t *RCC_APB2ENR_F1 = (uint32_t *)0x40021018u;
static volatile uint32_t *AFIO_MAPR_F1 = (uint32_t *)0x40010004u;

static volatile uint32_t *GPIOA_CRL_F1 = (uint32_t *)0x40010800u;
static volatile uint32_t *GPIOA_CRH_F1 = (uint32_t *)0x40010804u;
static volatile uint32_t *GPIOA_IDR_F1 = (uint32_t *)0x40010808u;
static volatile uint32_t *GPIOA_BSRR_F1 = (uint32_t *)0x40010810u;

static volatile uint32_t *GPIOB_CRL_F1 = (uint32_t *)0x40010C00u;
static volatile uint32_t *GPIOB_CRH_F1 = (uint32_t *)0x40010C04u;
static volatile uint32_t *GPIOB_IDR_F1 = (uint32_t *)0x40010C08u;
static volatile uint32_t *GPIOB_BSRR_F1 = (uint32_t *)0x40010C10u;

#define DIO GPIOA, P13
#define CLK GPIOA, P14

enum Bank {
	GPIOA,
	GPIOB,
};

enum Pin {
	P0 = 0,
	P1 = 1,
	P2 = 2,
	P3 = 3,
	P4 = 4,
	P5 = 5,
	P6 = 6,
	P7 = 7,
	P8 = 8,
	P9 = 9,
	P10 = 10,
	P11 = 11,
	P12 = 12,
	P13 = 13,
	P14 = 14,
	P15 = 15,
};

enum Dir {
	INPUT,
	OUTPUT,
	OUTPUT_OD,
};

static void (*gpio_out)(enum Bank bank, enum Pin pin, bool on);
static bool (*gpio_get)(enum Bank bank, enum Pin pin);
static void (*gpio_dir)(enum Bank bank, enum Pin pin, enum Dir dir);

static uint32_t readl(const volatile void *addr)
{
	return *(volatile uint32_t *)addr;
}

static void writel(volatile void *addr, uint32_t val)
{
	*(volatile uint32_t *)addr = val;
}

static void delay(uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
		__asm__("nop");
	}
}

static void gpio_out_f1(enum Bank bank, enum Pin pin, bool on)
{
	if (on) {
		pin += 0;
	} else {
		pin += 16;
	}

	switch (bank) {
	case GPIOA:
		writel(GPIOA_BSRR_F1, 1 << pin);
		break;
	case GPIOB:
		writel(GPIOB_BSRR_F1, 1 << pin);
		break;
	}
}

static bool gpio_get_f1(enum Bank bank, enum Pin pin)
{
	switch (bank) {
	case GPIOA:
		return !!(readl(GPIOA_IDR_F1) & (1 << pin));
		break;
	case GPIOB:
		return !!(readl(GPIOB_IDR_F1) & (1 << pin));
		break;
	}
	return 0;
}

static void gpio_dir_f1(enum Bank bank, enum Pin pin, enum Dir dir)
{
	volatile uint32_t *reg = NULL;
	switch (bank) {
	case GPIOA:
		if (pin < 8) {
			reg = GPIOA_CRL_F1;
		} else {
			reg = GPIOA_CRH_F1;
			pin -= 8;
		}
		break;
	case GPIOB:
		if (pin < 8) {
			reg = GPIOB_CRL_F1;
		} else {
			reg = GPIOB_CRH_F1;
			pin -= 8;
		}
		break;
	}

	uint32_t dir_value = 4;
	if (dir == INPUT) {
		dir_value = 4;
	} else if (dir == OUTPUT) {
		dir_value = 3;
	} else if (dir == OUTPUT_OD) {
		dir_value = 7;
	}
	writel(reg, (readl(reg) & ~(0xF << (pin * 4))) | (dir_value << (pin * 4)));
}

static void gpio_init(bool repurpose_swd)
{
	switch (readl(IDCODE) & 0xFFF) {
	case 0x410:
	default:
		// Ungate GPIOA and GPIOB, as well as AFIO
		writel(RCC_APB2ENR_F1, (1 << 2) | (1 << 3) | (1 << 0));

		gpio_out = gpio_out_f1;
		gpio_get = gpio_get_f1;
		gpio_dir = gpio_dir_f1;

		if (repurpose_swd) {
			// Disable the debug port, since we'll use it for GPIO access. Without
			// this, the pins will stay mapped as SWD.
			writel(AFIO_MAPR_F1, (4 << 24));

			// Set SWDIO (PA13) and SWCLK (PA14) to push-pull GPIO mode
			gpio_dir(DIO, INPUT);
			gpio_dir(CLK, INPUT);
		}

		// Configure LEDs on PA0 and PA3 as outputs
		gpio_dir(GPIOA, P0, OUTPUT);
		gpio_dir(GPIOA, P3, OUTPUT);

		// Configure PB7 as GPIO output
		gpio_dir(GPIOB, P7, OUTPUT);

		// Configure PB15 as GPIO output
		gpio_dir(GPIOB, P15, OUTPUT);

		break;
	}
}

static void led1(bool on)
{
	gpio_out(GPIOA, P3, !on);
}

static void led2(bool on)
{
	gpio_out(GPIOA, P0, !on);
}

static void led3(bool on)
{
	gpio_out(GPIOB, P15, !on);
}

static void led4(bool on)
{
	gpio_out(GPIOB, P7, !on);
}

int main_stage1(void)
{
	int count = 0;
	gpio_init(false);
	while (1) {
		delay(20);
		led2(!!(count & 8192));
		led4(!!(count & 8192));
		led3(count > 131072);
		count += 1;
	}
}

#define HOST_TO_TARGET_PREFIX 0x5ad7
#define TARGET_TO_HOST_PREFIX 0x734f

enum CommandState {
	/// @brief  Look for magic prefix HOST_TO_TARGET_PREFIX
	CMD_READ_PREFIX,
	CMD_READ_DATA,
	CMD_TURNAROUND_RW,
	/// @brief Writing prefix TARGET_TO_HOST_PREFIX
	CMD_WRITE_PREFIX,
	CMD_WRITE_DATA,
	CMD_TURNAROUND_WR,
};

static uint32_t handle_packet(uint32_t packet)
{
	switch (packet & 0xff) {
	case 0:
		return 0xdeadbeef;
	case 1:
		return 0xf00fc7c8;
	case 2:
		return 0x12345678;
	case 3:
		return 0x87654321;
	default:
		return 0x5aa55a55;
	}
}

enum TwiState {
	TWI_STATE_IDLE,
	TWI_STATE_START,
	TWI_STATE_RW,
	TWI_STATE_WRITE,
	TWI_STATE_READ,
};

struct Twi {
	uint32_t reg;
	enum TwiState state;
	uint8_t bit;
	uint8_t cmd;
	bool is_write;
};

int main_stage2(void)
{
	int count = 0;
	struct Twi twi = {
		.reg = 0,
		.state = TWI_STATE_IDLE,
		.bit = 0,
		.cmd = 0,
	};
	gpio_init(true);
	led1(false);
	led2(false);
	led3(false);
	led4(true);

	delay(60000);

	// Toggle swclk to let the other side know we're alive
	bool last_swclk = gpio_get(CLK);
	bool last_swdio = gpio_get(DIO);
	enum CommandState state = CMD_READ_PREFIX;

	while (1) {
		// led1(!!(count & 1024));
		// led2(!!(count & 512));
		// led3(!!(count & 1024));
		// led4(!!(count & 4096));

		bool clk = gpio_get(CLK);
		bool dio = gpio_get(DIO);
		bool falling_clk = !clk && last_swclk;
		bool rising_clk = clk && !last_swclk;
		bool falling_dio = !dio && last_swdio;
		bool rising_dio = dio && !last_swdio;
		last_swclk = clk;
		last_swdio = dio;

		// START condition -- the only time DIO falls when CLK is low
		if (falling_dio && !clk) {
			twi.state = TWI_STATE_START;
			twi.bit = 0;
			twi.cmd = 0;
			led4(true);
		} else if (falling_clk && (twi.state == TWI_STATE_START)) {
			led1(false);
			led2(true);
			led3(false);
			led4(false);
			if (dio) {
				twi.cmd |= 1 << twi.bit;
			}
			twi.bit += 1;
			if (twi.bit >= CMD_BITS) {
				twi.state = TWI_STATE_RW;
			}
		} else if (falling_clk && (twi.state == TWI_STATE_RW)) {
			led2(false);
			twi.is_write = dio;
			if (twi.is_write) {
				twi.state = TWI_STATE_WRITE;
				gpio_dir(DIO, INPUT);
				twi.reg = 0;
				led3(true);
			} else {
				twi.state = TWI_STATE_READ;
				gpio_out(DIO, false);
				gpio_dir(DIO, OUTPUT);
				// XXX HACK -- increment the register to show we're alive
				twi.reg += 1;
				led1(true);
			}
			twi.bit = 0;
		} else if (falling_clk && (twi.state == TWI_STATE_WRITE)) {
			twi.reg |= dio << twi.bit;
			twi.bit += 1;
			if (twi.bit >= DATA_BITS) {
				gpio_dir(DIO, INPUT);
				twi.state = TWI_STATE_IDLE;
			}
		} else if (rising_clk && (twi.state == TWI_STATE_READ)) {
			gpio_out(DIO, twi.reg & (1 << twi.bit));
			twi.bit += 1;
			if (twi.bit >= DATA_BITS) {
				gpio_dir(DIO, INPUT);
				twi.state = TWI_STATE_IDLE;
			}
		}
		count += 1;
	}
}

void alert_crash(uint32_t crash_id)
{
	(void)crash_id;
	// 	writeStr("!!! EXCEPTION !!!\r\nID: ");
	// 	writeByte(crashId);
	// 	writeStr("\r\nRestart required!\r\n\r\n");
	// 	*((uint32_t *) 0xE000ED0C) = 0x05FA0004u;
	static int error_count = 0;
	led1(true);
	led2(false);
	while (1) {
		led3(!!(error_count & 2));
		led4(!!(error_count & 4));
		error_count += 1;
		delay(1000);
	}
}
