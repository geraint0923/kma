/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Interface for the kernel memory allocator
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.3  2009/10/31 21:28:52  jot836
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
 *    Revision 1.2  2009/10/21 07:06:46  npb853
 *    New test framework in place. Also adding a new sample testcase file
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
 *    Revision 1.1  2004/11/03 18:34:52  sbirrer
 *    - initial version of the kernel memory project
 *
 ***************************************************************************/

#ifndef __KMA_H__
#define __KMA_H__

/************System include***********************************************/

/************Private include**********************************************/

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#define bool short
#define TRUE 1
#define FALSE 0

#undef EXTERN
#ifdef __KMA_IMPL__
#define EXTERN
#else
#define EXTERN extern
#endif

typedef int kma_size_t;

/************Global Variables*********************************************/

/************Function Prototypes******************************************/

/***********************************************************************
 *  Title: Allocates kernel memory
 * ---------------------------------------------------------------------
 *    Purpose: Allocates size bytes and returns a pointer to the
 *             allocated kernel memory
 *    Input: the size
 *    Output: the allocated memory of the specified size
 *            or NULL on failure
 ***********************************************************************/
EXTERN void* kma_malloc(kma_size_t size);

/***********************************************************************
 *  Title: Frees kernel memory spaced
 * ---------------------------------------------------------------------
 *    Purpose: Frees the memory space pointed to by ptr, which must
 *             have been returned by a previous call to kma_malloc()
 *    Input: the pointer to the memory space, the size of the memory
 *           space
 *    Output: none
 ***********************************************************************/
EXTERN void kma_free(void*, kma_size_t size);

/************External Declaration*****************************************/

/**************Definition***************************************************/

void error(char* message, char* arg );

#endif /* __KMA_H__ */
