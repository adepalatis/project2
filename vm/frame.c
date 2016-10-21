#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "frame.h"

struct frame {
	void* u_page;
	bool in_use;
	bool pinned;
};

struct frame* palloc_get_multiple_frames(enum palloc_flags flags, size_t frame_cnt) {

}

struct frame* palloc_get_frame(enum palloc_flags flags) {
	
}

