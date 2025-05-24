#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <list.h>
#include <stdint.h>
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "devices/block.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE) // 4KB / 512B = 8
typedef int swap_index_t; // Index of the swap page
struct suppPage;

enum swap_status{
    SWAP_FREE,
    SWAP_USED
};

extern struct list swap_list;
extern struct lock swap_lock;
extern struct block * swap_block;

struct swap_page {
    struct list_elem elem;      // List element
    swap_index_t index;         // Index of the swap page
    enum swap_status status;    // Swap status
};

void swap_init(void);
swap_index_t swap_out_from_memory(struct suppPage* sp);
void swap_to_memory(struct suppPage* sp, uint8_t* kpage);
struct swap_page* get_swap_page_by_index(swap_index_t index);

#endif /* vm/swap.h */