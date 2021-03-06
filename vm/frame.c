#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "frame.h"
#include "devices/block.h"

struct frame {
	struct thread* owner;
	void* u_page;
	bool in_use;

	bool pinned;
};

static struct frame frame_table[367];

void frame_table_init(void) {
	for(int k = 0; k < 367; k++) {
		frame_table[k].u_page = palloc_get_page(PAL_USER | PAL_ZERO);
		frame_table[k].in_use = false;
		frame_table[k].pinned = false;
	}
}


static void evict(struct frame* toEvict){
	printf("EVICTED BITCH\n");
	struct ste* swap_entry = get_ste();
	if (swap_entry==NULL){
		PANIC("No more swap");
	}
	struct block* swap_block = block_get_role (BLOCK_SWAP);
	// printf("BEFORE FOR LOOP\n");
	// block_print_stats();
	for (int idx = swap_entry->swap_ofs; idx<swap_entry->swap_ofs+8 ; idx++){
		// printf("TRYING TO WRITE: %s, %d\n", swap_block->name, swap_block->size);
		block_write(swap_block, idx, ((int*) toEvict->u_page) + 128*idx);
	}
	// printf("PAST WRITE BITCH\n");
	swap_entry->thread = toEvict->owner;
	swap_entry->kpage = pg_round_down(toEvict->u_page);
	// pagedir_clear_page(toEvict->owner->pagedir, toEvict->u_page);
	memset (toEvict->u_page, 0, PGSIZE);
}


// NEED TO IMPLEMENT EVICTION POLICY AND USE IT HERE IF NO PAGES ARE FREE
/* Finds and returns the first unused, unpinned frame */
void* get_frame(void) {
	for(int k = 0; k < 367; k++) {
		if(!frame_table[k].in_use && !frame_table[k].pinned) {
			frame_table[k].owner = thread_current();
			frame_table[k].in_use = true;
			return frame_table[k].u_page;
		}
	}
	// printf("PAST INIT GET FRAME\n");
	void* pd = get_pd();
	for(int k = 0; k < 367; k++) {
		if (!pagedir_is_accessed(pd, frame_table[k].u_page) && !frame_table[k].pinned){
			evict(&frame_table[k]);
			return frame_table[k].u_page;
		}
		else{
			pagedir_set_accessed(pd, frame_table[k].u_page, false);
		}
	}
	for(int k = 0; k < 367; k++) {
		if (!pagedir_is_accessed(pd, frame_table[k].u_page) && !frame_table[k].pinned){
			evict(&frame_table[k]);
			return frame_table[k].u_page;
		}
	}
	PANIC("No more free frames");
	return NULL;
}

void free_frame(void* u_page) {
	for(int k = 0; k < 367; k++) {
		if(frame_table[k].u_page == u_page) {
			memset(u_page, 0, PGSIZE);
			frame_table[k].owner = NULL;
			frame_table[k].in_use = false;
			frame_table[k].pinned = false;
		}
	}
}


