#include "blackmagic/blackmagic/src/include/exception.h"
#include "blackmagic/blackmagic/src/include/target.h"
#include "config.h"
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool adiv5_swd_scan(const uint32_t targetid);
// void stimpik_pc_write(target_s *target, const uint32_t val);
// void stimpik_sp_write(target_s *target, const uint32_t val);
void stimpik_start_stub(target_s *target, uint32_t pc, uint32_t sp);

int bmp_loader_launcher(const char *data, uint32_t length, uint32_t offset, int stage)
{
	target_s *cur_target;
	target_controller_s cur_controller;
	memset(&cur_controller, 0, sizeof(cur_controller));

	volatile struct exception e;
	TRY_CATCH (e, EXCEPTION_ALL) {
		if (!adiv5_swd_scan(0)) {
			printf("No target found\n");
			return -1;
		}
		cur_target = target_attach_n(1, &cur_controller);
		if (cur_target == NULL) {
			printf("Unable to attach to target\n");
			return -1;
		}
		// Note: TRY_CATCH(..) apparently has a loop in it, so it's effectively
		// while (1) { try { ... } catch { ... } }
		// Hence we need to "break" here.
		break;
	}
	if (e.type) {
		printf("Unable to attach: %s\n", e.msg);
		return -1;
	}

	target_halt_request(cur_target);

	if (target_mem_write(cur_target, offset, data, length)) {
		printf("Unable to write payload!\n");
		return -1;
	}

	uint32_t sp = ((uint32_t *)data)[0];
	uint32_t pc = 0;
	if (stage == 1)
		pc = ((uint32_t *)data)[1]; // Stage 1
	else if (stage == 2)
		pc = ((uint32_t *)data)[8]; // Stage 2
	printf("Jumping to stage %d -- Set payload PC to 0x%08x and SP to 0x%08x\n", stage, pc, sp);
	stimpik_start_stub(cur_target, pc, sp);

	int halt_reason = target_halt_poll(cur_target, NULL);
	if (halt_reason != 0) {
		printf("Target halted!   Reason: %d\r\n", halt_reason);
		return -1;
	}
	printf("Target is now running\n");

	// {
	// 	platform_delay(500);
	// 	void stimpik_regs_read(target_s * target, uint32_t regs[20 + 33]);
	// 	uint32_t regs[20 + 33] = {0};
	// 	stimpik_regs_read(cur_target, regs);
	// 	printf("PC now at: 0x%08x  SP now at: 0x%08x\n", regs[15], regs[13]);
	// }

	return 0;
}

int bmp_loader(const char *data, uint32_t length, uint32_t offset, bool check_only)
{
	target_s *cur_target;
	target_controller_s cur_controller;
	memset(&cur_controller, 0, sizeof(cur_controller));

	volatile struct exception e;
	TRY_CATCH (e, EXCEPTION_ALL) {
		if (!adiv5_swd_scan(0)) {
			printf("No target found\n");
			return -1;
		}
		cur_target = target_attach_n(1, &cur_controller);
		if (cur_target == NULL) {
			printf("Unable to attach to target\n");
			return -1;
		}
		// Note: TRY_CATCH(..) apparently has a loop in it, so it's effectively
		// while (1) { try { ... } catch { ... } }
		// Hence we need to "break" here.
		break;
	}
	if (e.type) {
		printf("Unable to attach: %s\n", e.msg);
		return -1;
	}

	target_halt_request(cur_target);

	const char *const core_name = target_core_name(cur_target);
	printf("Target driver: %s  designer: %u, part ID: %u, core: %s\n", target_driver_name(cur_target),
		target_designer(cur_target), target_part_id(cur_target), core_name ? core_name : "<unknown>");

	uint32_t debug_code = 0;
	target_mem_read(cur_target, &debug_code, 0xe0042000, 4);
	printf("Debug code: %08x -- family: %03x  Revision: %04x\n", debug_code, debug_code & 0xfff, debug_code >> 16);

	if (!check_only) {
		if (target_mem_write(cur_target, offset, data, length)) {
			printf("Unable to write payload!\n");
			return -1;
		}
	}

	char *readback = malloc(length);
	if (target_mem_read(cur_target, readback, offset, length)) {
		printf("Unable to read back payload!\n");
		free(readback);
		return -1;
	}
	int check_offset = 0;
	bool error = false;
	for (check_offset = 0; check_offset < length / 4; check_offset += 1) {
		if (((uint32_t *)readback)[check_offset] != ((uint32_t *)data)[check_offset]) {
			printf("Readback does not match at offset 0x%08x! %08x -> %08x\n", check_offset * 4 + 0x20000000,
				((uint32_t *)data)[check_offset], ((uint32_t *)readback)[check_offset]);
			error = true;
		}
	}
	if (error) {
		free(readback);
		return -1;
	}

	free(readback);

	// Put device into reset to prevent whatever's running from
	// stomping on the payload.
	gpio_set_dir(RESET_PIN, GPIO_OUT);
	// target_halt_resume(cur_target, false);
	target_detach(cur_target);

	return 0;
}
