/**********************************************************************
 * Copyright (c) 2020
 *  Jinwoo Jeong <jjw8967@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdint.h>

#include "malloc.h"
#include "types.h"
#include "list_head.h"

#define ALIGNMENT 32
#define HDRSIZE sizeof(header_t)
#define INT_MAX 2147483647

static LIST_HEAD(free_list); // Don't modify this line
static algo_t g_algo;        // Don't modify this line
static void *bp;             // Don't modify thie line

/***********************************************************************
 * extend_heap()
 *
 * DESCRIPTION
 *   allocate size of bytes of memory and returns a pointer to the
 *   allocated memory.
 *
 * RETURN VALUE
 *   Return a pointer to the allocated memory.
 */
struct header_t
{
  size_t size;
	bool free;
  struct list_head list;
};

void *my_malloc(size_t size)
{
  /* Implement this function */
  if (g_algo == FIRST_FIT) {
    struct header_t *find_e = NULL;
    struct list_head *p = NULL;

    list_for_each(p, &free_list){
        find_e = list_entry(p, struct header_t, list);

        //fit
        if(find_e->free == true && find_e->size >= size) {
          
          if(find_e->size == size) {
            find_e->free = false;
            return (void *)(find_e + 1);
          }

          else  {
            int alloc_data_size = ((size % 32) ? (32 + (32 * ((long unsigned int)(size/32)))) : size);
            if((long unsigned int)alloc_data_size == find_e->size){
              find_e->free = false;
              return (void *)(find_e + 1);
            }

            find_e->free = false;
            size_t tmp_size = find_e->size;
            find_e->size = alloc_data_size;

            struct header_t *new_e = (struct header_t *)(void *)(find_e + 1 + (alloc_data_size/32));
            new_e->free = true;
            new_e->size = tmp_size - alloc_data_size - HDRSIZE;

            struct header_t *next_e = list_next_entry(find_e, list);
            __list_add(&(new_e->list), &(find_e->list), &(next_e->list));
            return (void *)(find_e + 1);
          }
        }
    }

    //no fit
    int alloc_data_size = ((size % 32) ? (32 + (32 * ((long unsigned int)(size/32)))) : size);

    struct header_t *e = (struct header_t *)sbrk(HDRSIZE + alloc_data_size);

    e->size = alloc_data_size;
    e->free = false;

    struct header_t *e2 = NULL;
    struct list_head *p2 = NULL;
    list_for_each_prev(p2, &free_list) {
      e2 = list_entry(p2, struct header_t, list);
      if(e2->free == false) {
        struct header_t *next_e2 = list_next_entry(e2, list);
        __list_add(&(e->list), &(e2->list), &(next_e2->list));
          
        return (void *)(e + 1);
      }
    }
    list_add_tail(&(e->list), &free_list);
    return (void *)(e + 1);
  }






  if(g_algo == BEST_FIT) {
    struct header_t *find_e = NULL;
    struct list_head *p = NULL;

    int min = INT_MAX;
    struct header_t *opt_e = NULL;
    list_for_each(p, &free_list){
        find_e = list_entry(p, struct header_t, list);
        if(find_e ->free == true && find_e->size >= size) {
          if((long unsigned int)min > find_e->size - size) {
            min = find_e->size - size;
            opt_e = find_e;
          
          }
        }
    }

        //fit
        if(opt_e) {
          if(opt_e->size == size) {
            opt_e->free = false;
            return (void *)(opt_e + 1);
          }

          else  {
            int alloc_data_size = ((size % 32) ? (32 + (32 * ((long unsigned int)(size/32)))) : size);
            if((long unsigned int)alloc_data_size == opt_e->size){
              opt_e->free = false;
              return (void *)(opt_e + 1);
            }

            opt_e->free = false;
            size_t tmp_size = opt_e->size;
            opt_e->size = alloc_data_size;

            struct header_t *new_e = (struct header_t *)(void *)(opt_e + 1 + (alloc_data_size/32));
            new_e->free = true;
            new_e->size = tmp_size - alloc_data_size - HDRSIZE;

            struct header_t *next_e = list_next_entry(opt_e, list);
            __list_add(&(new_e->list), &(opt_e->list), &(next_e->list));
            return (void *)(opt_e + 1);
          }
    }

    //no fit
    else {
      int alloc_data_size = ((size % 32) ? (32 + (32 * ((long unsigned int)(size/32)))) : size);

      struct header_t *e = (struct header_t *)sbrk(HDRSIZE + alloc_data_size);

      e->size = alloc_data_size;
      e->free = false;

      struct header_t *e2 = NULL;
      struct list_head *p2 = NULL;
      list_for_each_prev(p2, &free_list) {
        e2 = list_entry(p2, struct header_t, list);
        if(e2->free == false) {
          struct header_t *next_e2 = list_next_entry(e2, list);
          __list_add(&(e->list), &(e2->list), &(next_e2->list));
          
          return (void *)(e + 1);
        }
      }
      list_add_tail(&(e->list), &free_list);
      return (void *)(e + 1);
    }
  }

  return NULL;
}
/***********************************************************************
 * my_realloc()
 *
 * DESCRIPTION
 *   tries to change the size of the allocation pointed to by ptr to
 *   size, and returns ptr. If there is not enough memory block,
 *   my_realloc() creates a new allocation, copies as much of the old
 *   data pointed to by ptr as will fit to the new allocation, frees
 *   the old allocation.
 *
 * RETURN VALUE
 *   Return a pointer to the reallocated memory
 */
void *my_realloc(void *ptr, size_t size)
{
  /* Implement this function */
  struct header_t *e = (struct header_t *)(ptr - 32);
  if(size == e->size) return ptr;

  void* p = my_malloc(size);
  my_free(ptr);
  return p;
}

/***********************************************************************
 * my_realloc()
 *
 * DESCRIPTION
 *   deallocates the memory allocation pointed to by ptr.
 */
// void my_free(void *ptr)
// {
//   /* Implement this function */

//   struct header_t *e = (struct header_t *)(ptr - 32);
//   struct header_t *next_e = list_next_entry(e, list);
//   struct header_t *prev_e = list_prev_entry(e, list);

//   e->free = true;
//   if((void *)prev_e != (void *)(&free_list - 1) && (void *)next_e == (void *)(&free_list - 1)) {
//     if(prev_e->free == true) {
//       prev_e->size += e->size + 32;
//       list_del(&e->list);
//     }
//   }

//   if((void *)prev_e == (void *)(&free_list - 1) && (void *)next_e != (void *)(&free_list - 1)) {
//     if(next_e->free == true) {
//       e->size += next_e->size + 32;
//       list_del(&next_e->list);
//     }
//   }

//   if((void *)prev_e != (void *)(&free_list - 1) && (void *)next_e != (void *)(&free_list - 1)) {
//     if(prev_e->free == true && next_e->free == true) {
//       prev_e->size += (e->size + next_e->size + 64);
//       list_del(&e->list);
//       list_del(&next_e->list);
//     }
//     if(prev_e->free == true && next_e->free == false ) {
//       prev_e->size += e->size + 32;
//       list_del(&e->list);
//     }
//     if(prev_e->free == false && next_e->free == true) {
//       e->size += next_e->size + 32;
//       list_del(&next_e->list);
//     }
//   }
//   return;
// }
void my_free(void *ptr)
{
  header_t *e = (header_t *)(ptr - 32);
  header_t *e_next = list_next_entry(e, list);
  header_t *e_prev = list_prev_entry(e, list);
  e->free = true;
  if(e_next->free == true) {
    e->size += e_next->size + 32;
    list_del(&(e_next->list));
  }
  if(e_prev->free == true) {
    e_prev->size += e->size + 32;
    list_del(&(e->list));
  }
  return;
}

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
/*          ****** EXCEPT TO mem_init() AND mem_deinit(). ******      */
void mem_init(const algo_t algo)
{
  g_algo = algo;
  bp = sbrk(0);
}

void mem_deinit()
{
  header_t *header;
  size_t size = 0;
  list_for_each_entry(header, &free_list, list) {
    size += HDRSIZE + header->size;
  }
  sbrk(-size);

  if (bp != sbrk(0)) {
    fprintf(stderr, "[Error] There is memory leak\n");
  }
}

void print_memory_layout()
{
  header_t *header;
  int cnt = 0;

  printf("===========================\n");
  list_for_each_entry(header, &free_list, list) {
    cnt++;
    printf("%c %ld\n", (header->free) ? 'F' : 'M', header->size);
  }

  printf("Number of block: %d\n", cnt);
  printf("===========================\n");
  return;
}
