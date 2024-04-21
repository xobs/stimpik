#include "blackmagic/blackmagic/src/include/exception.h"
#include "blackmagic/blackmagic/src/include/target.h"
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool adiv5_swd_scan(const uint32_t targetid);
void stimpik_cortexm_pc_write(target_s *target, const uint32_t val);

int bmp_loader(const char *data, uint32_t length, uint32_t offset, bool check_only)
{
	target_s *cur_target;
	target_controller_s cur_controller;
	memset(&cur_controller, 0, sizeof(cur_controller));
	printf("Starting scan...\n");

	volatile struct exception e;
	TRY_CATCH (e, EXCEPTION_ALL) {
		printf("Calling adiv5_swd_scan...\n");
		if (!adiv5_swd_scan(0)) {
			printf("No target found\n");
			return -1;
		}
		printf("Attempting to attach to index 1...\n");
		cur_target = target_attach_n(1, &cur_controller);
		if (cur_target == NULL) {
			printf("Unable to attach to target\n");
			return -1;
		}
		printf("Success!\n");
		// Note: TRY_CATCH(..) apparently has a loop in it, so it's effectively
		// while (1) { try { ... } catch { ... } }
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
	for (check_offset = 0; check_offset < length / 4; check_offset += 1) {
		if (((uint32_t *)readback)[check_offset] != ((uint32_t *)data)[check_offset]) {
			printf("Readback does not match at offset 0x%08x! %08x -> %08x\n", check_offset * 4 + 0x20000000,
				((uint32_t *)data)[check_offset], ((uint32_t *)readback)[check_offset]);
		}
	}
	if (memcmp(data, readback, length)) {
		printf("Readback does not match!\n");
		free(readback);
		return -1;
	}
	free(readback);

	// stimpik_cortexm_pc_write(cur_target, ((uint32_t *)data)[1]);
	// target_halt_resume(cur_target, false);

	// sleep_ms(10000);

    target_detach(cur_target);

	return 0;
}
