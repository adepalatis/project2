#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

struct lock l;

static void syscall_handler (struct intr_frame *);
void close_and_remove_file(int fd);

void
syscall_init (void) 
{
	lock_init(&l);
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
	
}

void halt() {
	shutdown_power_off();
}

void exit(int status) {
	printf("EXITING****************************\n");
	struct thread* cur = thread_current();
	cur->exitCode = status;
	cur->called_exit = true;
	thread_exit();
	printf("PAST EXIT\n");
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
	printf("EXECING NOW*********\n");
	if(!chillPtr(cmd_line)) {
		// deal with naughty pointers 
		return -1;
	}

	pid_t pid = process_execute(cmd_line);

	if(pid == TID_ERROR) {

	}
	else{
		struct thread* thisThread = thread_current();
		struct thread* child = in_child_processes(&(thisThread->children), pid);
		sema_init(&(child->waitSema), 0);
		child->parent = thread_current();
		struct list childList;
		list_init(&childList);
		thread_current()->children = childList;
		list_push_front(&childList, &(child->cochildren));
	}

	//sema_down(&thread_current()->order);

	/* Make sure file loaded successfully */

	return pid;

}

int wait(pid_t pid) {
	printf("IN WAIT\n");
	struct thread* thisThread = thread_current();
	if (in_child_processes(&(thisThread->children), pid)==NULL){
		return -1;
	}
	struct thread* dead;
	if (in_all_threads(pid)!=NULL){
		int toReturn = process_wait(pid);
		return toReturn;
	}
	else if (dead = in_grave(pid) !=NULL) {
		return dead->exitCode;
	}
	else{
		return -1;
	}
}

bool create(const char* file, unsigned initial_size) {
	lock_acquire(&l);

	if(file == NULL || !chillPtr(file)) {
		lock_release(&l);
		exit(-1);
	}

	bool success = filesys_create(file, initial_size);

	lock_release(&l);
	return success;
}

bool remove(const char* file) {
	lock_acquire(&l);

	if(!chillPtr(file)) {
		lock_release(&l);
		exit(-1);
	}

	bool success = filesys_remove(file);

	lock_release(&l);
	return success;
}

int open(const char* file) {
	lock_acquire(&l);

	if(!chillPtr(file)) {
		lock_release(&l);
		exit(-1);
	}	

	struct file* f = filesys_open(file);

	if(!strcmp(thread_current()->name, file)) {
		file_deny_write(f);
	}

	else if(f == NULL) {
		lock_release(&l);
		return -1;	// ??? exit??
	}

	int fd = add_file(f);
	lock_release(&l);
	return fd;
}

int filesize(int fd) {

}

int read(int fd, void* buffer, unsigned size) {

}

int write (int fd, const void *buffer, unsigned size) {
	lock_acquire(&l);

	if(!chillPtr(buffer)) {
		lock_release(&l);
		exit(-1);
	}

	if(fd == 0) {
		lock_release(&l);
		return 0;
	}
	else if (fd == 1) {
		putbuf(buffer, size);
		lock_release(&l);
		return size;
	}
	else {
		struct file* f = get_file(fd);

		if(f == NULL || f->deny_write) {
			lock_release(&l);
			return 0;
		}

		off_t bytes_written = file_write(f, buffer, size);

		lock_release(&l);
		return bytes_written;
	}
}

void seek (int fd, unsigned position) {

}

unsigned tell (int fd) {

}

void close (int fd) {
	sema_down(&(thread_current()->waitSema));
	close_and_remove_file(fd);
	sema_up(&(thread_current()->waitSema));
}

int add_file(struct file* f) {
	// sema_down(&file_lock);
	struct thread* current = thread_current();
	struct list* open_file_list = &current->open_file_list;

	f->fd = current->fd;
	current->fd++;

	list_push_back(open_file_list, &f->file_elem);

	// sema_up(&file_lock);
	return f->fd;
}

void close_and_remove_file(int fd) {
	// sema_down(&file_lock);
	struct list_elem* e;
	struct thread* current = thread_current();
	struct list* open_file_list = &current->open_file_list;

	for(e = list_begin(open_file_list); e != list_end(open_file_list); e = list_next(e)) {
		struct file* f = list_entry(e, struct file, file_elem);

		if(f->fd == fd) {
			list_remove(&f->file_elem);
			file_close(f);
			break;
		}
	}
	// sema_up(&file_lock);
}
