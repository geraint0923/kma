#ifndef __UTILS_H__
#define __UTILS_H__

extern int roundup_pow2(int num);

extern void *get_page_start(void *addr);

extern void *get_page_end(void *addr);

extern int get_buddy_index(int idx, int order);

extern int get_parent_index(int idx, int order);

extern void set_bit(unsigned char *bitmap, int idx);

extern void clear_bit(unsigned char *bitmap, int idx);

extern int get_bit(unsigned char *bitmap, int idx);

extern int get_set_bit_num(unsigned int num);

#endif	// __UTILS_H__
