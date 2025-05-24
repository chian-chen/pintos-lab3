#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

#include <threads/palloc.h>
#include <threads/synch.h>
#include <list.h>
#include <stdlib.h>
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "userprog/process.h"

struct list frame_table; 
struct lock frame_lock;  

void frame_table_init(void);
uint8_t *frame_get_page(enum palloc_flags flags);
void frame_free_page(uint8_t *kpage);
bool try_evict_frame(void);

void
frame_table_init(void) {
  list_init(&frame_table);
  lock_init(&frame_lock);
}

uint8_t *
frame_get_page(enum palloc_flags flags) {
  uint8_t *kpage = palloc_get_page(flags);
  if (kpage == NULL){
      // printf("[Debug] out of frame\n");
      bool success = try_evict_frame();
      // printf("[Debug] try_evict_frame: %s\n", success ? "success" : "failed");
      if (!success) {
          return NULL;
      }
      kpage = palloc_get_page(flags);
  }

  if (kpage == NULL)
    return NULL;

  struct frame *f = malloc(sizeof *f);
  if (!f) {
    palloc_free_page(kpage);
    return NULL;
  }

  f->kpage = kpage;
  f->t = thread_current();

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

bool try_evict_frame(void) {
  struct thread *cur = thread_current();
  struct frame *victim_f = NULL;
  struct suppPage *victim_sp = NULL;
  struct list_elem *e;

  for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
    victim_f = list_entry(e, struct frame, elem);
    victim_sp = get_suppPage_by_frame(victim_f);
    if (victim_sp == NULL || victim_sp->pinning == true)
    {
      continue;
    }
    else{
      break;
    }
  }

  // printf("[Debug] got a victim? f: %p, sp: %p\n", victim_f, victim_sp);
  if (victim_f == NULL || victim_sp == NULL) {
    return false;
  }

  swap_index_t index = swap_out_from_memory(victim_sp);
  // printf("[Debug] Swap index? index: %d\n", index);

  if (index == -1) {
    return false;
  }

  // update the status
  pagedir_clear_page(victim_f->t->pagedir, victim_sp->upage);
  palloc_free_page(victim_f->kpage);

  lock_acquire(&frame_lock);
  list_remove(&victim_f->elem);
  free(victim_f);
  lock_release(&frame_lock);

  victim_sp->loc = PAGE_IN_SWAP;
  victim_sp->swap_index = index;

  return true;
}