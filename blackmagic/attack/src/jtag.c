#include "jtagtap.h"
#include <stdio.h>

jtag_proc_s jtag_proc;

static void misuse_jtagtap_reset(void)
{
	printf("ERROR: JTAG should not be used on this platform\n");
}

static bool misuse_jtagtap_next(const bool dTMS, const bool dTDI)
{
	(void)dTMS;
	(void)dTDI;
	printf("ERROR: JTAG should not be used on this platform\n");
}

static void misuse_jtagtap_tms_seq(uint32_t MS, size_t ticks)
{
	(void)MS;
	(void)ticks;
	printf("ERROR: JTAG should not be used on this platform\n");
}

static void misuse_jtagtap_tdi_tdo_seq(uint8_t *DO, const bool final_tms, const uint8_t *DI, size_t ticks)
{
	(void)DO;
	(void)final_tms;
	(void)DI;
	(void)ticks;
	printf("ERROR: JTAG should not be used on this platform\n");
}

static void misuse_jtagtap_tdi_seq(const bool final_tms, const uint8_t *data_in, size_t clock_cycles)
{
	(void)final_tms;
	(void)data_in;
	(void)clock_cycles;
	printf("ERROR: JTAG should not be used on this platform\n");
}

void jtagtap_init(void)
{
	jtag_proc.jtagtap_reset = misuse_jtagtap_reset;
	jtag_proc.jtagtap_next = misuse_jtagtap_next;
	jtag_proc.jtagtap_tms_seq = misuse_jtagtap_tms_seq;
	jtag_proc.jtagtap_tdi_tdo_seq = misuse_jtagtap_tdi_tdo_seq;
	jtag_proc.jtagtap_tdi_seq = misuse_jtagtap_tdi_seq;
}
