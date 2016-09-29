#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

int chillPtr(void* ptr){
	if (ptr==NULL || ptr > PHYS_BASE){
		return 0;
	}
	if (pagedir_get_page(thread_current()->pagedir, ptr) == NULL){
		return 0;
	}
	return 1;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int* sp = f->esp;
	int syscall_num = *sp;
	if (!(chillPtr(sp)&&chillPtr(sp+1)&&chillPtr(sp+2)&&chillPtr(sp+3))) {
		exit(-1);
	}
	printf("%04x\t %d\n", sp, syscall_num);

	printf ("system call!\n");

	switch(syscall_num) {
		case SYS_HALT:
			halt();
			break;

		case SYS_EXIT:
			exit(*(sp+1));
			break;

		case SYS_EXEC:
			f->eax = exec((char *)*(sp+1));
			break;

		case SYS_WAIT:
			f->eax=wait(*(pid_t*) (sp+1));
			break;

		case SYS_CREATE:
			f->eax= create((char*) (sp+1), *(unsigned*)(sp+2));
			break;

		case SYS_REMOVE:
			f->eax=remove((char*) (sp+1));
			break;

		case SYS_OPEN:
			f->eax=open((char*) (sp+1));
			break;

		case SYS_FILESIZE:
			f->eax=filesize(*(int*)(sp+1));
			break;

		case SYS_READ:
			f->eax=read(*(int*)(sp+1), *(void**) (sp+2),*(unsigned*)(sp+3));
			break;

		case SYS_WRITE:
			f->eax=write(*(int*)(sp+1), *(void**) (sp+2),*(unsigned*)(sp+3));
			break;

		case SYS_SEEK:
			seek(*(int*)(sp+1),*(unsigned*)(sp+2));
			break;

		case SYS_TELL:
			f->eax=tell(*(int*)(sp+1));
			break;

		case SYS_CLOSE:
			close(*(int*)(sp+1));
			break;
	}
	thread_exit ();
}

void halt() {

}

void exit(int status) {
	// lock_acquire(&l);

	// printf("%s: exit(%d)\n", thread_current()->name, status, thread_current()->tid);

	// struct thread* cur = thread_current();
	// struct child_info* ci = tid_find_child_info(cur->tid);
	// struct thread* pt;

	// ci->status = status;
	// ci->terminated = true;
	// ci->accessed = false;

	// if(cur->children != 0) {
	// 	tell_children_terminated(cur);
	// 	if(cur->parent_terminated == false) {
	// 		pt = cur->parent_thread;
	// 		if(pt->waiting == true && pt->waiting_on_tid == cur->tid) {
	// 			sema_up(&pt->block);
	// 		}
	// 	}
	// }
	// else if(cur->parent_terminated == false) {
	// 	pt = cur->parent_thread;
	// 	if(pt->waiting == true && pt->waiting_on_tid == cur->tid) {
	// 		sema_up(&pt->block);
	// 	}
	// }

	// lock_release(&l);
	// thread_exit();
}

pid_t exec(const char* cmd_line) {

}

int wait(pid_t pid) {

}

bool create(const char* file, unsigned initial_size) {

}

bool remove(const char* file) {

}

int open(const char* file) {

}

int filesize(int fd) {

}

int read(int fd, void* buffer, unsigned size) {

}

int write (int fd, const void *buffer, unsigned size) {

}

void seek (int fd, unsigned position) {

}

unsigned tell (int fd) {

}

void close (int fd) {

}
