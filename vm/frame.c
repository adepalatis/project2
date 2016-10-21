#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "frame.h"

struct frame {
	struct thread* owner;
	void* u_page;
	bool in_use;
	bool pinned;
};

static struct frame* frame_table[367];

void frame_table_init(void) {
	for(int k = 0; k < 367; k++) {
		frame_table[k]->u_page = palloc_get_page(PAL_USER);
		frame_table[k]->in_use = false;
		frame_table[k]->pinned = false;
	}
}

// NEED TO IMPLEMENT EVICTION POLICY AND USE IT HERE IF NO PAGES ARE FREE
/* Finds and returns the first unused, unpinned frame */
void* get_frame(void) {
	for(int k = 0; k < 367; k++) {
		if(!frame_table[k]->in_use && !frame_table[k]->pinned) {
			frame_table[k]->owner = thread_current();
			frame_table[k]->in_use = true;
			return frame_table[k]->u_page;
		}
	}
	PANIC("No more free frames");
	return NULL;
}

void free_frame(void* u_page) {
	for(int k = 0; k < 367; k++) {
		if(frame_table[k]->u_page == u_page) {
			memset(u_page, 0, PGSIZE);
			frame_table[k]->in_use = false;
			frame_table[k]->pinned = false;
		}
	}
}

