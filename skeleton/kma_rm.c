/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
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
#ifdef KMA_RM
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

kma_page_t *first_page = NULL;

struct page_info {
	kma_page_t *page;
//	int ref_count;
};

struct free_node {
	void *addr;
	int size;
	struct free_node *prev;
	struct free_node *next;
};

struct rm_ctl {
	int total_alloc;
	int total_free;
	struct free_node free_list;
	struct free_node unused_list;
	struct free_node page_list;
};

struct rm_ctl *get_rm_ctl() {
	assert(first_page);
	return (struct rm_ctl*)((char*)first_page->ptr + sizeof(struct page_info));
}

void list_append(struct free_node *item, struct free_node *header) {
	item->prev = header->prev;
	item->next = header;
	header->prev = item;
	item->prev->next = item;
}

void list_insert_head(struct free_node *item, struct free_node *header) {
	item->prev = header;
	item->next = header->next;
	header->next = item;
	item->next->prev = item;
}

void list_insert_before(struct free_node *item, struct free_node *target) {
	item->prev = target->prev;
	item->next = target;
	item->prev->next = item;
	item->next->prev = item;
}

void list_remove(struct free_node *item) {
	item->prev->next = item->next;
	item->next->prev = item->prev;
}

extern struct free_node *get_unused_free_node();

void add_page_for_free_node() {
	struct rm_ctl *ctl = get_rm_ctl();
	struct free_node *cur, *end;
	struct page_info *info;
	kma_page_t *page;
	assert(ctl);
	page = get_page();
	info = (struct page_info*)page->ptr;
	info->page = page;
	cur = (struct free_node*)((char*)info + sizeof(struct page_info));
	end = (struct free_node*)get_page_end(cur);
	for(; cur + 1 < end; cur++) {
		list_append(cur, &(ctl->unused_list));
	}
	cur = get_unused_free_node();
	cur->addr = (void*)page;
	list_append(cur, &(ctl->page_list));
}

void init_first_page() {
	struct page_info *info;
	struct rm_ctl *ctl;
	struct free_node *cur, *end;
	first_page = get_page();
	memset(first_page->ptr, 0, first_page->size);
	info = (struct page_info*)first_page->ptr;
	ctl = (struct rm_ctl*)((char*)first_page->ptr + sizeof(struct page_info));
	info->page = first_page;
	ctl->total_alloc = 0;
	ctl->total_free = 0;
	ctl->free_list.prev = ctl->free_list.next = &(ctl->free_list);
	ctl->unused_list.prev = ctl->unused_list.next = &(ctl->unused_list);
	ctl->page_list.prev = ctl->page_list.next = &(ctl->page_list);
	cur = (struct free_node*)((char*)ctl + sizeof(struct rm_ctl));
	end = (struct free_node*)get_page_end((void*)cur);
	for(; cur + 1 < end; cur++) {
		list_append(cur, &(ctl->unused_list));
	}
}

struct free_node *get_unused_free_node() {
	struct rm_ctl *ctl = get_rm_ctl();
	struct free_node *node;
	assert(ctl);
	if(ctl->unused_list.prev == &(ctl->unused_list))
		add_page_for_free_node();
	node = ctl->unused_list.next;
	list_remove(node);
	return node;
};

void put_unused_free_node(struct free_node *node) {
	struct rm_ctl *ctl = get_rm_ctl();
	assert(node);
	list_insert_head(node, &(ctl->unused_list));
}

void *first_fit(kma_size_t size) {
	struct free_node *cur, *node;
	kma_page_t *page;
	struct page_info *info;
	void *ptr;
	int found = 0;
	struct rm_ctl *ctl = get_rm_ctl();
	assert(ctl);
	cur = ctl->free_list.next;
	while(cur != &(ctl->free_list)) {
		if(cur->size >= size) {
			found = 1;
			break;
		}
		cur = cur->next;
	}
	if(!found) {
		page = get_page();
		info = (struct page_info*)page->ptr;
		info->page = page;
		cur = get_unused_free_node();
		cur->addr = (void*)((char*)page->ptr + sizeof(struct page_info));
		cur->size = page->size - sizeof(struct page_info);
	}
	assert(cur);
	ptr = cur->addr;
	cur->addr = (void*)((char*)cur->addr + size);
	cur->size -= size;
	if(cur->size == 0) {
		if(found)
			list_remove(cur);
		put_unused_free_node(cur);
	} else {
		if(!found) {
			node = ctl->free_list.next;
			while(node != &(ctl->free_list)) {
				if(cur->addr < node->addr) {
					break;
				}
				node = node->next;
			}
			list_insert_before(cur, node);
		}
	}
	ctl->total_alloc++;
	return ptr;
}

void*
kma_malloc(kma_size_t size)
{
	if(size + sizeof(void*) > PAGESIZE)
		return NULL;
	if(!first_page) {
		init_first_page();
	}
	return first_fit(size);
}

void
kma_free(void* ptr, kma_size_t size)
{
	struct rm_ctl *ctl = get_rm_ctl();
	void *base_addr;
	struct free_node *cur, *node;
	struct page_info *info;
	int done = 0, count = 0;
	kma_page_t *page_array[MAXPAGES/2];
	assert(ctl);


	base_addr = get_page_start(ptr);
	cur = ctl->free_list.next;
	while(cur != &(ctl->free_list)) {
		if(base_addr == get_page_start(cur->addr) &&
				(ptr == (void*)((char*)cur->addr + cur->size))) {
			done = 1;
			node = cur->next;
			cur->size += size;
			if(node != &(ctl->free_list) &&
					base_addr == get_page_start(node->addr) &&
					((void*)((char*)cur->addr + (long)cur->size) == node->addr)) {
				cur->size += node->size;
				list_remove(node);
				put_unused_free_node(node);
			}
			break;
		} else if(base_addr == get_page_start(cur->addr) &&
				(cur->addr == (void*)((char*)ptr + size))) {
			cur->addr = ptr;
			cur->size += size;
			done = 1;
			break;
		} else if((void*)((char*)ptr + size) < cur->addr) {
			break;
		}
		cur = cur->next;
	}
	if(!done) {
		node = get_unused_free_node();
		node->addr = ptr;
		node->size = size;
		list_insert_before(node, cur);
	}
	ctl->total_free++;


	if(ctl->total_alloc == ctl->total_free) {
		cur = ctl->free_list.next;
		while(cur != &(ctl->free_list)) {
			info = (struct page_info*)get_page_start(cur->addr);
			assert(info->page->ptr);
			free_page(info->page);
			cur = cur->next;
		}
		cur = ctl->page_list.next;
		while(cur != &(ctl->page_list)) {
			assert(((kma_page_t*)cur->addr)->ptr);
			page_array[count++] = (kma_page_t*)cur->addr;
			cur = cur->next;
		}
		for(count = count - 1; count >= 0; count--)
			free_page(page_array[count]);
		free_page(first_page);
		first_page = NULL;
	}
}

#endif // KMA_RM
