#include <stdbool.h>
#include <stdint.h>
#include "blackmagic/blackmagic/src/include/exception.h"
#include "blackmagic/blackmagic/src/include/target.h"


bool adiv5_swd_scan(const uint32_t targetid);

int bmp_loader(void) {
	volatile struct exception e;
    target_s *cur_target;
    target_controller_s cur_controller;
	TRY_CATCH (e, EXCEPTION_ALL) {
		adiv5_swd_scan(0);
		cur_target = target_attach_n(1, &cur_controller);
	}
    if (e.type) {
        printf("Unable to attach: %s\n", e.msg);
        return -1;
    }
    return 0;
}
