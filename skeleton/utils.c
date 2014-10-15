#include "utils.h"
#include "kma_page.h"

int roundup_pow2(int v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}


void *get_page_start(void *addr) {
	return (void*)((unsigned long)addr & ~((unsigned long)(PAGESIZE-1)));
};

void *get_page_end(void *addr) {
	return (void*)((char*)get_page_start(addr) + PAGESIZE);
}


int get_buddy_index(int idx, int order) {
	return idx ^ (1 << order);
}

int get_parent_index(int idx, int order) {
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

int get_set_bit_num(unsigned int i)
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}


/*
   unsigned int v;  // find the number of trailing zeros in 32-bit v 
   int r;           // result goes here
   static const int MultiplyDeBruijnBitPosition[32] = 
   {
   0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
   31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
   };
   r = MultiplyDeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];
*/
