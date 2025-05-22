#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"


extern struct list frame_table; // frame table list, a globla variable
extern struct lock frame_lock;  // Lock for the frame table

//The frame table entry that contains a user page
struct frame {
    struct list_elem elem; // List element
    void *kpage; // Pointer to the page
    struct thread *t; // Thread that owns the page
};


void frame_table_init(void);
uint8_t *frame_get_page(enum palloc_flags flags);
void frame_free_page(uint8_t *kpage);


#endif /* vm/frame.h */