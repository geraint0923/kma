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
#include "utils.h"

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

#define HAVE_HEADER

static kma_page_t *first_page = NULL;


struct page_item {
	kma_page_t *page;
	int start;
	struct page_item *prev;
	struct page_item *next;
};

struct free_block {
	void *next;
};

struct block_list {
	int size;
	struct free_block *next;
};

struct p2fl_ctl {
	int total_alloc;
	int total_free;
	struct block_list free_list[SIZE_NUM];
	struct page_item unused_list;
	struct page_item page_list;
	struct page_item ctl_page_list;
};

struct p2fl_ctl *get_p2fl_ctl() {
	assert(first_page);
	return (struct p2fl_ctl*)(first_page->ptr);
}

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

extern struct page_item *get_unused_page_item();

void add_page_for_page_item() {
	struct p2fl_ctl *ctl = get_p2fl_ctl();
	struct page_item *cur, *end;
	kma_page_t *page;
	assert(ctl);
	page = get_page();
	cur = (struct page_item*)(page->ptr);
	end = (struct page_item*)get_page_end(cur);
	for(; cur + 1 < end; cur++) {
		list_append(cur, &(ctl->unused_list));
	}
	cur = get_unused_page_item();
	cur->page = page;
	list_append(cur, &(ctl->ctl_page_list));
}

void init_first_page() {
	struct p2fl_ctl *ctl;
	struct page_item *cur, *end;
	int i;
	first_page = get_page();
	memset(first_page->ptr, 0, first_page->size);
	ctl = (struct p2fl_ctl*)(first_page->ptr);
	ctl->total_alloc = 0;
	ctl->total_free = 0;

	ctl->unused_list.prev = ctl->unused_list.next = &(ctl->unused_list);
	ctl->page_list.prev = ctl->page_list.next = &(ctl->page_list);
	ctl->ctl_page_list.prev = ctl->ctl_page_list.next = &(ctl->ctl_page_list);
	cur = (struct page_item*)((char*)ctl + sizeof(struct p2fl_ctl));
	end = (struct page_item*)get_page_end((void*)cur);
	for(; cur + 1 < end; cur++) {
		list_append(cur, &(ctl->unused_list));
	}

	for(i = 0; i < SIZE_NUM; i++) {
		ctl->free_list[i].size = (1<< (i+SIZE_OFFSET));
		ctl->free_list[i].next = NULL;
	}
}

struct page_item *get_unused_page_item() {
	struct p2fl_ctl *ctl = get_p2fl_ctl();
	struct page_item *node;
	assert(ctl);
	if(ctl->unused_list.prev == &(ctl->unused_list))
		add_page_for_page_item();
	node = ctl->unused_list.next;
	list_remove(node);
	return node;
};

void put_unused_page_item(struct page_item *node) {
	struct p2fl_ctl *ctl = get_p2fl_ctl();
	assert(node);
	list_insert_head(node, &(ctl->unused_list));
}

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
	struct p2fl_ctl *ctl;
	int sz, idx;
	struct page_item *item;
	kma_page_t *page;
	struct free_block *block;
	int found = 0;
	if(size + sizeof(void*) > PAGESIZE)
		return NULL;
	if(!first_page) {
		init_first_page();
	}
	ctl = get_p2fl_ctl();

#ifdef HAVE_HEADER
	sz = roundup_pow2(size+sizeof(struct free_block));
#else
	sz = roundup_pow2(size);
#endif
	idx = get_list_index_by_size(sz);
	if(ctl->free_list[idx].next == NULL) {
		item = ctl->page_list.next;
		while(item != &(ctl->page_list)) {
			if(item->start + sz < item->page->size) {
				found = 1;
				break;
			}
			item = item->next;
		}
		if(!found) {
			page = get_page();
			item = get_unused_page_item();
			item->page = page;
			item->start = 0;
			list_append(item, &(ctl->page_list));
		}
		block = (struct free_block*)((char*)item->page->ptr + item->start);
		item->start += sz;
		block->next = (void*)ctl->free_list[idx].next;
		ctl->free_list[idx].next = block;	
	}
	block = ctl->free_list[idx].next;
	ctl->free_list[idx].next = block->next;
#ifdef HAVE_HEADER
	block->next = (void*)&(ctl->free_list[idx]);
#endif

	ctl->total_alloc++;
	
#ifdef HAVE_HEADER
	return (void*)(block+1);
#else
	return (void*)block;
#endif
}

void
kma_free(void* ptr, kma_size_t size)
{
	struct p2fl_ctl *ctl = get_p2fl_ctl();
	struct page_item *cur;
	struct free_block *block;
	struct block_list *list;
	int count = 0;
#ifdef HAVE_HEADER
#endif
	kma_page_t *page_array[MAXPAGES];
	assert(ctl);

	block = (struct free_block*)ptr;
#ifdef HAVE_HEADER
	block -= 1;
	list = (struct block_list*)(block->next);
#else
	list = &(ctl->free_list[get_list_index_by_size(roundup_pow2(size))]);
#endif
	block->next = list->next;
	list->next = block;

	
	ctl->total_free++;

	if(ctl->total_alloc == ctl->total_free) {
		cur = ctl->page_list.next;
		while(cur != &(ctl->page_list)) {
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
		for(count = count - 1; count >= 0; count--)
			free_page(page_array[count]);
		free_page(first_page);
		first_page = NULL;
	}
}

#endif // KMA_P2FL
