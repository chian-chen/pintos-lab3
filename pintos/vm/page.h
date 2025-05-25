#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "vm/swap.h"
#include <list.h>
#include <stdint.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "vm/frame.h"



#define MAX_STACK_SIZE (1 << 23) // 8MB
enum page_type {
    PAGE_STACK,
    PAGE_FILE,
    PAGE_CODE
};

enum page_loc {
    PAGE_IN_MEMORY, // Page is in memory
    PAGE_IN_SWAP,   // Page is in swap
    PAGE_NOT_LOAD  // Page is not loaded
};

/* Supplementary page structure to hold pages' information*/
struct suppPage {
    enum page_type type;
    enum page_loc loc; // Location of the page, in memory or in swap

    struct list_elem elem;      /* List element for the page table */
    struct thread *owner_t;          /* Thread that owns the page */
    
    // process: load_segment
    struct file *file;
    off_t offset;
	void * upage;
	uint32_t read_bytes;
	uint32_t zero_bytes;
	bool writable;
    
    bool pinning;      // the page can be evicted or not

    // swap setting
    swap_index_t swap_index; // index of the swap page, we can use it to recover the page
};


struct frame;

void suppPage_cleanup_all(struct thread* t);
struct suppPage* get_suppPage_by_addr(uint8_t *upage);
bool load_in_memory(struct suppPage* sp);
struct suppPage* get_suppPage_by_frame(struct frame* f);
bool stack_grow(void *addr);



#endif  /* vm/page.h */