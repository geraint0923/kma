/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Test suite for the kernel memory allocator
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.1  2005/10/24 16:17:20  sbirrer
 *    - added test suite
 *
 *    Revision 1.3  2004/11/16 19:33:50  sbirrer
 *    - increased the request size
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 *    Revision 1.1  2004/11/03 18:34:52  sbirrer
 *    - initial version of the kernel memory project
 *
 ***************************************************************************/
	#define __KMA_TEST_IMPL__

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

	/*  These variables define the test sequence. For grading we will use
	 *  a different set of variable values. It is your responsibility to
	 *  verify the general correctness of your algorithm.
	 *
	 *  For debugging purpose, it may be helpfull to set NUM_OPS initially
	 *  to a small number. 
	 */
	#define SEED 113
	#define NUM_OPS 24000
	#define PROB_ALLOC 0.6
	#define MAX_SIZE 8000
	#define PROB_OVERSIZED 0.02

	typedef struct mem
	{
		int size;
		void* ptr;
		void* value; // to check correctness
		struct mem* next;
	} mem_t;
	
  /************Global Variables*********************************************/

	static mem_t* first = NULL;
	static int size = 0;
	static int val = 0;
	
  /************Function Prototypes******************************************/
	float frand();
	void allocate();
	void deallocate();
	void fill(char*, int);
	void check(char*, char*, int);
	
  /************External Declaration*****************************************/

/**************Implementation***********************************************/

int main(int argc, char* argv[])
{
	int i;
	int n_alloc=0, n_dealloc=0;
	kma_page_stat_t* stat;
	srand(SEED);

	for (i = 0; i < NUM_OPS; i++)
	{
		if ((first != NULL) && (first->next != NULL) && (frand() > PROB_ALLOC))
		{ // always one page remaining
			deallocate();
			n_dealloc++;
		}
		else
		{
			allocate();
			n_alloc++;
		}
	}

	stat = page_stats();

	printf("Allocation/Deallocation:     %5d/%5d\n",
		   n_alloc, n_dealloc);
	printf("Page Requested/Freed/In_Use: %5d/%5d/%5d\n",
		   stat->num_requested, stat->num_freed, stat->num_in_use);

	printf("Freeing all memory now\n");
	// free all allocated memory
	while (first)
	{
		deallocate();
	}

	stat = page_stats();

	printf("Page Requested/Freed/In Use: %5d/%5d/%5d\n",
		   stat->num_requested, stat->num_freed, stat->num_in_use);	

	if ((stat->num_requested == stat->num_freed) && stat->num_in_use == 0)
	{
		printf("Test: PASS\n");
	}
	else
	{
		printf("Test: FAILED\n");
	}

	exit(0);
}

float frand()
{
	return	((float) rand()) / ((float) RAND_MAX);
}

void allocate()
{
	mem_t* new = malloc(sizeof(mem_t));
	new->size = 0;

	while (new->size == 0)
	{
		new->size = frand() * MAX_SIZE;
	}

	if (frand() < PROB_OVERSIZED)
	{
		new->size += PAGESIZE;
	}

	new->ptr = kma_malloc(new->size);

	assert(((new->ptr != NULL) && (new->size <= MAX_SIZE))
		   || ((new->ptr == NULL) && (new->size > MAX_SIZE)));
	
	if (new->ptr == NULL)
	{
		free(new);
		return;
	}	
	
	new->value = malloc(new->size);

	// initialize memory
	fill((char*)new->ptr, new->size);

	// copy the value for further reference
	bcopy(new->ptr, new->value, new->size);
	
	check((char*)new->ptr, (char*)new->value, new->size);
	
	// add to list
	new->next = first;
	first = new;
	size++;
}

void deallocate()
{
	int idx = frand() * (size - 1);
	mem_t* cur = first;
	mem_t* prv = NULL;

	assert(size > 0);

	while (idx > 0) // find current
	{
		prv = cur;
		cur = cur->next;
		idx--;	
	}

	assert(cur->size > 0);
	
	// check memory
	check((char*)cur->ptr, (char*)cur->value, cur->size);
	
	kma_free(cur->ptr, cur->size);

	// fix structure
	size--;
	if (prv)
	{
		prv->next = cur->next;
	}
	else
	{
		first = cur->next;
	}

	free(cur->value);
	free(cur);
}

void fill(char* ptr, int size)
{
	int i;
	
	for (i = 0; i < size; i++)
	{
		ptr[i] = (char) val++;
	}
}

void check(char* lhs, char* rhs, int size)
{
	int i;
	
	for (i = 0; i < size; i++)
	{
		if (lhs[i] != rhs[i])
		{
			fprintf(stderr, "memory mismatch at position %d (%3d!=%3d)\n",
				   	i, lhs[i], rhs[i]);
		}
	}
}
