#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "syscall.h"
#include "vm/frame.h"
#include <string.h>

/* Number of page faults processed. */
static long long page_fault_cnt;

#define MAX_STACK_SIZE 2000 * PGSIZE

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}


bool load_from_SPT(struct thread* th, void* fault_addr){
  struct supp_page_table_entry* supp_table = th->spt;

  if(supp_table == NULL) return false;

  void* low_bound = pg_round_down(fault_addr);
  off_t offset = (int) fault_addr - (int) low_bound;

  // check all supplemental page table entries and put the matching one onto memory
  for(int each_spte = 0; each_spte < 40; each_spte ++){
    if(supp_table[each_spte].upage == low_bound){
      // load file from supplemental page to memory
      void* page = get_frame();
      file_read_at (supp_table[each_spte].file, page, 4096, offset);
      return true;
    }
  }

  return false;
}


/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */
  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */

  // printf("IN PAGE FAULT\n");
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  if (!chillFault(fault_addr)){
    f->eax = -1;
    // printf("FAULTED IN PAGE FAULT\n");
    exit(-1);
  }
  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  /* Stack */
  struct thread* current = thread_current();
  if(user && (int)f->esp - (int) fault_addr <= 32 && (int)f->esp - (int) fault_addr > 0) {
    // call function to allocate an additional page
    current->stack_size += PGSIZE;
    current->max_esp = (void*)((int)current->max_esp - PGSIZE);
    void* newFrame = get_frame();
    install_page_public (current->max_esp, newFrame, true);
    return;
  } else if(user && (int)f->esp - (int)fault_addr > 32) {
    exit(-1);
  }

  /* Lazy Loading - look through SPT */
  if(is_user_vaddr(fault_addr)) {
    void* lower_bound = pg_round_down(fault_addr);
    for(int k = 0; k < 40; k++) {
      if(current->spt[k].upage == lower_bound) {
        // int bytes = file_read_at(current->spt[k].file, kframe, current->spt[k].read_bytes, current->spt[k].ofs);
        
        struct file* file = current->spt[k].file;
        off_t ofs = current->spt[k].ofs;
        uint8_t* upage = current->spt[k].upage;
        uint32_t read_bytes = current->spt[k].read_bytes;
        uint32_t zero_bytes = current->spt[k].zero_bytes;
        bool writable = current->spt[k].writable;
        file_seek (file, ofs);
        while (read_bytes > 0 || zero_bytes > 0) 
        {
          printf("IN WHILE LOOP: %d\n", read_bytes);
          /* Calculate how to fill this page.
             We will read PAGE_READ_BYTES bytes from FILE
             and zero the final PAGE_ZERO_BYTES bytes. */
          size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
          size_t page_zero_bytes = PGSIZE - page_read_bytes;

          /* Get a page of memory. */
          uint8_t *kpage = get_frame();

          /* Load this page. */
          int i = file_read (file, kpage, page_read_bytes);
          if (i != (int) page_read_bytes)
            {
              printf("file_read res: %d\n", i);
              printf("file: %04x\n", file);
              printf("kpage: %04x\n", kpage);
              printf("page_read_bytes: %d\n", page_read_bytes);
              free_frame(kpage);
              PANIC("file_read() failed in lazy loading"); 
            }
          memset (kpage + page_read_bytes, 0, page_zero_bytes);

          /* Add the page to the process's address space. */
          if (!install_page_public (upage, kpage, writable)) 
            {
              free_frame(kpage);
              PANIC("install_page() failed in lazy loading"); 
            }

          /* Advance. */
          read_bytes -= page_read_bytes;
          zero_bytes -= page_zero_bytes;
          upage += PGSIZE;
        }
        file_close(file);
      }
    }
  }

  /* Look through Swap */
  if(not_present){
    void* page = pg_round_down(fault_addr);
    // printf("Fault PAGE1: %04x\nRounded PAGE1: %04x\n", fault_addr);
    bool loaded = load_to_mem(page, thread_current());
    if (!loaded){
      // printf("NOT PRESENT AND TRYING TO LOAD FROM DISK");
      loaded = load_from_SPT(thread_current(), fault_addr);
      loaded = false;
    }
    if (loaded){
      return;
    }
    else{
      f->eax = -1;
      PANIC("SHIT NOT RIGHT. TOO LAZY TO WAIT FOR THIS TO FINISH\n");
      exit(-1);
    }
  }

  // if (pagedir_get_page(thread_current()->pagedir, ptr) == NULL)
  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */
  printf ("Page fault at %p: %s error %s page in %s context.\n",
          fault_addr,
          not_present ? "not present" : "rights violation",
          write ? "writing" : "reading",
          user ? "user" : "kernel");
  kill (f);
}

