/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
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
#ifdef KMA_BUD
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


#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define OFFSET(Type, member) ((size_t)&(((Type*)0)->member))

#define SIZE_OFFSET	(6)
#define MAX_SIZE	PAGESIZE
//#define BITMAP_LEN	(PAGESIZE/(8*(1<<SIZE_OFFSET)))
#define BITMAP_LEN	(32)
#define PAGE_INDEX_MASK	(~(PAGESIZE-1))
#define PAGE_BIT_LEN	((PAGESIZE==8192)?(13):((PAGESIZE==4096)?12:11))
#define SIZE_NUM	(PAGE_BIT_LEN+1-SIZE_OFFSET)


inline void *get_page_start(void *addr) {
	return (void*)((unsigned long)addr & ~((unsigned long)(PAGESIZE-1)));
};

inline void *get_page_end(void *addr) {
	return (void*)((char*)get_page_start(addr) + PAGESIZE);
}


int get_set_bit_num(unsigned int i)
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}



static kma_page_t *first_page = NULL;
struct page_item {
	kma_page_t *page;
	unsigned char *bitmap;
	struct page_item *prev;
	struct page_item *next;
//char padding[16];
};
struct page_map {
	struct page_item *page;
};
struct free_block {
	char order;
	char padding[3];
	struct free_block *prev;
	struct free_block *next;
};
struct block_list {
	struct free_block block;
};
struct bud_ctl {
	int total_alloc;
	int total_free;
	struct block_list free_list[SIZE_NUM];
	int MultiplyDeBruijnBitPosition[32];
	kma_page_t *cur_page;
	int cur_used;
	int max_order;
	struct page_item unused_list;
	struct page_item bitmap_list;
	struct page_item work_page_list;
	struct page_item ctl_page_list;
	struct page_item page_map_list;
};

static void insert_page_map(struct page_item *item);

inline int get_buddy_index(int idx, int order) {
	return idx ^ (1 << order);
}

inline int get_parent_index(int idx, int order) {
	return idx & ~(1 << order);
}

inline void set_bit(unsigned char *bitmap, int idx) {
//	bitmap[idx >> 3] |= 1 << (idx - ((idx >> 3) << 3));
	bitmap[idx >> 3] |= 1 << (idx & 0x7);
}

inline void clear_bit(unsigned char *bitmap, int idx) {
//	bitmap[idx >> 3] &= ~(1 << (idx - ((idx >> 3) << 3)));
	bitmap[idx >> 3] &= ~(1 << (idx & 0x7));
}

inline int get_bit(unsigned char *bitmap, int idx) {
//	return (bitmap[idx >> 3] & (1 << (idx - ((idx >> 3) << 3)))) != 0;
	return (bitmap[idx >> 3] & (1 << (idx & 0x7))) != 0;
}

inline struct bud_ctl *get_bud_ctl() {
	//assert(first_page);
	return (struct bud_ctl*)(first_page->ptr);
}

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

void add_page_for_page_item() {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *cur, *end;
	kma_page_t *page;
	//assert(ctl);
	page = get_page();
	cur = (struct page_item*)(page->ptr);
	end = (struct page_item*)get_page_end(cur);
	for(; cur + 1 <= end; cur++) {
		cur->bitmap = NULL;
		list_insert_head(cur, &(ctl->unused_list));
	}
	cur = ctl->unused_list.next;
	list_remove(cur);
	cur->page = page;
	cur->bitmap = NULL;
	cur->bitmap++;
	insert_page_map(cur);
	list_append(cur, &(ctl->ctl_page_list));
}

void add_page_for_bitmap() {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *node;
	unsigned char *cur, *end;
	kma_page_t *page;
	//assert(ctl);
	page = get_page();
	cur = (unsigned char*)(page->ptr);
	end = (unsigned char*)get_page_end(cur);
	for(; cur + BITMAP_LEN <= end; cur += BITMAP_LEN) {
		node = (struct page_item*)cur;
		list_insert_head(node, &(ctl->bitmap_list));
	}
	node = get_unused_page_item(0);
	node->page = page;
	node->bitmap = NULL;
	insert_page_map(node);
	list_append(node, &(ctl->ctl_page_list));
}


inline int get_list_index_by_size(int *table, int sz) {
	/*
	int ret = 3;
	sz >>= 3;
	while(sz != 1) {
		sz >>= 1;
		ret++;
	}
	ret -= SIZE_OFFSET;
	return ret <= 0 ? 0 : ret;
	*/
	int ret = table[(unsigned int)(sz*0x077CB531U)>>27];
	ret -= SIZE_OFFSET;
	return ret <= 0 ? 0 : ret;
}

void init_first_page() {
	struct bud_ctl *ctl;
	struct page_item *fp, *rsv, *cur, *end;
//	char *st, *ed;
	int i, count;
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

	ctl->max_order = get_list_index_by_size(ctl->MultiplyDeBruijnBitPosition, PAGESIZE);
	ctl->unused_list.prev = ctl->unused_list.next = &(ctl->unused_list);
	ctl->bitmap_list.prev = ctl->bitmap_list.next = &(ctl->bitmap_list);
	ctl->ctl_page_list.prev = ctl->ctl_page_list.next = &(ctl->ctl_page_list);
	ctl->work_page_list.prev = ctl->work_page_list.next = &(ctl->work_page_list);
	ctl->page_map_list.prev = ctl->page_map_list.next = &(ctl->page_map_list);
	cur = (struct page_item*)((char*)ctl + sizeof(struct bud_ctl));
	cur->bitmap = NULL;
	cur->bitmap += 999999;
	//list_append(cur, &(ctl->ctl_page_list));
	cur->page = first_page;
	fp = cur;
	cur++;
	rsv = cur;
	cur++;
	end = (struct page_item*)get_page_end((void*)cur);
	count = ((unsigned long)((char*)end-(char*)cur)) / (1 * sizeof(struct page_item));
	/*
	for(; cur + 1 < end; cur++) {
		cur->bitmap = NULL;
		list_append(cur, &(ctl->unused_list));
	}
	*/
	while(count > 0) {
		list_append(cur, &(ctl->unused_list));
		cur++;
		count--;
	}
/*
	st = (char*)cur;
	ed = (char*)end;
	for(; st + BITMAP_LEN <= ed; st += BITMAP_LEN) {
		list_append((struct page_item*)st, &(ctl->bitmap_list));
	}
*/

	for(i = 0; i < SIZE_NUM; i++) {
		ctl->free_list[i].block.next = ctl->free_list[i].block.prev = &(ctl->free_list[i].block);
	}
	rsv->page = get_page();
	memset(rsv->page->ptr, 0, PAGESIZE);
	list_append(rsv, &(ctl->page_map_list));
	insert_page_map(fp);
}

unsigned char *get_bitmap();
void put_bitmap(unsigned char*);
static struct page_item *find_page_item_by_addr(void*);
static void remove_page_map_by_addr(void*);

inline struct page_item *get_unused_page_item(int need_bitmap) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *node, *tp;
//	unsigned char *bmp;
	//assert(ctl);
	if(unlikely(ctl->unused_list.prev == &(ctl->unused_list)))
		add_page_for_page_item();
	if(need_bitmap) {
//		bmp = get_bitmap();
		node = ctl->unused_list.prev;
//		node->bitmap = bmp;
	} else
		node = ctl->unused_list.next;
	list_remove(node);
	tp = find_page_item_by_addr((void*)node);
	tp->bitmap++;
	return node;
};

void put_unused_page_item(struct page_item *node, int have_bitmap) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *tp, *cur, *end;
	//assert(node);
	if(have_bitmap) {
	//	put_bitmap((unsigned char*)node->bitmap);
	}
	if(first_page->ptr == get_page_start((void*)node))
		list_insert_head(node, &(ctl->unused_list));
	else
		list_append(node, &(ctl->unused_list));
	tp = find_page_item_by_addr((void*)node);
	tp->bitmap--;
	if(unlikely(tp->bitmap == (unsigned char*)0x1)) {
		cur = (struct page_item*)tp->page->ptr;
		end = (struct page_item*)((char*)cur + PAGESIZE);
		for(; cur + 1 <= end; cur++){
			list_remove(cur);
		}
		remove_page_map_by_addr(tp->page->ptr);
//		list_remove(tp);
		free_page(tp->page);
		//put_unused_page_item(tp, 0);
	}
}

inline unsigned char *get_bitmap() {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *node, *tp;
	//assert(ctl);
	if(unlikely(ctl->bitmap_list.prev == &(ctl->bitmap_list)))
		add_page_for_bitmap();
	node = ctl->bitmap_list.next;
	list_remove(node);
	tp = find_page_item_by_addr((void*)node);
	tp->bitmap++;
	memset((unsigned char*)node, 0, BITMAP_LEN);
	return (unsigned char*)node;
}

inline void put_bitmap(unsigned char *bmp) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *tp;
	unsigned char *cur, *end;
	//assert(ctl);
	if(first_page->ptr == get_page_start((void*)bmp))
		list_insert_head((struct page_item*)bmp, &(ctl->bitmap_list));
	else
		list_append((struct page_item*)bmp, &(ctl->bitmap_list));
	tp = find_page_item_by_addr((void*)bmp);
	tp->bitmap--;
	if(unlikely(!(tp->bitmap))) {
		cur = tp->page->ptr;
		end = cur + PAGESIZE;
		for(; cur + BITMAP_LEN <= end; cur += BITMAP_LEN) {
			list_remove((struct page_item*)cur);
		}
		remove_page_map_by_addr(tp->page->ptr);
		list_remove(tp);
		free_page(tp->page);
		put_unused_page_item(tp, 0);
	}
}

static inline int get_block_index(void *ptr) {
	return (((unsigned long)ptr) >> SIZE_OFFSET) & ((PAGESIZE >> SIZE_OFFSET) - 1);
}

static inline void *get_block_addr(void *page_start, int idx) {
	return (void*)((char*)page_start + (idx << SIZE_OFFSET));
}

static inline int get_page_map_index(void *ptr) {
	return (((unsigned long)ptr)>>PAGE_BIT_LEN)&(PAGESIZE/sizeof(struct page_map)-1);
}

inline static void insert_page_map(struct page_item *item) {
	int idx;
	struct page_item *cur;
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_map *map_arr;
	int found = 0;
	//assert(item);
	idx = get_page_map_index(item->page->ptr);
	cur = ctl->page_map_list.next;
	while(likely(cur != &(ctl->page_map_list))) {
		map_arr = (struct page_map*)cur->page->ptr;	
		if(likely(!map_arr[idx].page)) {
			found = 1;
			break;
		}
		cur = cur->next;
	}
	if(unlikely(!found)) {
		cur = get_unused_page_item(0);
		cur->page = get_page();
		memset(cur->page->ptr, 0, cur->page->size);
		list_append(cur, &(ctl->page_map_list));
	}
	map_arr = (struct page_map*)cur->page->ptr;
	map_arr[idx].page = item;
}

inline static inline struct page_item *find_page_item_by_addr(void *ptr) {
	struct page_item *cur;
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_map *map_arr;
	int idx;
	//assert(ptr);
	cur = ctl->page_map_list.next;
	idx = get_page_map_index(ptr);
	ptr = get_page_start(ptr);
	while(likely(cur != &(ctl->page_map_list))) {
		map_arr = (struct page_map*)cur->page->ptr;
		if(likely(map_arr[idx].page && map_arr[idx].page->page->ptr == ptr))
			return map_arr[idx].page;
		cur = cur->next;
	}
	return NULL;
}

inline static inline void remove_page_map_by_addr(void *ptr) {
	struct page_item *cur;
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_map *map_arr;
	int idx;
	//assert(ptr);
	cur = ctl->page_map_list.next;
	idx = get_page_map_index(ptr);
	ptr = get_page_start(ptr);
	while(likely(cur != &(ctl->page_map_list))) {
		map_arr = (struct page_map*)cur->page->ptr;
		if(likely(map_arr[idx].page && map_arr[idx].page->page->ptr == ptr)) {
			map_arr[idx].page = NULL;
			return;
		}
		cur = cur->next;
	}
}

inline void alloc_work_page() {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *item;
	struct free_block *block;
	//assert(ctl);	
	item = get_unused_page_item(1);
	item->page = get_page();
	//init_bitmap(item);
	insert_page_map(item);
	block = (struct free_block*)item->page->ptr;
	block->order = ctl->max_order;
	block_list_append(block, &(ctl->free_list[ctl->max_order].block));
	list_append(item, &(ctl->work_page_list));
}

inline int check_buddy_free(unsigned char *bitmap, int begin_idx, int order) {
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

inline void set_block_used(unsigned char *bitmap, int begin_idx) {
	// set one bit is enough
	set_bit(bitmap, begin_idx);
}

inline void set_block_unused(unsigned char *bitmap, int begin_idx) {
	// clear one bit is enough
	clear_bit(bitmap, begin_idx);
}

inline void free_work_page(struct page_item *item) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *cur;
	struct page_map *map_arr;
	int idx;
	//assert(ctl);
	idx = get_page_map_index(item->page->ptr);
	cur = ctl->page_map_list.next;
	while(likely(cur != &(ctl->page_map_list))) {
		map_arr = (struct page_map*)cur->page->ptr;
		if(likely(map_arr[idx].page && map_arr[idx].page == item)) {
			map_arr[idx].page = NULL;
			break;
		}
		cur = cur->next;
	}
	list_remove(item);
	free_page(item->page);
	put_unused_page_item(item, 1);
}

inline void *get_free_block(int order, int sz) {
	struct bud_ctl *ctl = get_bud_ctl();
	int i, end_order = order;
	struct free_block *block = NULL, *buddy_block;
//	struct page_item *item;
	//assert(ctl);

	if(sz <= sizeof(struct page_item))
		return (void*)get_unused_page_item(0);
if(sz <= BITMAP_LEN) {
	return (void*)get_bitmap();
}

	for(i = order; i <= ctl->max_order; i++) {
		block = ctl->free_list[i].block.next;
		if(block != &(ctl->free_list[i].block)) {
			end_order = i;
			block_list_remove(block);
			break;
		}
		if(i == ctl->max_order) {
			alloc_work_page();
			block = ctl->free_list[i].block.next;
			block_list_remove(block);
			end_order = i;
		}
	}
	//assert(block);
	for(i = end_order - 1; i >= order; i--) {
		buddy_block = (struct free_block*)((char*)block + (1<<(i+SIZE_OFFSET)));
		buddy_block->order = i;
		block_list_append(buddy_block, &(ctl->free_list[i].block));
	}
//	item = find_page_item_by_addr((void*)block);
	//assert(item);
	//set_block_used(item->bitmap, get_block_index(block));
	block->order = -1;
	return (void*)block + 1;
}


int __check_buddy_free(struct free_block *buddy, int order) {
	if(buddy->order != order)
		return 0;
	return 1;
}

inline void put_free_block(void *__block, int order, int sz) {
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *item;
	struct free_block *block, *buddy_block;
	int idx;
	//assert(ctl);

	if(sz <= sizeof(struct page_item)) {
		put_unused_page_item((struct page_item*)__block, 0);
		return;
	}

if(sz <= BITMAP_LEN) {
	put_bitmap((unsigned char*)__block);
return;
}
	block = (struct free_block*)(__block - 1);

	item = find_page_item_by_addr((void*)block);
	idx = get_block_index((void*)block);
//	set_block_unused(item->bitmap, idx);
//	block->order = order
	while(order < ctl->max_order) {
		buddy_block = (struct free_block*)get_block_addr(item->page->ptr, get_buddy_index(idx, order));
		//if(check_buddy_free(item->bitmap, idx, order)) {
		if(__check_buddy_free(buddy_block, order)) {
			block_list_remove(buddy_block);
			idx = get_parent_index(idx, order);
			order++;
		}else {
			block = (struct free_block*)get_block_addr(item->page->ptr, idx);
			break;
		}
	}
	//assert(block);
	if(order == ctl->max_order)
		free_work_page(item);
	else {
		block->order = order;
		block_list_append(block, &(ctl->free_list[order].block));
	}
}

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
	if(unlikely(size + sizeof(void*) > PAGESIZE))
		return NULL;
	if(unlikely(!first_page)) {
		init_first_page();
	}
	ctl = get_bud_ctl();

	ctl->total_alloc++;

	idx = get_list_index_by_size(ctl->MultiplyDeBruijnBitPosition, __roundup_pow2(size + 1));
	return get_free_block(idx, size);
}

void
kma_free(void* ptr, kma_size_t size)
{
	struct bud_ctl *ctl = get_bud_ctl();
	struct page_item *cur;
	int count = 0;
	kma_page_t *page_array[MAXPAGES];
	//assert(ctl);

	put_free_block((struct free_block*)ptr, get_list_index_by_size(ctl->MultiplyDeBruijnBitPosition,
				__roundup_pow2(size + 1)), size);
	ctl->total_free++;

	// return all the pages if all the requests are done
	if(unlikely(ctl->total_alloc == ctl->total_free)) {
		cur = ctl->work_page_list.next;
		while(cur != &(ctl->work_page_list)) {
			//assert(cur->page->ptr);
			page_array[count++] = cur->page;
			cur = cur->next;
		}
		cur = ctl->ctl_page_list.next;
		while(cur != &(ctl->ctl_page_list)) {
			//assert(cur->page->ptr);
			page_array[count++] = cur->page;
			cur = cur->next;
		}
		cur = ctl->page_map_list.next;
		while(cur != &(ctl->page_map_list)) {
			//assert(cur->page->ptr);
			page_array[count++] = cur->page;
			cur = cur->next;
		}
		for(count = count - 1; count >= 0; count--)
			free_page(page_array[count]);
		free_page(first_page);
		first_page = NULL;
	}
}


#endif // KMA_BUD
