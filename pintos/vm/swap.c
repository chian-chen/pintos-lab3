#include <stdint.h>
#include "vm/swap.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/malloc.h"


void swap_init(void);
swap_index_t swap_out_from_memory(struct suppPage* sp);
void swap_to_memory(struct suppPage* sp, uint8_t* kpage);
struct swap_page* get_swap_page_by_index(swap_index_t index);

struct list swap_list;
struct lock swap_lock;
struct block * swap_block;

void swap_init(void){
    list_init(&swap_list);
    lock_init(&swap_lock);
    swap_block = block_get_role(BLOCK_SWAP);
    if (swap_block == NULL) {
        PANIC("Swap block not found");
    }
    
    /* Initialize swap_list: list of swap_page */
    if(lock_held_by_current_thread(&swap_lock) == false)
    {
        lock_acquire(&swap_lock);
    }
    // lock_acquire(&swap_lock);
    int i = 0, total_size = block_size(swap_block) / SECTORS_PER_PAGE;
    for (i = 0; i < total_size; i++) {
        struct swap_page *sp = malloc(sizeof(struct swap_page));
        if (sp == NULL) {
            PANIC("Swap page allocation failed");
        }
        sp->index = i;
        sp->status = SWAP_FREE;
        list_push_back(&swap_list, &sp->elem);
    }
    lock_release(&swap_lock);
    return;
}

struct swap_page* get_swap_page_by_index(swap_index_t index){
    if (index < 0) {
        return NULL;
    }
    struct list_elem *e;
    struct swap_page *sp;
    for (e = list_begin(&swap_list); e != list_end(&swap_list); e = list_next(e)) {
        sp = list_entry(e, struct swap_page, elem);
        if (sp->index == index) {
            return sp;
        }
    }
    return NULL;
}

swap_index_t swap_out_from_memory(struct suppPage* sp){
    uint8_t* upage = sp->upage;
    if(sp->loc == PAGE_NOT_LOAD && sp->pinning == true && swap_block == NULL){ 
        return -1;
    }
    swap_index_t index = -1;
    // printf("[Debug] swap_out_from_memory: %p\n", upage);

    lock_acquire(&swap_lock);

    struct swap_page *spage;
    for(struct list_elem *e = list_begin(&swap_list); e != list_end(&swap_list); e = list_next(e)) {
        spage = list_entry(e, struct swap_page, elem);
        if (spage->status == SWAP_FREE) {
            index = spage->index;
            spage->status = SWAP_USED;
            break;
        }
    }
    if(index != -1){
        for(int i = 0; i < SECTORS_PER_PAGE; i++)
        {
            block_write(swap_block, index * SECTORS_PER_PAGE + i, upage + i * BLOCK_SECTOR_SIZE);
        }
    }

    lock_release(&swap_lock);
    return index;
}


void swap_to_memory(struct suppPage* sp, uint8_t* kpage){
    // uint8_t* upage = sp->upage;
    swap_index_t index = sp->swap_index;

    if(index == -1 && sp->loc != PAGE_IN_SWAP){
        return;
    }

    struct swap_page *spage = get_swap_page_by_index(index);

    if(spage == NULL || spage->status != SWAP_USED){
        return;
    }

    lock_acquire(&swap_lock);
    for (int i = 0; i < SECTORS_PER_PAGE; i++) 
	{
		block_read (swap_block, index * SECTORS_PER_PAGE + i, kpage + i * BLOCK_SECTOR_SIZE);
	}
    spage->status = SWAP_FREE;
    lock_release(&swap_lock);
    return;
}