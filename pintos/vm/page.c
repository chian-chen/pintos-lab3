#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include <stdlib.h>
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <string.h>



void suppPage_cleanup_all(struct thread* t);
void suppPage_cleanup_file(struct file* file);

struct suppPage* get_suppPage_by_addr(uint8_t *upage);
bool load_in_memory(struct suppPage* sp);
struct suppPage* get_suppPage_by_frame(struct frame* f);
bool stack_grow(void *addr);



void suppPage_cleanup_all(struct thread* t) {
    struct list_elem *e, *next;
    struct suppPage *sp;
    lock_acquire(&t->supp_page_lock);
    for (e = list_begin(&t->supp_page_table); e != list_end(&t->supp_page_table);) {
        sp = list_entry(e, struct suppPage, elem);
        next = list_next (e);
        list_remove(&sp->elem);
        free(sp);
        e = next;
    }
    lock_release(&t->supp_page_lock);
    return;
}

void suppPage_cleanup_file(struct file* file) {
    struct list_elem *e;
    struct suppPage *sp;
    struct thread* t = thread_current();

    for (e = list_begin(&t->supp_page_table); e != list_end(&t->supp_page_table); e = list_next(e)) {
        sp = list_entry(e, struct suppPage, elem);
        if (sp->file == file) {
            lock_acquire(&t->supp_page_lock);
            list_remove(&sp->elem);
            lock_release(&t->supp_page_lock);
            free(sp);
        }
    }
}

struct suppPage* get_suppPage_by_addr(uint8_t *upage) {
    struct list_elem *e;
    struct suppPage *sp;
    // printf("try get_suppPage_by_addr: %p\n", upage);
    upage = pg_round_down(upage);
    for (e = list_begin(&thread_current()->supp_page_table); e != list_end(&thread_current()->supp_page_table); e = list_next(e)) {
        sp = list_entry(e, struct suppPage, elem);
        if (sp->upage == upage) {
            // printf("found: %p\n", sp->upage);
            return sp;
        }
    }
    // printf("not found: %p\n", upage);
    return NULL;
}

bool load_in_memory(struct suppPage* sp) {
    sp->pinning = true;

    enum palloc_flags flags = PAL_USER | PAL_ZERO;
    uint8_t * kpage = frame_get_page(flags);

    if (kpage == NULL) {
		return false;
	}

    if(sp->loc == PAGE_IN_SWAP) 
    {
        swap_to_memory(sp, kpage);
    }
    else
    {
        if(sp->file != NULL) {
            file_seek(sp->file, sp->offset);
            if (file_read (sp->file, kpage, sp->read_bytes) != (int) sp->read_bytes)
            {
                frame_free_page (kpage);
                return false; 
            }
            memset (kpage + sp->read_bytes, 0, sp->zero_bytes);
        }
    }

    if (!install_page (sp->upage, kpage, sp->writable)) 
	{
	    frame_free_page (kpage);
	    return false; 
	}

    sp->loc = PAGE_IN_MEMORY;
    sp->pinning = false;
    return true;
}

struct suppPage* get_suppPage_by_frame(struct frame* f){
    struct thread* t = f->t;
    struct list_elem *e;
    struct suppPage *sp;
    
    for(e = list_begin(&t->supp_page_table); e != list_end(&t->supp_page_table); e = list_next(e)){
        sp = list_entry(e, struct suppPage, elem);
        uint32_t * kpage = pagedir_get_page(t->pagedir, sp->upage);
		if (kpage == f->kpage)
		{
			return sp;
		}
    }
    return NULL;
}

bool stack_grow(void *addr){
    if ((size_t) (PHYS_BASE - pg_round_down(addr)) > MAX_STACK_SIZE) {
        return false;
    }
    struct suppPage *sp = malloc(sizeof(struct suppPage));
    if(sp == NULL) {
        return false;
    }
    
    uint8_t * kpage = frame_get_page(PAL_USER);
    if (kpage == NULL) {
        free(sp);
        return false;
    }

    sp->upage = pg_round_down(addr);
    sp->loc = PAGE_IN_MEMORY;
    sp->writable = true;
    sp->owner_t = thread_current();
    sp->type = PAGE_STACK;
    sp->pinning = false;
    sp->file = NULL;
    sp->swap_index = -1;

    bool install_success = install_page(sp->upage, kpage, sp->writable);
    if (install_success == false) 
    {
        free(sp);
        frame_free_page(kpage);
        return false;
    }

    lock_acquire(&thread_current()->supp_page_lock);
    list_push_back(&thread_current()->supp_page_table, &sp->elem);
    lock_release(&thread_current()->supp_page_lock);

    return install_success;
}