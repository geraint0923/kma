/***************************************************************************
 *  Title: Kernel Page Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Interface for the kernel page allocator
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
 *    Revision 1.4  2004/11/16 21:08:40  sbirrer
 *    - support continous pages
 *
 *    Revision 1.3  2004/11/16 19:12:15  sbirrer
 *    - added base address macro
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

#ifndef __KPAGE_H__
#define __KPAGE_H__

/************System include***********************************************/

/************Private include**********************************************/

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#undef EXTERN
#ifdef __KPAGE_IMPL__
#define EXTERN 
#else
#define EXTERN extern
#endif

#define PAGESIZE 8192

#define MAXPAGES 4096

/***********************************************************************
 *  Title: Base Address Macro
 * ---------------------------------------------------------------------
 *    Purpose: Get the base address of a pointer (that is the start
 *             of the page
 *    Input: pointer
 *    Output: the base address of the page
 ***********************************************************************/
#define BASEADDR(x) ((void*)(((long) (x)) & ~(PAGESIZE-1)))

typedef struct
{
  int id;
  void* ptr;
  int size;
} kma_page_t;

typedef struct
{
  int num_requested;
  int num_freed;
  int num_in_use;
  int page_size;
} kma_page_stat_t;

/************Global Variables*********************************************/

/************Function Prototypes******************************************/

/***********************************************************************
 *  Title: Allocates a memory page
 * ---------------------------------------------------------------------
 *    Purpose: Allocates a memory page
 *    Input: none
 *    Output: the allocated memory page
 ***********************************************************************/
EXTERN kma_page_t* get_page();

/***********************************************************************
 *  Title: Releases a memory page 
 * ---------------------------------------------------------------------
 *    Purpose: Releases a memory page
 *    Input: the pointer to the memory page structure
 *    Output: none
 ***********************************************************************/
EXTERN void free_page(kma_page_t*);

/***********************************************************************
 *  Title: Memory page statistics
 * ---------------------------------------------------------------------
 *    Purpose: Get the memory page statistics
 *    Input: none 
 *    Output: the memory page statistics in a static buffer
 ***********************************************************************/
EXTERN kma_page_stat_t* page_stats();

/************External Declaration*****************************************/

/**************Definition***************************************************/

#endif /* __KPAGE_H__ */
