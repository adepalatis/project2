#include "vm/swap.h"

struct ste swap_table[1024];

void swap_table_init(void) {
	for(int k = 0; k < 1024; k++) {
		swap_table[k].thread = NULL;
		swap_table[k].kpage = NULL;
		swap_table[k].swap_ofs = 8*k; //in number of blocks
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
	// printf("KPAGE: %04x\nTHREAD: %04x\n", kpage, process);
	struct ste* swap_entry = get_ste_for_thread(kpage, process);

	if (swap_entry==NULL){
		return false;
	}
	struct block* swap_block = block_get_role (BLOCK_SWAP);
	void* page = get_frame();
	for (int idx = swap_entry->swap_ofs; idx<swap_entry->swap_ofs+8 ; idx++){
		//write all blocks to the newly allocated page from the block
		block_read(swap_block, idx, ((int*) kpage) + 128*idx);
	}
	pagedir_set_page(process->pagedir, page, kpage, true);
	swap_entry->thread = NULL;
	return true;
}

struct ste* get_ste_for_thread(void* kpage, struct thread* process) {
	for(int k = 0; k < 1024; k++) {
		if (swap_table[k].thread != NULL){
			printf("SWAP ADDR: %04x\nCHECK ADDR: %04x\n", swap_table[k].kpage, kpage);
		}
		if(swap_table[k].thread == process && swap_table[k].kpage == kpage) {
			return &swap_table[k];
		}
	}
	// PANIC("Page not found for thread in swap table");
	return NULL;
}
