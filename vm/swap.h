#ifndef SWAP_H 
#define SWAP_H 

#include "devices/block.h"

struct ste {
	struct thread* thread;
	void* page;
	struct block* swap_block;
};

void swap_table_init(void);

#endif /* vm/swap.h */