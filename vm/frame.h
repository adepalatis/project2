#ifndef FRAME_H 
#define FRAME_H 

#include "threads/palloc.h"
#include "threads/palloc.c"

struct frame* get_multiple_frames(enum palloc_flags flags, size_t frame_cnt);
struct frame* get_frame(enum palloc_flags flags);

#endif /* vm/frame.h */