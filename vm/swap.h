#ifndef SWAP_H 
#define SWAP_H 

#include "devices/block.h"
#include <debug.h>
#include "frame.h"

struct ste {
	struct thread* thread;
	void* kpage;
	struct block* swap_block;
};

void swap_table_init(void);
struct ste* get_ste(void);
struct ste* get_ste_for_thread(void* kpage, struct thread* process);
void load_to_mem(void* kpage, struct thread* process);

#endif /* vm/swap.h */