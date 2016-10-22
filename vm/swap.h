#ifndef SWAP_H 
#define SWAP_H 

#include "devices/block.h"

struct ste {
	struct thread* thread;
	void* page;
	int swap_idx;
};

void swap_table_init(void);

#endif /* vm/swap.h */