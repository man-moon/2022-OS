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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <sys/stat.h>

#include <string.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"

static int __process_command(char *command);
/* Entry for the history */
struct entry
{
	struct list_head list;
	char *command;
	int count;
};

int count = 0;

/***********************************************************************
 * struct list_head history
 *
 * DESCRIPTION
 *   Use this list_head to store unlimited command history.
 */
LIST_HEAD(history);

/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */
static void append_history(char *const command)
{
	struct entry *e = (struct entry *)malloc(sizeof(struct entry));
	e->command = malloc(200);
	strcpy(e->command, command);
	e->count = count++;
	list_add(&(e->list), &history);
}

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

static int run_command(int nr_tokens, char *tokens[])
{
	// exit
	if (strcmp(tokens[0], "exit") == 0)
	{
		// free history memory

		return 0;
	}

	bool flag = false;
	// pipe
	for (int i = 0; i < nr_tokens; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			flag = true;
			struct entry *e = list_first_entry(&history, struct entry, list);
			char *cmd = malloc(200);
			strcpy(cmd, e->command);
			char *first = strtok(cmd, "|");
			char *second = strtok(NULL, "|");

			char *tokens_first[MAX_NR_TOKENS] = {NULL};
			int nr_tokens_first = 0;

			char *tokens_second[MAX_NR_TOKENS] = {NULL};
			int nr_tokens_second = 0;

			parse_command(first, &nr_tokens_first, tokens_first);
			parse_command(second, &nr_tokens_second, tokens_second);

			int pipefd[2];
			pid_t p1, p2;

			if (pipe(pipefd) < 0)
				return -EINVAL;

			p1 = fork();
			if (p1 < 0)
				return -EINVAL;

			if (p1 == 0) {
				close(pipefd[0]);
				dup2(pipefd[1], STDOUT_FILENO);
				close(pipefd[1]);

				if (execvp(tokens_first[0], tokens_first) < 0)
					exit(0);
			}

			else {
				p2 = fork();
				if (p2 < 0)
					return -EINVAL;

				if (!p2) {
					close(pipefd[1]);
					dup2(pipefd[0], STDIN_FILENO);
					close(pipefd[0]);

					if (execvp(tokens_second[0], tokens_second) < 0)
						exit(0);

				}

				else {
					wait(NULL);
					close(pipefd[1]);
					wait(NULL);
				}
			}
		}
	}

	if(flag) return 1;

	// cd
	if (!strcmp(tokens[0], "cd")) {

		if (nr_tokens == 1) 
			chdir(getenv("HOME"));

		else {
			if (!strcmp("~", tokens[1]))
				chdir(getenv("HOME"));

			else if (chdir(tokens[1]) == -1) {
				mkdir(tokens[1], 0777);
				chdir(tokens[1]);
			}
		}
	}

	// history command
	else if (!strcmp(tokens[0], "!")) {
		int command_line = atoi(tokens[1]);
		struct entry *e = NULL;
		struct list_head *p = NULL;

		list_for_each_prev(p, &history) {
			e = list_entry(p, struct entry, list);

			if (e->count == command_line) {
				char *new_command = malloc(200);
				strcpy(new_command, e->command);
				return __process_command(new_command);
			}
		}
	}

	// history
	else if (!strcmp(tokens[0], "history")) {
		struct entry *e = NULL;
		struct list_head *p = NULL;
		list_for_each_prev(p, &history) {
			e = list_entry(p, struct entry, list);
			fprintf(stderr, "%2d: %s", e->count, e->command);
		}
	}

	// external command
	else {
		pid_t pid = fork();
		if (pid == 0) {
			if (execvp(tokens[0], tokens) < 0) {
				fprintf(stderr, "Unable to execute %s\n", tokens[0]);
				exit(0);
			}
		}

		else if (pid > 0)
			wait(NULL);

		else
			return -EINVAL;
	}

	return 1;
}

/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char *const argv[])
{
	return 0;
}

/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char *const argv[])
{
}

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
static int __process_command(char *command)
{
	char *tokens[MAX_NR_TOKENS] = {NULL};
	int nr_tokens = 0;

	if (parse_command(command, &nr_tokens, tokens) == 0)
		return 1;

	return run_command(nr_tokens, tokens);
}

static bool __verbose = true;
static const char *__color_start = "[0;31;40m";
static const char *__color_end = "[0m";

static void __print_prompt(void)
{
	char *prompt = "$";
	if (!__verbose)
		return;

	fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char *const argv[])
{
	char command[MAX_COMMAND_LEN] = {'\0'};
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1)
	{
		switch (opt)
		{
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv)))
		return EXIT_FAILURE;

	/**
	 * Make stdin unbuffered to prevent ghost (buffered) inputs during
	 * abnormal exit after fork()
	 */
	setvbuf(stdin, NULL, _IONBF, 0);

	while (true)
	{
		__print_prompt();

		if (!fgets(command, sizeof(command), stdin))
			break;

		append_history(command);
		ret = __process_command(command);

		if (!ret)
			break;
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
