#include "blackmagic/blackmagic/src/include/exception.h"
#include "blackmagic/blackmagic/src/include/target.h"
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool adiv5_swd_scan(const uint32_t targetid);

int bmp_loader(const char *data, uint32_t length, uint32_t offset)
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

	const char *const core_name = target_core_name(cur_target);
	printf("Target driver: %s\n", target_driver_name(cur_target));
	printf("Target designer: %u\n", target_designer(cur_target));
	printf("Target part ID: %u\n", target_part_id(cur_target));
	printf("Target core: %s\n", core_name ? core_name : "<unknown>");
    if (target_mem_write(cur_target, offset, data, length)) {
        printf("Unable to write payload!\n");
        return -1;
    }
    char readback[length];
    if (target_mem_read(cur_target, readback, offset, length)) {
        printf("Unable to read back payload!\n");
        return -1;
    }
    if (memcmp(data, readback, length)) {
        printf("Readback does not match!\n");
        return -1;
    }

	return 0;
}
