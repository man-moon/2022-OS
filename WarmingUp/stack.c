/**********************************************************************
 * Copyright (c) 2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
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

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING FROM THIS LINE ******       */
#include <stdio.h>
#include "types.h"
#include "list_head.h"

/* Declaration for the stack instance defined in pa0.c */
extern struct list_head stack;

/* Entry for the stack */
struct entry {
	struct list_head list;
	char *string;
};
/*          ****** DO NOT MODIFY ANYTHING ABOVE THIS LINE ******      */
/*====================================================================*/
/*====================================================================*
 * The rest of this file is all yours. This implies that you can      *
 * include any header files if you want to ...                        */

#include <stdlib.h>                    /* like this */
#include <string.h>
/**
 * push_stack()
 *
 * DESCRIPTION
 *   Push @string into the @stack. The @string should be inserted into the top
 *   of the stack. You may use either the head or tail of the list for the top.
 */
void push_stack(char *string)
{
	/* TODO: Implement this function */

	struct entry *e1 = (struct entry*)malloc(sizeof(struct entry));
	e1->string = malloc(100);
	strcpy(e1->string, string);
	list_add(&(e1->list), &stack);

	//test code
	//struct entry *le = list_last_entry(&stack, struct entry, list);
	//printf("Last entry: %s\n", le->string);
	return;
}


/**
 * pop_stack()
 *
 * DESCRIPTION
 *   Pop a value from @stack and return it through @buffer. The value should
 *   come from the top of the stack, and the corresponding entry should be
 *   removed from @stack.
 *
 * RETURN
 *   If the stack is not empty, pop the top of @stack, and return 0
 *   If the stack is empty, return -1
 */
int pop_stack(char *buffer)
{	
	/* TODO: Implement this function */
	if(list_empty(&stack)) return -1;
	
	struct entry *e = list_first_entry(&stack, struct entry, list);
	strcpy(buffer, e->string);
	list_del(&e->list);
	free(e->string);
	free(e);
	return 0;
}


/**
 * dump_stack()
 *
 * DESCRIPTION
 *   Dump the contents in @stack. Print out @string of stack entries while
 *   traversing the stack from the bottom to the top. Note that the value
 *   should be printed out to @stderr to get properly graded in pasubmit.
 */
void dump_stack(void)
{
	/* TODO: Implement this function */
	struct entry *e = NULL;
	struct list_head *p = NULL;
	list_for_each_prev(p, &stack){
		e = list_entry(p, struct entry, list);
		//printf("Present entry: %s\n", e->string);
		fprintf(stderr, "%s\n", e->string);
	}

	//fprintf(stderr, "%s\n", "0xdeadbeef"); /* Example. 
	//Print out values in this form */
}
