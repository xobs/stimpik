#include <stdbool.h>
#include <stdint.h>

typedef uint32_t *reg_t;

static const volatile reg_t RCC_APB2ENR = (reg_t)0x40021018u;

static const volatile reg_t GPIOA_CRL = (reg_t)0x40010800u;
static const volatile reg_t GPIOA_CRH = (reg_t)0x40010804u;
static const volatile reg_t GPIOA_BSRR = (reg_t)0x40010810u;
static const volatile reg_t GPIOA_BRR = (reg_t)0x40010814u;

static const volatile reg_t GPIOB_CRL = (reg_t)0x40010C00u;
static const volatile reg_t GPIOB_CRH = (reg_t)0x40010C04u;
static const volatile reg_t GPIOB_BSRR = (reg_t)0x40010C10u;
static const volatile reg_t GPIOB_BRR = (reg_t)0x40010C14u;

// SWDIO: PA13
// SWCLK: PA14

static void configure_swd_as_gpio(void)
{
	// Ungate GPIOA
	*RCC_APB2ENR |= (1 << 2);

	// Set SWDIO (PA13) and SWCLK (PA14) to push-pull GPIO mode
	*GPIOA_CRH = (*GPIOA_CRH & ~0x0FF000000) | 0x033000000;
}

static void configure_leds(void)
{
	// Ungate GPIOA and GPIOB
	*RCC_APB2ENR |= (1 << 2) | (1 << 3);

	// Configure PA0 and PA3 as outputs
	*GPIOA_CRL = (*GPIOA_CRL & ~0x0000F00F) | 0x00003003;

	// Configure PB2 as GPIO output
	*GPIOB_CRL = (*GPIOB_CRL & ~0x000000F00) | 0x000000300;

	// Configure PB15 as GPIO output
	*GPIOB_CRH = (*GPIOB_CRH & ~0xF00000000) | 0x300000000;
}

static void led1(bool on)
{
	*(on ? GPIOA_BSRR : GPIOA_BRR) = 1 << 0;
}

static void led2(bool on)
{
	*(on ? GPIOA_BSRR : GPIOA_BRR) = 1 << 3;
}

static void led3(bool on)
{
	*(on ? GPIOB_BSRR : GPIOB_BRR) = 1 << 2;
}

static void led4(bool on)
{
	*(on ? GPIOB_BSRR : GPIOB_BRR) = 1 << 15;
}

static void send_bit(bool bit)
{
	int i;
	*(bit ? GPIOA_BSRR : GPIOA_BRR) = 1 << 13;

	// Toggle the clock bit to indicate a sample is ready
	*GPIOA_BSRR = (1 << 14);
	for (i = 0; i < 1000; i++) {
		__asm__("nop");
	}
	*GPIOA_BRR = (1 << 14);
	for (i = 0; i < 1000; i++) {
		__asm__("nop");
	}
}

int main(void)
{
	configure_leds();
	led1(false);
	led2(true);
	configure_swd_as_gpio();
	int count = 0;
	// Toggle swclk to let the other side know we're alive
	while (1) {
		led3(count&9);
		led4(count&6);
		send_bit(1);
		send_bit(0);
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

void alertCrash(uint32_t crashId)
{
	// 	writeStr("!!! EXCEPTION !!!\r\nID: ");
	// 	writeByte(crashId);
	// 	writeStr("\r\nRestart required!\r\n\r\n");
	// 	*((uint32_t *) 0xE000ED0C) = 0x05FA0004u;
	while (1)
		;
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
