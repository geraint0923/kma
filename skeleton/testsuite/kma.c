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
 *    Revision 1.4  2004/11/30 22:11:42  sbirrer
 *    - assure always one allocation pending during test
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

enum REQ_STATE
  {
    FREE,
    USED
  };

typedef struct mem
{
  int size;
  void* ptr;
  void* value; // to check correctness
  enum REQ_STATE state;
} mem_t;

/************Global Variables*********************************************/

static int val = 0;

/************Function Prototypes******************************************/
void allocate();
void deallocate();
void fill(char*, int);
void check(char*, char*, int);
void usage();
void error(char*, char*);
void pass();
void fail();

/************External Declaration*****************************************/



/**************Implementation***********************************************/

int anyMismatches = 0;

int currentAllocBytes = 0;

char *name = NULL;

int
main(int argc, char* argv[])
{
  
  name = argv[0];
  
#ifdef COMPETITION
  printf("%s: Running in competition mode\n", name);
#endif

#ifndef COMPETITION
  printf("%s: Running in correctness mode\n", name);
#endif

  int n_req = 0, n_alloc=0, n_dealloc=0;
  kma_page_stat_t* stat;

#ifdef COMPETITION
  double ratioSum = 0.0;
  int ratioCount = 0;
#endif
  
#ifndef COMPETITION
  FILE* allocTrace = fopen("kma_output.dat", "w");
  if (allocTrace == NULL)
    {
      error("unable to open allocation output file", "kma_output.dat");
    }
  fprintf(allocTrace, "0 0 0\n");
#endif

  if (argc != 2)
    {
      usage();
    }
  
  FILE* f_test = fopen(argv[1], "r");
  if (f_test == NULL)
    {
      error("unable to open input test file", argv[1]);
    }
  
  // Get the number of requests in the trace file
  // Allocate some memory...
  int status = fscanf(f_test, "%d\n", &n_req);
  if(status != 1)
    error("Couldn't read number of requests at head of file", "");
  
  mem_t* requests = malloc((n_req + 1)*sizeof(mem_t));
  memset(requests, 0, (n_req + 1)*sizeof(mem_t));
  
  char command[16];
  int req_id, req_size, index = 1;

  // Parse the lines in the file, and call allocate or
  // deallocate accordingly.
  while (fscanf(f_test, "%10s", command) == 1)
    {
      if (strcmp(command, "REQUEST") == 0)
	{
	  
	  if (fscanf(f_test, "%d %d", &req_id, &req_size) != 2)
	    error("Not enough arguments to REQUEST", "");

	  assert(req_id >= 0 && req_id < n_req);
	  
	  allocate(requests, req_id, req_size);
	  n_alloc++;
	}
      else if (strcmp(command, "FREE") == 0)
	{
	  if (fscanf(f_test, "%d", &req_id) != 1)
	    error("Not enough arguments to FREE", "");
	  
	  assert(req_id >= 0 && req_id < n_req);
	  
	  deallocate(requests, req_id);
	  n_dealloc++;
	}
      else
	{
	  error("unknown command type:", command);
	}

      stat = page_stats();
      int totalBytes = stat->num_in_use * stat->page_size;

      
#ifdef COMPETITION
      if(req_id < n_req && n_alloc != n_dealloc)
	{
	  // We can calculate the ratio of wasted to used memory here.

	  int wastedBytes = totalBytes - currentAllocBytes;
	  ratioSum += ((double) wastedBytes) / currentAllocBytes;
	  ratioCount += 1;
	}
#endif

#ifndef COMPETITION
      fprintf(allocTrace, "%d %d %d\n", index, currentAllocBytes, totalBytes);
#endif
      
      index += 1;
    }

#ifndef COMPETITION
  fclose(allocTrace);
#endif
  
  
  stat = page_stats();
  
  printf("Page Requested/Freed/In Use: %5d/%5d/%5d\n",
	 stat->num_requested, stat->num_freed, stat->num_in_use);	
  
  if (stat->num_requested != stat->num_freed || stat->num_in_use != 0)
    {
      error("not all pages freed", "");
    }
  
  if(anyMismatches)
    {
      error("there were memory mismatches", "");
    }

#ifdef COMPETITION
  printf("Competition average ratio: %f\n", ratioSum / ratioCount);
#endif
  
  pass();
  return 0;
}

void
fail()
{
  printf("Test: FAILED\n");
  exit(-1);
}

void
pass()
{
  printf("Test: PASS\n");
  exit(0);
}

void
usage() {
  printf("Usage: %s traceFile\n", name);
  exit(0);
}

void
error(char* message, char* arg ) {
  fprintf(stderr, "ERROR: %s: %s.\n", message, arg);
  fail();
}

void
allocate(mem_t* requests, int req_id, int req_size)
{
  mem_t* new = &requests[req_id];
  
  assert(new->state == FREE);
  
  new->size = req_size;
  new->ptr = kma_malloc(new->size);
  
  // Accept a NULL response in some cases... 
  if(!(((new->ptr != NULL) && (new->size <= (PAGESIZE - sizeof(void*))))
       || ((new->ptr == NULL) && (new->size > (PAGESIZE - sizeof(void*))))))
    {
      error("got NULL from kma_malloc for alloc'able request", "");
    }
  
  if (new->ptr == NULL)
    {
      return;
    }

  currentAllocBytes += req_size;
  
#ifndef COMPETITION
  // Only run the actual memory accesses/copies/checks if we're
  // testing for correctness.
  
  new->value = malloc(new->size);
  assert(new->value != NULL);
  
  // initialize memory
  fill((char*)new->ptr, new->size);
  
  // copy the value for further reference
  bcopy(new->ptr, new->value, new->size);
  
  check((char*)new->ptr, (char*)new->value, new->size);
  
#endif

  new->state = USED;
}

void
deallocate(mem_t* requests, int req_id)
{
  mem_t* cur = &requests[req_id];
  
  assert(cur->state == USED);
  assert(cur->size > 0);
  
#ifndef COMPETITION
  // Only run the memory checks if we're testing for correctness.

  // check memory
  check((char*)cur->ptr, (char*)cur->value, cur->size);

  // free memory
  free(cur->value);
#endif

  kma_free(cur->ptr, cur->size);

  currentAllocBytes -= cur->size;
  
  cur->state = FREE;
}

void
fill(char* ptr, int size)
{
  int i;
  
  for (i = 0; i < size; i++)
    {
      ptr[i] = (char) val++;
    }
}

void
check(char* lhs, char* rhs, int size)
{
  int i;
  
  for (i = 0; i < size; i++)
    {
      if (lhs[i] != rhs[i])
	{
	  fprintf(stderr, "memory mismatch at position %d (%3d!=%3d)\n", 
		  i, lhs[i], rhs[i]);
	  anyMismatches = 1;
	}
    }
}
