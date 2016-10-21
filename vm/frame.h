#ifndef FRAME_H 
#define FRAME_H 

#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"

void frame_table_init(void);
void* get_frame(void);
void free_frame(void* u_page);

#endif /* vm/frame.h */