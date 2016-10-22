#include "vm/swap.h"

struct ste swap_table[1024];

void swap_table_init(void) {
	for(int k = 0; k < 1024; k++) {
		swap_table[k].thread = NULL;
		swap_table[k].kpage = NULL;
		// swap_table[k].swap_block = block_register(NULL, BLOCK_SWAP, NULL, 8, );	// fill in params
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

struct ste* get_ste_for_thread(void* kpage, struct thread* process) {
	for(int k = 0; k < 1024; k++) {
		if(swap_table[k].thread == process && swap_table[k].kpage == kpage) {
			return &swap_table[k];
		}
	}
	PANIC("Page not found for thread in swap table");
	return NULL;
}
