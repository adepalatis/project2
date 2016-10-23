#include "vm/swap.h"

struct ste swap_table[1024];

static struct block_operations swap_ops  = { 
	block_read,
	block_write
};

void swap_table_init(void) {
	for(int k = 0; k < 1024; k++) {
		swap_table[k].thread = NULL;
		swap_table[k].kpage = NULL;
		swap_table[k].swap_block = block_register(NULL, BLOCK_SWAP, NULL, 8, &swap_ops, NULL);	// fill in params
	}
}

struct ste* get_ste(void) {
	for(int k = 0; k < 1024; k++) {
		if(swap_table[k].thread == NULL) {
			return &swap_table[k];
		}
	}
	return NULL;
}

bool load_to_mem(void* kpage, struct thread* process){
	struct ste* swap_entry = get_ste_for_thread(kpage, process);
	if (swap_entry==NULL){
		return false;
	}
	void* page = get_frame();
	for (int idx = 0; idx<8 ; idx++){
		//write all blocks to the newly allocated page from the block
		block_read(swap_entry->swap_block, idx, ((int*)page) + 128*idx);
	}
	pagedir_set_page(process->pagedir, page, kpage, true);
	swap_entry->thread = NULL;
	return true;
}

struct ste* get_ste_for_thread(void* kpage, struct thread* process) {
	for(int k = 0; k < 1024; k++) {
		if(swap_table[k].thread == process && swap_table[k].kpage == kpage) {
			return &swap_table[k];
		}
	}
	// PANIC("Page not found for thread in swap table");
	return NULL;
}
