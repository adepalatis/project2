#ifndef SWAP_H 
#define SWAP_H 

#include "devices/block.h"
#include <debug.h>

struct ste {
	struct thread* thread;
	void* page;
	struct block* swap_block;
};

void swap_table_init(void);
struct ste* get_ste(void);

#endif /* vm/swap.h */