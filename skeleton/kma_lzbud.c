/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the SVR4 lazy budy
 *             algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_LZBUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

/************Function Prototypes******************************************/

/************External Declaration*****************************************/

/**************Implementation***********************************************/

#define SIZE_NUM	(16)
#define SIZE_OFFSET	(5)
#define MAX_SIZE	PAGESIZE
#define BITMAP_LEN	(PAGESIZE/(8*(1<<SIZE_OFFSET)))
#define PAGE_INDEX_MASK	(~(PAGESIZE-1))
#define PAGE_BIT_LEN	((PAGESIZE==8192)?(13):((PAGESIZE==4096)?12:11))


// get the page start address of a specified address
inline void *get_page_start(void *addr) {
	return (void*)((unsigned long)addr & ~((unsigned long)(PAGESIZE-1)));
};

// get hte next page start address using a specified address
inline void *get_page_end(void *addr) {
	return (void*)((char*)get_page_start(addr) + PAGESIZE);
}


// the entry point of our own control information
static kma_page_t *first_page = NULL;

// store the information of each page
struct page_item {
	kma_page_t *page;
	unsigned char *bitmap;	// to identify the free block
	struct page_item *prev;
	struct page_item *next;
};
// the page map unit, indexed by the page start address
struct page_map {
	struct page_item *page;
};

// use this structure to specify the requested memory
struct free_block {
	struct free_block *prev;
	struct free_block *next;
};

// all the free block will be stored here
struct block_list {
	int slack;
	struct free_block block;
};

// our control unit for Lazy Buddy allocator
struct bud_ctl {
	int total_alloc;
	int total_free;
	struct block_list free_list[SIZE_NUM];
	int MultiplyDeBruijnBitPosition[32];
	kma_page_t *cur_page;
	int cur_used;
	int max_order;
	struct page_item unused_list;
	struct page_item work_page_list;
	struct page_item ctl_page_list;
	struct page_item page_map_list;		// to store pages used to lookup for page_item
};

inline int get_buddy_index(int idx, int order) {
	return idx ^ (1 << order);
}

inline int get_parent_index(int idx, int order) {
	return idx & ~(1 << order);
}

// set the specified bit
inline void set_bit(unsigned char *bitmap, int idx) {
	bitmap[idx >> 3] |= 1 << (idx & 0x7);
}

// clear the specified 
inline void clear_bit(unsigned char *bitmap, int idx) {
	bitmap[idx >> 3] &= ~(1 << (idx & 0x7));
}

inline int get_bit(unsigned char *bitmap, int idx) {
	return (bitmap[idx >> 3] & (1 << (idx & 0x7))) != 0;
}

// easy to get the control unit 
inline struct bud_ctl *get_bud_ctl() {
	assert(first_page);
	return (struct bud_ctl*)(first_page->ptr);
}

/*
 * list operation functions
 */
inline void list_append(struct page_item *item, struct page_item *header) {
	item->prev = header->prev;
	item->next = header;
	header->prev = item;
	item->prev->next = item;
}

inline void list_insert_head(struct page_item *item, struct page_item *header) {
	item->prev = header;
	item->next = header->next;
	header->next = item;
	item->next->prev = item;
}

inline void list_insert_before(struct page_item *item, struct page_item *target) {
	item->prev = target->prev;
	item->next = target;
	item->prev->next = item;
	item->next->prev = item;
}


inline void list_remove(struct page_item *item) {
	item->prev->next = item->next;
	item->next->prev = item->prev;
}


inline void block_list_append(struct free_block *item, struct free_block *header) {
	item->prev = header->prev;
	item->next = header;
	header->prev = item;
	item->prev->next = item;
}

inline void block_list_insert_head(struct free_block *item, struct free_block *header) {
	item->prev = header;
	item->next = header->next;
	header->next = item;
	item->next->prev = item;
}

inline void block_list_insert_before(struct free_block *item, struct free_block *target) {
	item->prev = target->prev;
	item->next = target;
	item->prev->next = item;
	item->next->prev = item;
}

inline void block_list_remove(struct free_block *item) {
	item->prev->next = item->next;
	item->next->prev = item->prev;
}


extern struct page_item *get_unused_page_item(int need_bitmap);

// add all the list items in a page to the unused list item list
void add_page_for_page_item() {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *cur, *end;
	kma_page_t *page;
	assert(ctl);
	page = get_page();
	cur = (struct page_item*)(page->ptr);
	end = (struct page_item*)get_page_end(cur);
	for(; cur + 1 < end; cur++) {
		cur->bitmap = NULL;
		list_insert_head(cur, &(ctl->unused_list));
	}
	cur = get_unused_page_item(0);
	cur->page = page;
	list_append(cur, &(ctl->ctl_page_list));
}

// a fast function to calculate the list index
inline int get_list_index_by_size(int *table, int sz) {
	int ret = table[(unsigned int)(sz*0x077CB531U)>>27];
	ret -= SIZE_OFFSET;
	return ret <= 0 ? 0 : ret;
}

// initialize the first page
void init_first_page() {
	struct bud_ctl *ctl;
	struct page_item *cur, *end;
	int i;
	int _MultiplyDeBruijnBitPosition[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};
	first_page = get_page();
	memset(first_page->ptr, 0, first_page->size);
	ctl = (struct bud_ctl*)(first_page->ptr);
	ctl->total_alloc = 0;
	ctl->total_free = 0;

	for(i = 0; i < 32; i++) {
		ctl->MultiplyDeBruijnBitPosition[i] = _MultiplyDeBruijnBitPosition[i];
	}

	ctl->cur_page = NULL;
	ctl->cur_used = 0;
	ctl->max_order = get_list_index_by_size(ctl->MultiplyDeBruijnBitPosition, PAGESIZE);
	ctl->unused_list.prev = ctl->unused_list.next = &(ctl->unused_list);
	ctl->ctl_page_list.prev = ctl->ctl_page_list.next = &(ctl->ctl_page_list);
	ctl->work_page_list.prev = ctl->work_page_list.next = &(ctl->work_page_list);
	ctl->page_map_list.prev = ctl->page_map_list.next = &(ctl->page_map_list);
	cur = (struct page_item*)((char*)ctl + sizeof(struct bud_ctl));
	end = (struct page_item*)get_page_end((void*)cur);
	for(; cur + 1 < end; cur++) {
		cur->bitmap = NULL;
		list_append(cur, &(ctl->unused_list));
	}

	for(i = 0; i < SIZE_NUM; i++) {
		ctl->free_list[i].slack = 0;
		ctl->free_list[i].block.next = ctl->free_list[i].block.prev = &(ctl->free_list[i].block);
	}
}

// get a page item from the unused page item list
struct page_item *get_unused_page_item(int need_bitmap) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *node;
	assert(ctl);
	if(ctl->unused_list.prev == &(ctl->unused_list))
		add_page_for_page_item();
	if(need_bitmap)
		node = ctl->unused_list.prev;
	else
		node = ctl->unused_list.next;
	list_remove(node);
	return node;
};

// return a page item after using it
void put_unused_page_item(struct page_item *node, int have_bitmap) {
	struct bud_ctl *ctl = get_bud_ctl();
	assert(node);
	if(have_bitmap)
		list_append(node, &(ctl->unused_list));
	else
		list_insert_head(node, &(ctl->unused_list));
}

// initialize the bitmap, allocate new page if needed
void init_bitmap(struct page_item *item) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *ii;
	assert(item);
	assert(ctl);
	if(item->bitmap)
		goto clear_bit;
	if(!(ctl->cur_page && (ctl->cur_used + BITMAP_LEN <= PAGESIZE))) {
		ii = get_unused_page_item(0);
		ii->page = get_page();
		list_append(ii, &(ctl->ctl_page_list));
		ctl->cur_used = 0;
		ctl->cur_page = ii->page;
	}
	item->bitmap = (unsigned char*)ctl->cur_page->ptr + ctl->cur_used;
	ctl->cur_used += BITMAP_LEN;

	// set all the bit to zero
clear_bit:
	memset(item->bitmap, 0, BITMAP_LEN);
}


// get the index of the block in bitmap
static inline int get_block_index(void *ptr) {
	return (((unsigned long)ptr) >> SIZE_OFFSET) & ((PAGESIZE >> SIZE_OFFSET) - 1);
}

// get the block address with index in bitmap
static inline void *get_block_addr(void *page_start, int idx) {
	return (void*)((char*)page_start + (idx << SIZE_OFFSET));
}

// given an address, get the page index in the page map
static inline int get_page_map_index(void *ptr) {
	return (((unsigned long)ptr)>>PAGE_BIT_LEN)&(PAGESIZE/sizeof(struct page_map)-1);
}

// insert the working page into the page map which could accelerate the page finding
static void insert_page_map(struct page_item *item) {
	int idx;
	struct page_item *cur;
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_map *map_arr;
	int found = 0;
	assert(item);
	idx = get_page_map_index(item->page->ptr);
	cur = ctl->page_map_list.next;
	while(cur != &(ctl->page_map_list)) {
		map_arr = (struct page_map*)cur->page->ptr;	
		if(!map_arr[idx].page) {
			found = 1;
			break;
		}
		cur = cur->next;
	}
	// if we couldn't find a place to insert the page map
	if(!found) {
		cur = get_unused_page_item(0);
		cur->page = get_page();
		memset(cur->page->ptr, 0, cur->page->size);
		list_append(cur, &(ctl->page_map_list));
	}
	map_arr = (struct page_map*)cur->page->ptr;
	map_arr[idx].page = item;
}

// given an address, to find the corresponding page item
static struct page_item *find_page_item_by_addr(void *ptr) {
	struct page_item *cur;
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_map *map_arr;
	int idx;
	assert(ptr);
	cur = ctl->page_map_list.next;
	idx = get_page_map_index(ptr);
	ptr = get_page_start(ptr);
	while(cur != &(ctl->page_map_list)) {
		map_arr = (struct page_map*)cur->page->ptr;
		if(map_arr[idx].page && map_arr[idx].page->page->ptr == ptr)
			return map_arr[idx].page;
		cur = cur->next;
	}
	ptr = get_page_start(ptr);
	return NULL;
}

// allocate the work page for kma_malloc
void alloc_work_page() {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *item;
	struct free_block *block;
	assert(ctl);	
	item = get_unused_page_item(1);
	item->page = get_page();
	init_bitmap(item);
	insert_page_map(item);
	block = (struct free_block*)item->page->ptr;
	block_list_append(block, &(ctl->free_list[ctl->max_order].block));
	list_append(item, &(ctl->work_page_list));
}

// to check if the buddy block is also free now
int check_buddy_free(unsigned char *bitmap, int begin_idx, int order) {
	int i, cidx, bidx;
	unsigned int *arr;
	i = get_buddy_index(begin_idx, order);
	cidx = i >> 3;
	bidx = i & 0x7;
	switch(order) {
		case 0:
		case 1:
		case 2:
			if(bitmap[cidx] & (((1<<(1<<order))-1)<<bidx))
				return 0;
			break;
		case 3:
			// for 8 bits
			if(bitmap[cidx])
				return 0;
			break;
		case 4:
			if(*((unsigned short*)(bitmap+cidx)))
				return 0;
			// for 2 bytes
			break;
		default:
			// for 4 bytes or more
			arr = (unsigned int*)(bitmap+cidx);
			for(i = 0; i < (1<<(order-5)); i++)
				if(arr[i])
					return 0;
			break;
	}
	return 1;
}

// set a block showed as used in bitmap
inline void set_block_used(unsigned char *bitmap, int begin_idx) {
	// set one bit is enough
	set_bit(bitmap, begin_idx);
}

// set a block showed as free in bitmap
inline void set_block_unused(unsigned char *bitmap, int begin_idx) {
	// clear one bit is enough
	clear_bit(bitmap, begin_idx);
}

// get to know if a given block is free
inline int test_block_unused(unsigned char *bitmap, int begin_idx) {
	return get_bit(bitmap, begin_idx) == 0;
}

// free a working page
void free_work_page(struct page_item *item) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *cur;
	struct page_map *map_arr;
	int idx;
	assert(ctl);
	idx = get_page_map_index(item->page->ptr);
	cur = ctl->page_map_list.next;
	// find the page item in page map and remove
	while(cur != &(ctl->page_map_list)) {
		map_arr = (struct page_map*)cur->page->ptr;
		if(map_arr[idx].page && map_arr[idx].page == item) {
			map_arr[idx].page = NULL;
			break;
		}
		cur = cur->next;
	}
	list_remove(item);
	free_page(item->page);
	put_unused_page_item(item, 1);
}

// used by kma_malloc to get a free block
struct free_block *get_free_block(int order) {
	struct bud_ctl *ctl = get_bud_ctl();
	int i, end_order = order;
	struct free_block *block = NULL, *buddy_block;
	struct page_item *item;
	assert(ctl);
	// loop to find the avaailable block in increased order 
	for(i = order; i <= ctl->max_order; i++) {
		block = ctl->free_list[i].block.next;
		if(block != &(ctl->free_list[i].block)) {
			end_order = i;
			block_list_remove(block);
			item = find_page_item_by_addr((void*)block);
			if(!test_block_unused(item->bitmap, get_block_index(block))) {
				// local free
				ctl->free_list[i].slack += 2;
			} else {
				// global free
				ctl->free_list[i].slack += 1;
			}
			break;
		}
		if(i == ctl->max_order) {
			alloc_work_page();
			block = ctl->free_list[i].block.next;
			block_list_remove(block);
			end_order = i;
		}
	}
	assert(block);
	// split the avaailable block if needed
	for(i = end_order - 1; i >= order; i--) {
		buddy_block = (struct free_block*)((char*)block + (1<<(i+SIZE_OFFSET)));
		item = find_page_item_by_addr((void*)buddy_block);
		set_block_used(item->bitmap, get_block_index(buddy_block));
		block_list_append(buddy_block, &(ctl->free_list[i].block));
	}
	item = find_page_item_by_addr((void*)block);
	assert(item);
	set_block_used(item->bitmap, get_block_index(block));
	return block;
}

// return the free block to the right list and do coalesc if possible
void put_free_block(struct free_block *block, int order) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *item;
	int idx;
	assert(ctl);
	item = find_page_item_by_addr((void*)block);
	idx = get_block_index((void*)block);

	switch(ctl->free_list[order].slack) {
		// slack == 0
		case 0:
			set_block_unused(item->bitmap, idx);
			if(order < ctl->max_order) {
				if(check_buddy_free(item->bitmap, idx, order)) {
					block_list_remove((struct free_block*)get_block_addr(item->page->ptr, 
								get_buddy_index(idx, order)));
					idx = get_parent_index(idx, order);
					block = (struct free_block*)get_block_addr(item->page->ptr, idx);
					set_block_used(item->bitmap, idx);
					put_free_block(block, order+1);
				}else {
					block_list_insert_head(block, &(ctl->free_list[order].block));
				}
			} else if(order == ctl->max_order) {
				free_work_page(item);
				break;
			} else {
				assert("impossible branch here" == NULL);
			}

			order++;
			// select on locally free block of order + 1, mark it globally and coalesce if possible
			block = ctl->free_list[order].block.prev;
			while(block != &(ctl->free_list[order].block)) {
				item = find_page_item_by_addr((void*)block);
				idx = get_block_index((void*)block);
				if(test_block_unused(item->bitmap, idx)) {
					block = block->prev;
				} else {
					block_list_remove(block);
					set_block_unused(item->bitmap, idx);
					if(order < ctl->max_order) {
						if(check_buddy_free(item->bitmap, idx, order)) {
							block_list_remove((struct free_block*)get_block_addr(item->page->ptr, 
										get_buddy_index(idx, order)));
							idx = get_parent_index(idx, order);
							block = (struct free_block*)get_block_addr(item->page->ptr, idx);
							set_block_used(item->bitmap, idx);
							put_free_block(block, order+1);
						}else {
							block_list_insert_head(block, &(ctl->free_list[order].block));
						}
					} else if(order == ctl->max_order)
						free_work_page(item);
					else {
						assert("impossible branch here" == NULL);
					}
					break;
				}
			}

			break;
		// slack == 1
		case 1 :
			// set the block as global free and coalesc if possible
			set_block_unused(item->bitmap, idx);
			if(order < ctl->max_order) {
				if(check_buddy_free(item->bitmap, idx, order)) {
					block_list_remove((struct free_block*)get_block_addr(item->page->ptr, 
								get_buddy_index(idx, order)));
					idx = get_parent_index(idx, order);
					block = (struct free_block*)get_block_addr(item->page->ptr, idx);
					set_block_used(item->bitmap, idx);
					put_free_block(block, order+1);
				}else {
					block_list_append(block, &(ctl->free_list[order].block));
				}
			} else if(order == ctl->max_order)
				free_work_page(item);
			else {
				assert("impossible branch here" == NULL);
			}

			ctl->free_list[order].slack = 0;
			break;
		// otherwise slack >= 2
		default:
			// set the freed block as local free
			assert(ctl->free_list[order].slack >= 0);
			block_list_append(block, &(ctl->free_list[order].block));
			ctl->free_list[order].slack -= 2;
			break;
	}

}

// a quick function to round a integer up to its nearest power of 2
inline int __roundup_pow2(int v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}


void*
kma_malloc(kma_size_t size)
{
	struct bud_ctl *ctl;
	int idx;
	void *blk;
	if(size + sizeof(void*) > PAGESIZE)
		return NULL;
	if(!first_page) {
		init_first_page();
	}
	ctl = get_bud_ctl();

	ctl->total_alloc++;

	idx = get_list_index_by_size(ctl->MultiplyDeBruijnBitPosition, __roundup_pow2(size));
	blk = (void*)get_free_block(idx);
	return blk;
}


void
kma_free(void* ptr, kma_size_t size)
{
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *cur;
	int count = 0;
	kma_page_t *page_array[MAXPAGES];
	assert(ctl);

	put_free_block((struct free_block*)ptr, get_list_index_by_size(ctl->MultiplyDeBruijnBitPosition,
				__roundup_pow2(size)));
	ctl->total_free++;

	if(ctl->total_alloc == ctl->total_free) {
		cur = ctl->work_page_list.next;
		while(cur != &(ctl->work_page_list)) {
			assert(cur->page->ptr);
			page_array[count++] = cur->page;
			cur = cur->next;
		}
		cur = ctl->ctl_page_list.next;
		while(cur != &(ctl->ctl_page_list)) {
			assert(cur->page->ptr);
			page_array[count++] = cur->page;
			cur = cur->next;
		}
		cur = ctl->page_map_list.next;
		while(cur != &(ctl->page_map_list)) {
			assert(cur->page->ptr);
			page_array[count++] = cur->page;
			cur = cur->next;
		}
		for(count = count - 1; count >= 0; count--)
			free_page(page_array[count]);
		free_page(first_page);
		first_page = NULL;
	}
}


#endif // KMA_LZBUD
