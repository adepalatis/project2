#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "frame.h"

struct frame {
	void* u_page;
	bool in_use;
	bool pinned;
};

struct frame* get_multiple_frames(enum palloc_flags flags, size_t frame_cnt) {
	struct pool* pool = flags & PAL_USER ? &user_pool : &kernel_pool;
	void* frames;
	size_t frame_idx;

	if(frame_cnt == 0) {
		return NULL;
	}

	lock_acquire(&pool->lock);
	frame_idx = bitmap_scan_and_flip(pool->used_map, 0, frame_cnt, false);
	lock_release(&pool->lock);

	if(frame_idx != BITMAP_ERROR) {
		frames = pool->base + PGSIZE * frame_idx;
	} else {
		frames = NULL;
	}

	if(frames != NULL) {
		if(flags & PAL_ZERO) {
			memset(frames, 0, PGSIZE * frame_cnt);
		}
	} else if(flags & PAL_ASSERT) {
		PANIC("palloc_get: out of frames");
	}

	return frames;
}

struct frame* get_frame(enum palloc_flags flags) {
	return get_multiple_frames(flags, 1);
}

