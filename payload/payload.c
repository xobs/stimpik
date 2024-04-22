#include <stdbool.h>
#include <stdint.h>

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

static void (*gpio_out)(enum Bank bank, enum Pin pin, bool on);
static bool (*gpio_get)(enum Bank bank, enum Pin pin);

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
	for (int i = 0; i < count; i++) {
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

static void gpio_init(bool repurpose_swd)
{
	switch (readl(IDCODE) & 0xFFF) {
	case 0x410:
	default:
		// Ungate GPIOA and GPIOB, as well as AFIO
		writel(RCC_APB2ENR_F1, (1 << 2) | (1 << 3) | (1 << 0));

		if (repurpose_swd) {
			// Disable the debug port, since we'll use it for GPIO access. Without
			// this, the pins will stay mapped as SWD.
			writel(AFIO_MAPR_F1, (4 << 24));

			// Set SWDIO (PA13) and SWCLK (PA14) to push-pull GPIO mode
			writel(GPIOA_CRH_F1, (readl(GPIOA_CRH_F1) & ~0x0FF00000) | 0x03300000);
		}

		// Configure LEDs on PA0 and PA3 as outputs
		writel(GPIOA_CRL_F1, (readl(GPIOA_CRL_F1) & ~0x0000F00F) | 0x00003003);

		// Configure PB7 as GPIO output
		writel(GPIOB_CRL_F1, (readl(GPIOB_CRL_F1) & ~0xF0000000) | 0x30000000);

		// Configure PB15 as GPIO output
		writel(GPIOB_CRH_F1, (readl(GPIOB_CRH_F1) & ~0xF0000000) | 0x30000000);

		gpio_out = gpio_out_f1;
		gpio_get = gpio_get_f1;
		break;
	}
}

static void led1(bool on)
{
	gpio_out(GPIOA, P0, on);
}

static void led2(bool on)
{
	gpio_out(GPIOA, P3, on);
}

static void led3(bool on)
{
	gpio_out(GPIOB, P15, on);
}

static void led4(bool on)
{
	gpio_out(GPIOB, P7, on);
}

static void send_bit(bool bit)
{
	gpio_out(GPIOA, P13, bit);

	// Toggle the clock bit to indicate a sample is ready
	gpio_out(GPIOA, P14, false);
	delay(10);
	gpio_out(GPIOA, P14, true);
	delay(10);
}

int main_stage1(void)
{
	static int count = 0;
	gpio_init(false);
	while (1) {
		delay(20);
		led2(!!(count & 8192));
		led4(!!(count & 8192));
		led3(count > 131072);
		count += 1;
	}
}

int main_stage2(void)
{
	static int count = 0;
	gpio_init(true);
	led1(false);
	led2(false);
	led3(false);
	led4(true);

	// Toggle swclk to let the other side know we're alive
	while (1) {
		led1(!!(count & 512));
		led2(!!(count & 1024));
		led3(!!(count & 2048));
		led4(!!(count & 4096));
		send_bit(true);
		send_bit(false);
		count += 1;
	}
	// // Flash size register, RM0008, page 1076:
	// // https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

	// uint32_t flash_size = *(uint32_t*) 0x1FFFF7E0 & 0xFFFF;
	// if(flash_size == 64)        // Force reading of the entire 128KB flash in 64KB devices, often used.
	// 	flash_size = 128;

	// /* Print start magic to inform the attack board that
	//    we are going to dump */
	// for (uint32_t i = 0; i < sizeof(DUMP_START_MAGIC); i++) {
	// 	writeChar(DUMP_START_MAGIC[i]);
	// }

	// uint32_t const * addr = (uint32_t*) 0x08000000;
	// while (((uintptr_t) addr) < (0x08000000U + (flash_size * 1024U) )) {
	// 	writeWordBe(*addr);
	// 	++addr;
	// }
}

// /* hex must have length 8 */
// uint32_t hexToInt(uint8_t const * const hex)
// {
// 	uint32_t ind = 0u;
// 	uint32_t res = 0u;

// 	for (ind = 0; ind < 8; ++ind) {
// 		uint8_t chr = hex[ind];
// 		uint32_t val = 0u;

// 		res <<= 4u;

// 		if ((chr >= '0') && (chr <= '9')) {
// 			val = chr - '0';
// 		} else if ((chr >= 'a') && (chr <= 'f')) {
// 			val = chr - 'a' + 0x0a;
// 		} else if ((chr >= 'A') && (chr <= 'F')) {
// 			val = chr - 'A' + 0x0a;
// 		} else {
// 			val = 0u;
// 		}
// 		res |= val;
// 	}

// 	return res;
// }

// void readChar( uint8_t const chr )
// {
// #define CMDBUF_LEN (64u)
// 	static uint8_t cmdbuf[CMDBUF_LEN] = {0u};
// 	static uint32_t cmdInd = 0u;

// 	switch (chr) {
// 	case '\n':
// 	case '\r':
// 		cmdbuf[cmdInd] = 0u;
// 		if (cmdInd != 0) {
// 			writeStr("\r\n");
// 		}
// 		readCmd(cmdbuf);
// 		cmdInd = 0u;
// 		writeStr("\r\n> ");
// 		{
// 			uint32_t ind = 0u;
// 			for (ind = 0; ind<CMDBUF_LEN; ++ind) {
// 				cmdbuf[ind]=0x00u;
// 			}
// 		}
// 		break;

// 	case 8:
// 	case 255:
// 	case 127: /* TODO backspace */
// 		if (cmdInd > 0u)
// 			--cmdInd;
// 		writeChar(chr);
// 		break;

// 	default:
// 		if (cmdInd < (CMDBUF_LEN - 1)) {
// 			cmdbuf[cmdInd] = chr;
// 			++cmdInd;
// 			writeChar(chr);
// 		}
// 		break;
// 	}
// }

// void readCmd( uint8_t const * const cmd )
// {
// 	switch (cmd[0]) {
// 	case 0:
// 		return;
// 		break;

// 	/* read 32-bit command */
// 	case 'r':
// 	case 'R':
// 		/* r 08000000 00000100 */
// 		readMem(hexToInt(&cmd[2]), hexToInt(&cmd[11]));
// 		break;

// 	/* write 32-bit command */
// 	case 'w':
// 	case 'W':
// 		/* w 20000000 12345678 */
// 		writeMem(hexToInt(&cmd[2]), hexToInt(&cmd[11]));
// 		break;

// 	/* Dump all flash */
// 	case 'd':
// 	case 'D':
// 		writeStr("\r\n\r\n");
// 		{
// 			uint32_t const * addr = (uint32_t*) 0x08000000;
// 			uint32_t br = 8u;
// 			while (((uintptr_t) addr) < (0x08000000 + 64u * 1024u)) {
// 				if (br == 8u) {
// 					writeStr("\r\n[");
// 					writeWordBe((uint32_t) addr);
// 					writeStr("]: ");
// 					br = 0u;
// 				}

// 				writeWordBe(*addr);
// 				writeChar(' ');
// 				++addr;
// 				++br;
// 			}
// 		}
// 		writeStr("\r\n\r\n");
// 		break;

// 	/* Help command */
// 	case 'h':
// 	case 'H':
// 		writeStr(strHelp);
// 		break;

// 	/* Reboot */
// 	case 's':
// 	case 'S':
// 		writeStr("Rebooting...\r\n\r\n");
// 		*((uint32_t *) 0xE000ED0C) = 0x05FA0004u;
// 		break;

// 	/* exit */
// 	case 'e':
// 	case 'E':
// 		writeStr("Bye.\r\n");
// 		while (1) {
// 			__asm__ volatile("wfi");
// 		}
// 		break;

// 	default:
// 		writeStr("Unknown command: ");
// 		writeStr(cmd);
// 		writeStr("\r\n");
// 		break;
// 	}
// }

// const uint8_t txtMap[] = "0123456789ABCDEF";

// void writeByte( uint8_t b )
// {
// 	writeChar(txtMap[b >> 4]);
// 	writeChar(txtMap[b & 0x0F]);
// }

// void writeStr( uint8_t const * const str )
// {
// 	uint32_t ind = 0u;

// 	while (str[ind]) {
// 		writeChar(str[ind]);
// 		++ind;
// 	}
// }

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

// void readMem(uint32_t const addr, uint32_t const len)
// {
// 	uint32_t it = 0u;
// 	uint32_t addrx = 0u;
// 	uint32_t lenx = 0u;

// 	lenx = len;
// 	if (lenx == 0u) {
// 		lenx = 4u;
// 	}

// 	for (it = 0u; it < (lenx / 4u); ++it) {
// 		addrx = addr + it*4u;
// 		writeStr("Read [");
// 		writeWordBe(addrx);
// 		writeStr("]: ");
// 		writeWordBe(*((uint32_t*)addrx));
// 		writeStr("\r\n");
// 	}
// }

// void writeMem(uint32_t const addr, uint32_t const data)
// {
// 	writeStr("Write [");
// 	writeWordBe(addr);
// 	writeStr("]: ");
// 	writeWordBe(data);
// 	*((uint32_t*) addr) = data;
// 	writeStr("\r\n");
// }
