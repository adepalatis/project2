#ifndef SWAP_H 
#define SWAP_H 

#include "devices/block.h"
#include "devices/ide.h"
#include <debug.h>
#include "frame.h"

struct ste {
	struct thread* thread;
	void* kpage;
	int swap_ofs; //in number of blocks
};

void swap_table_init(void);
struct ste* get_ste(void);
struct ste* get_ste_for_thread(void* kpage, struct thread* process);
bool load_to_mem(void* kpage, struct thread* process);

#endif /* vm/swap.h */