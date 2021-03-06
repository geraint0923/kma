/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the McKusick-Karels
 *              algorithm
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
#ifdef KMA_MCK2
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

#define SIZE_NUM	(20)
#define SIZE_OFFSET	(4)
#define MAX_SIZE	PAGESIZE
#define PAGE_INDEX_MASK	(~(PAGESIZE-1))
#define PAGE_BIT_LEN	((PAGESIZE==8192)?(13):((PAGESIZE==4096)?12:11))


// a fast helper to calculate the nearest power of 2
inline int roundup_pow2(int v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

// get the start address of a page
inline void *get_page_start(void *addr) {
	return (void*)((unsigned long)addr & ~((unsigned long)(PAGESIZE-1)));
};

// get the start address of the next page
inline void *get_page_end(void *addr) {
	return (void*)((char*)get_page_start(addr) + PAGESIZE);
}


// the entry point of the first page
static kma_page_t *first_page = NULL;
struct page_map {
	struct page_item *page;
};

struct page_item {
	kma_page_t *page;
	int order;
	struct page_item *prev;
	struct page_item *next;
};

struct free_block {
	void *next;
};

struct block_list {
	struct free_block *next;
};

// the control unit of this allocator
struct mck2_ctl {
	int total_alloc;
	int total_free;
	struct block_list free_list[SIZE_NUM];
	struct page_item unused_list;
	struct page_item page_list;
	struct page_item page_map_list;
};

inline struct mck2_ctl *get_mck2_ctl() {
	assert(first_page);
	return (struct mck2_ctl*)(first_page->ptr);
}

/*
 * list operations for list item
 */
void list_append(struct page_item *item, struct page_item *header) {
	item->prev = header->prev;
	item->next = header;
	header->prev = item;
	item->prev->next = item;
}

void list_insert_head(struct page_item *item, struct page_item *header) {
	item->prev = header;
	item->next = header->next;
	header->next = item;
	item->next->prev = item;
}

void list_insert_before(struct page_item *item, struct page_item *target) {
	item->prev = target->prev;
	item->next = target;
	item->prev->next = item;
	item->next->prev = item;
}

void list_remove(struct page_item *item) {
	item->prev->next = item->next;
	item->next->prev = item->prev;
}

// to get the page map index by specified pointer or address
static inline int get_page_map_index(void *ptr) {
	return (((unsigned long)ptr)>>PAGE_BIT_LEN)&(PAGESIZE/sizeof(struct page_map)-1);
}

extern struct page_item *get_unused_page_item();

// to insert a page item into the page map
static void insert_page_map(struct page_item *item) {
	int idx;
	struct page_item *cur;
	struct mck2_ctl *ctl = get_mck2_ctl();
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
	if(!found) {
		cur = get_unused_page_item(0);
		cur->page = get_page();
		memset(cur->page->ptr, 0, cur->page->size);
		list_append(cur, &(ctl->page_map_list));
	}
	map_arr = (struct page_map*)cur->page->ptr;
	map_arr[idx].page = item;
}

// find a page item using a specified address
static struct page_item *find_page_item_by_addr(void *ptr) {
	struct page_item *cur;
	struct mck2_ctl *ctl = get_mck2_ctl();
	struct page_map *map_arr;
	int idx;
	assert(ptr);
	cur = ctl->page_map_list.next;
	idx = get_page_map_index(ptr);
	ptr = get_page_start(ptr);
	// traverse all the page map to find the specified page
	while(cur != &(ctl->page_map_list)) {
		map_arr = (struct page_map*)cur->page->ptr;
		if(map_arr[idx].page && map_arr[idx].page->page->ptr == ptr)
			return map_arr[idx].page;
		cur = cur->next;
	}
	ptr = get_page_start(ptr);
	return NULL;
}

// add and initialize all the list items in a new allocated page
void add_page_for_page_item() {
	struct mck2_ctl *ctl = get_mck2_ctl();
	struct page_item *cur, *end;
	kma_page_t *page;
	assert(ctl);
	page = get_page();
	cur = (struct page_item*)(page->ptr);
	end = (struct page_item*)get_page_end(cur);
	// initialize all the list items
	for(; cur + 1 < end; cur++) {
		list_append(cur, &(ctl->unused_list));
	}
	cur = get_unused_page_item();
	cur->page = page;
	list_append(cur, &(ctl->page_list));
}

// add the free blocks in one page to a specified free list
void add_page_for_idx(int idx) {
	struct mck2_ctl *ctl = get_mck2_ctl();
	// set the target size
	int sz = 1 << (idx + SIZE_OFFSET);
	char *cur, *end;
	struct free_block *block;
	kma_page_t *page;
	struct page_item *item;
	assert(ctl);
	page = get_page();
	cur = (char*)(page->ptr);
	end = (char*)get_page_end(cur);
	// add to free list
	for(; cur + sz <= end; cur += sz) {
		block = (struct free_block*)cur;
		block->next = ctl->free_list[idx].next;
		ctl->free_list[idx].next = block;
	}
	item = get_unused_page_item();
	assert(item);
	item->page = page;
	list_append(item, &(ctl->page_list));
	item->order = idx;
	insert_page_map(item);
}

// initialize the first control page
void init_first_page() {
	struct mck2_ctl *ctl;
	struct page_item *cur, *end;
	int i;
	first_page = get_page();
	memset(first_page->ptr, 0, first_page->size);
	ctl = (struct mck2_ctl*)(first_page->ptr);
	ctl->total_alloc = 0;
	ctl->total_free = 0;

	ctl->unused_list.prev = ctl->unused_list.next = &(ctl->unused_list);
	ctl->page_list.prev = ctl->page_list.next = &(ctl->page_list);
	ctl->page_map_list.prev = ctl->page_map_list.next = &(ctl->page_map_list);
	cur = (struct page_item*)((char*)ctl + sizeof(struct mck2_ctl));
	end = (struct page_item*)get_page_end((void*)cur);

	// use the rest room of the first page
	for(; cur + 1 < end; cur++) {
		list_append(cur, &(ctl->unused_list));
	}

	// initialize all the free list
	for(i = 0; i < SIZE_NUM; i++) {
		ctl->free_list[i].next = NULL;
	}
}

// get unused list item
struct page_item *get_unused_page_item() {
	struct mck2_ctl *ctl = get_mck2_ctl();
	struct page_item *node;
	assert(ctl);
	if(ctl->unused_list.prev == &(ctl->unused_list))
		add_page_for_page_item();
	node = ctl->unused_list.next;
	list_remove(node);
	return node;
};

// return the list item to unused list
void put_unused_page_item(struct page_item *node) {
	struct mck2_ctl *ctl = get_mck2_ctl();
	assert(node);
	list_insert_head(node, &(ctl->unused_list));
}


// calculate the log2
int get_list_index_by_size(int sz) {
	int ret = 3;
	sz >>= 3;
	while(sz != 1) {
		sz >>= 1;
		ret++;
	}
	ret -= SIZE_OFFSET;
	return ret <= 0 ? 0 : ret;
}


void*
kma_malloc(kma_size_t size)
{
	struct mck2_ctl *ctl;
	int sz, idx;
	struct free_block *block;
	if(size + sizeof(void*) > PAGESIZE)
		return NULL;
	if(!first_page) {
		init_first_page();
	}
	ctl = get_mck2_ctl();

	sz = roundup_pow2(size);

	idx = get_list_index_by_size(sz);

	// add page if the target free list is empty
	if(ctl->free_list[idx].next == NULL) {
		add_page_for_idx(idx);
	}
	block = ctl->free_list[idx].next;
	assert(block);
	ctl->free_list[idx].next = block->next;
	block->next = (void*)&(ctl->free_list[idx]);

	ctl->total_alloc++;
	
	return (void*)block;
}

void
kma_free(void* ptr, kma_size_t size)
{
	struct mck2_ctl *ctl = get_mck2_ctl();
	struct page_item *cur, *node;
	struct free_block *block;
	struct block_list *list;
	int count = 0;
	kma_page_t *page_array[MAXPAGES];
	assert(ctl);

	block = (struct free_block*)ptr;

	node = find_page_item_by_addr(ptr);
	//list = &(ctl->free_list[get_list_index_by_size(roundup_pow2(size))]);	
	list = &(ctl->free_list[node->order]);

	block->next = list->next;
	list->next = block;

	ctl->total_free++;

	// free all the pages after all the requests have been done
	if(ctl->total_alloc == ctl->total_free) {
		// traverse the control page list
		cur = ctl->page_list.next;
		while(cur != &(ctl->page_list)) {
			assert(cur->page->ptr);
			page_array[count++] = cur->page;
			cur = cur->next;
		}
		// traverse the page map list
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




#endif // KMA_MCK2
