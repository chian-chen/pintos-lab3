#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "vm/frame.h"


#define MAX_STACK_SIZE (1 << 20) // 1MB
enum page_type {
    PAGE_STACK,
    PAGE_FILE,
    PAGE_CODE,
    PAGE_IN_SWAP
};

/* Supplementary page structure to hold pages' information*/
struct suppPage {
    enum page_type type;

    struct list_elem elem;      /* List element for the page table */
    struct thread *owner_t;          /* Thread that owns the page */
    
    // process: load_segment
    struct file *file;
    off_t offset;
	void * upage;
	uint32_t read_bytes;
	uint32_t zero_bytes;
	bool writable;
    
    bool is_loaded; // true if the page is loaded in memory
    bool safe;      // the page can be evicted or not
};

void suppPage_cleanup_all(struct thread* t);
void suppPage_cleanup_file(struct file* file);

struct suppPage* get_suppPage_by_addr(uint8_t *upage);
bool load_in_memory(struct suppPage* sp);
struct suppPage* get_suppPage_by_frame(struct frame* f);
bool stack_grow(void *addr);






#endif  /* vm/page.h */