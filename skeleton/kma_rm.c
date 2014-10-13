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

kma_page_t *first_page = NULL;

struct page_info {
	kma_page_t *page;
	int ref_count;
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
};


void add_page_for_free_node() {
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
	info->ref_count = 0;
	ctl->total_alloc = 0;
	ctl->total_free = 0;
	ctl->free_list.prev = ctl->free_list.next = &(ctl->free_list);
	ctl->unused_list.prev = ctl->unused_list.next = &(ctl->unused_list);

}

void*
kma_malloc(kma_size_t size)
{
	if(size + sizeof(void*) > PAGESIZE)
		return NULL;
	if(!first_page) {
		init_first_page();
	}
	return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
	;
}

#endif // KMA_RM
