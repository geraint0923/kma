/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
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
#ifdef KMA_P2FL
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define SIZE_NUM	(13)
#define SIZE_OFFSET	(5)

struct free_block {
	kma_page_t *page;
	int ref_count;
	struct free_block *prev;
	struct free_block *next;
};

struct p2fl_ctl {
	int total_alloc;
	int total_free;
	struct free_block free_list[SIZE_NUM];
};

/*
 * list manipulation functions
 */
inline void list_append(struct free_block *item, struct free_block *header) {
	item->prev = header->prev;
	item->next = header;
	header->prev = item;
	item->prev->next = item;
}

// get the start address of the specified page
inline void *get_page_start(void *addr) {
	return (void*)((unsigned long)addr & ~((unsigned long)(PAGESIZE-1)));
};
// get the start address of the next page
inline void *get_page_end(void *addr) {
	return (void*)((char*)get_page_start(addr) + PAGESIZE);
}



inline void list_insert_head(struct free_block *item, struct free_block *header) {
	item->prev = header;
	item->next = header->next;
	header->next = item;
	item->next->prev = item;
}

inline void list_insert_before(struct free_block *item, struct free_block *target) {
	item->prev = target->prev;
	item->next = target;
	item->prev->next = item;
	item->next->prev = item;
}

inline void list_remove(struct free_block *item) {
	item->prev->next = item->next;
	item->next->prev = item->prev;
}

inline int get_list_index_by_size(int sz) {
	if(sz <= 32)
		return 0;
	else if(sz <= 64)
		return 1;
	else if(sz <= 128)
		return 2;
	else if(sz <= 256)
		return 3;
	else if(sz <= 512)
		return 4;
	else if(sz <= 1024)
		return 5;
	else if(sz <= 2048)
		return 6;
	else if(sz <= 4096)
		return 7;
	else
		return 8;
}

static kma_page_t *first_page = NULL;

// a helper to get the control unit
inline struct p2fl_ctl *get_p2fl_ctl() {
	//assert(first_page);
	return (struct p2fl_ctl*)(first_page->ptr);
}

void*
kma_malloc(kma_size_t size)
{
	struct p2fl_ctl *ctl;
	int i, order, len;
	struct free_block *block, *cur, *end;
	kma_page_t *pp;
	if(unlikely(!first_page)) {
		first_page = get_page();
		ctl = get_p2fl_ctl();
		ctl->total_alloc = 0;
		ctl->total_free = 0;
		for(i = 0; i < SIZE_NUM; i++) {
			ctl->free_list[i].prev = ctl->free_list[i].next = &(ctl->free_list[i]);
		}
	}
	ctl = get_p2fl_ctl();
	ctl->total_alloc++;

	order = get_list_index_by_size(size + sizeof(struct free_block));

	cur = ctl->free_list[order].next;
	if(cur == &(ctl->free_list[order])) {
		pp = get_page();
		cur = (struct free_block*)pp->ptr;
		cur->ref_count = 0;
		end = (struct free_block*)(pp->ptr + PAGESIZE);
		len = 1 << (5 + order);
		while(cur < end) {
			cur->page = pp;
			list_insert_head(cur, &(ctl->free_list[order]));
			cur = (struct free_block*)((void*)cur + len);
		}
		cur = ctl->free_list[order].next;
	}
	list_remove(cur);
	cur->prev = &(ctl->free_list[order]);
	cur->next = (struct free_block*)((long)(1 << (5 + order)));
	block = (struct free_block*)get_page_start(cur);
	block->ref_count++;
	return (void*)cur + sizeof(struct free_block);
	
}

void
kma_free(void* ptr, kma_size_t size)
{
	struct p2fl_ctl *ctl;
	struct free_block *block, *cur, *end;
	int len;

	ctl = get_p2fl_ctl();
	ctl->total_free++;

	block = (struct free_block*)get_page_start(ptr);
	cur = (struct free_block*)(ptr - sizeof(struct free_block));
	len = (long)cur->next;
	list_insert_head(cur, cur->prev);

	block->ref_count -= 1;
	if(block->ref_count == 0) {
		cur = block;
		end = (struct free_block*)((void*)block + PAGESIZE);
		while(cur < end) {
			list_remove(cur);
			cur = (struct free_block*)((void*)cur + len);
		}
		free_page(block->page);
	}

	if(unlikely(ctl->total_free == ctl->total_alloc)) {
		free_page(first_page);
		first_page = NULL;
	}
}

#endif // KMA_P2FL
