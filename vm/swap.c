#include "vm/swap.h"

struct ste swap_table[1024];

void swap_table_init(void) {
	for(int k = 0; k < 1024; k++) {
		swap_table[k].thread = NULL;
		swap_table[k].page = NULL;
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
