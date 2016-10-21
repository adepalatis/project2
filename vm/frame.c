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
	struct pool* pool = flags & PAL_USER ? &user_pool : &kernel_pool;
	
	if(frame_cnt == 0) {
		return NULL;
	}

	// lock_acquire(&pool->lock);

}

struct frame* palloc_get_frame(enum palloc_flags flags) {
	return palloc_get_multiple_frames(flags, 1);
}

