#include "vm/swap.h"

struct ste {
	struct thread* thread;
	void* page;
	int swap_idx;
}