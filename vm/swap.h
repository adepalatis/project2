#ifndef SWAP_H 
#define SWAP_H 

struct ste {
	struct thread* thread;
	void* page;
	int swap_idx;
}

#endif /* vm/swap.h */