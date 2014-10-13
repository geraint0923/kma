/***************************************************************************
 *  Title: Kernel Page Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Implementation of the kernel page allocator
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
 *    Revision 1.3  2004/11/16 21:08:40  sbirrer
 *    - support continous pages
 *
 *    Revision 1.2  2004/11/15 04:23:40  sbirrer
 *    - use constant alignment for pages
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 *    Revision 1.1  2004/11/03 18:34:52  sbirrer
 *    - initial version of the kernel memory project
 *
 ***************************************************************************/
#define __KPAGE_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
static kma_page_stat_t kma_page_stats = { 0, 0, 0, PAGESIZE };

static void* pool = NULL;
static void* next_free_page = NULL;

/************Function Prototypes******************************************/
void* allocPage();
void freePage(void*);
void initPages();

/************External Declaration*****************************************/

/**************Implementation***********************************************/

kma_page_t*
get_page()
{
  static int id = 0;
  kma_page_t* res;
  
  kma_page_stats.num_requested++;
  kma_page_stats.num_in_use++;
  
  res = (kma_page_t*) malloc(sizeof(kma_page_t));
  res->id = id++;
  res->size = kma_page_stats.page_size;
  res->ptr = allocPage();
  
  assert(res->ptr != NULL);
  
  return res;	
}

void
free_page(kma_page_t* ptr)
{
  assert(ptr != NULL);
  assert(ptr->ptr != NULL);
  assert(kma_page_stats.num_in_use > 0);
  
  kma_page_stats.num_freed++;
  kma_page_stats.num_in_use--;
  
  freePage(ptr->ptr);
  free(ptr);
}

kma_page_stat_t*
page_stats()
{
  static kma_page_stat_t stats;
  
  return memcpy(&stats, &kma_page_stats, sizeof(kma_page_stat_t));
}

void*
allocPage()
{
  void* res;
  
  if (pool == NULL)
    {
      initPages();
    }
  
  res = next_free_page;
  
  if (res == NULL)
    {
      error("error: all pages already allocated", "");
    }
  
  next_free_page = *((void**)next_free_page);
  
  assert(res != NULL);
  
  return res;
}

void
freePage(void* ptr)
{
  assert(ptr != NULL);
  
  *((void**)ptr) = next_free_page;
  next_free_page = ptr;
  
  if (kma_page_stats.num_in_use == 0)
    {
      free(pool);
      pool = NULL;
      next_free_page = NULL;
    }
}

void
initPages()
{
  int i;
  
  assert(next_free_page == NULL);
  assert(pool == NULL);
  
  //pool = calloc(MAXPAGES, PAGESIZE);
  int result = posix_memalign(&pool, PAGESIZE, MAXPAGES * PAGESIZE);
  if(result)
    error("Error using posix_memalign to allocate memory", "");
  next_free_page = pool;
  
  // use ptr to point to the next free page struct
  for (i = 0; i < (MAXPAGES - 1); i++)
    {
      void* ptr = (pool + i * PAGESIZE);
      void* next = ptr + PAGESIZE;
      
      *((void**) ptr) = next;
    }
  
  *((void**)(pool + (MAXPAGES - 1) * PAGESIZE)) = NULL;
}
