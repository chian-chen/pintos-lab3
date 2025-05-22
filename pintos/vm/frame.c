#include "vm/frame.h"
#include <threads/palloc.h>
#include <threads/synch.h>
#include <list.h>
#include <stdlib.h>
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include <stdlib.h>
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <string.h>

struct list frame_table; 
struct lock frame_lock;  

void
frame_table_init(void) {
  list_init(&frame_table);
  lock_init(&frame_lock);
}

uint8_t *
frame_get_page(enum palloc_flags flags) {
  uint8_t *kpage = palloc_get_page(flags);
  if (kpage == NULL)
    return NULL;

  struct frame *f = malloc(sizeof *f);
  if (!f) {
    palloc_free_page(kpage);
    return NULL;
  }

  f->kpage = kpage;
  f->t     = thread_current();
  lock_acquire(&frame_lock);
  list_push_back(&frame_table, &f->elem);
  lock_release(&frame_lock);

  return kpage;
}

void
frame_free_page(uint8_t *kpage) {
  lock_acquire(&frame_lock);
  for (struct list_elem *e = list_begin(&frame_table);
       e != list_end(&frame_table); e = list_next(e)) {
    struct frame *f = list_entry(e, struct frame, elem);
    if (f->kpage == kpage) {
      list_remove(&f->elem);
      free(f);
      break;
    }
  }
  lock_release(&frame_lock);
  palloc_free_page(kpage);
}
