/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Dummy kernel memory allocator
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
 *    Revision 1.5  2004/11/16 19:12:15  sbirrer
 *    - added base address macro
 *
 *    Revision 1.4  2004/11/11 23:22:54  sbirrer
 *    - updated the dummy algorithm to use the page as the storage for the pointer
 *
 *    Revision 1.3  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.2  2004/11/05 13:56:01  sbirrer
 *    - compare request size to page size
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_DUMMY
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

void* kma_malloc(kma_size_t size)
{
  kma_page_t* page;
  
  // get one page
  page = get_page();
  
  // add a pointer to the page structure at the beginning of the page
  *((kma_page_t**)page->ptr) = page;
  
  if ((size + sizeof(kma_page_t*)) > page->size)
    { // requested size too large
      free_page(page);
      return NULL;
    }
  
  // check whether the BASEADDR macro works
  //for (i = 0; i < page->size; i++)
  //{
  //  assert(BASEADDR(page->ptr + i) == page->ptr);
  //}
  // oh yea, it worked
  
  return page->ptr + sizeof(kma_page_t*);
}

void kma_free(void* ptr, kma_size_t size)
{
  kma_page_t* page;
  
  page = *((kma_page_t**)(ptr - sizeof(kma_page_t*)));
  
  free_page(page);
}

#endif // KMA_DUMMY
