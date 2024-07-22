#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "pico-con.h"

#define CH_ASCII_ETX 0x03
#define CH_ASCII_ESC 0x1b

#define INPUT_STATE_CHAR 0
#define INPUT_STATE_ESC  1
#define INPUT_STATE_CSI  2

struct pico_con_input
{
	int    state;
	size_t idx;
	size_t buffer_size;
	char   *buffer;
};

static int pico_con_input_init(struct pico_con_input *input, size_t input_buffer_size)
{
	input->state = INPUT_STATE_CHAR;
	input->idx = 0;
	input->buffer_size = input_buffer_size;
	input->buffer = malloc(input->buffer_size + 1);
	if (!input->buffer)
	{
		return -1;
	}

	return 0;
}

static void pico_con_input_free(struct pico_con_input *input)
{
	free(input->buffer);
	input->buffer = NULL;
}

static char *pico_con_input_get_token(struct pico_con_input *input, size_t start, size_t end)
{
	size_t size = end - start;
	char *token = malloc(size + 1);
	if (!token)
	{
		return NULL;
	}

	memcpy(token, input->buffer+start, size);
	token[size] = '\0';

	return token;
}

#define PARSER_ARG_CAPACITY_MIN 16

static size_t pico_con_parser_expand_args(size_t argc, char ***argv, size_t capacity)
{
	if (!*argv)
	{
		if (capacity <= 0) capacity = PARSER_ARG_CAPACITY_MIN;
		*argv = malloc(capacity*sizeof(char *));
		if (!*argv)
		{
			return 0;
		}
	}
	else if (argc >= capacity)
	{
		return 0;
	}

	return capacity;
}

static void pico_con_parser_cleanup(char **command, size_t *argc, char ***argv)
{
	if (*command)
	{
		free(*command);
		*command = NULL;
	}

	if (*argv)
	{
		while (*argc > 0)
		{
			(*argc)--;
			free((*argv)[*argc]);
		}

		free(*argv);
		*argv = NULL;
	}
}

#define PARSER_STATE_PRE_COMMAND 0
#define PARSER_STATE_COMMAND     1
#define PARSER_STATE_PRE_ARG     2
#define PARSER_STATE_ARG         3

static int pico_con_parse(struct pico_con_input *input, char **command, size_t *argc, char ***argv)
{
#ifdef DEBUG
	printf("\ninput: \"%s\"\n", input->buffer);
#else
	putchar('\n');
#endif

	int parser_state = PARSER_STATE_PRE_COMMAND;
	size_t i = 0;
	size_t start = 0;

	size_t arg_capacity = 0;
	*argc = 0;
	*argv = NULL;

	while (i <= input->buffer_size && input->buffer[i] != '\0')
	{
		switch (parser_state)
		{
		case PARSER_STATE_COMMAND:
			if (isspace(input->buffer[i]))
			{
				parser_state = PARSER_STATE_PRE_ARG;

				*command = pico_con_input_get_token(input, start, i);
				if (!*command)
				{
					pico_con_parser_cleanup(command, argc, argv);
					return -1;
				}
			}
			break;
		case PARSER_STATE_PRE_ARG:
			if (!isspace(input->buffer[i]))
			{
				start = i;
				parser_state = PARSER_STATE_ARG;
			}
			break;
		case PARSER_STATE_ARG:
			if (isspace(input->buffer[i]))
			{
				parser_state = PARSER_STATE_PRE_ARG;

				arg_capacity = pico_con_parser_expand_args(*argc, argv, arg_capacity);
				if (!arg_capacity)
				{
					pico_con_parser_cleanup(command, argc, argv);
					return -1;
				}

				(*argv)[*argc] = pico_con_input_get_token(input, start, i);
				if (!(*argv)[*argc])
				{
					pico_con_parser_cleanup(command, argc, argv);
					return -1;
				}
				(*argc)++;
			}
			break;
		default:
			if (!isspace(input->buffer[i]))
			{
				start = i;
				parser_state = PARSER_STATE_COMMAND;
			}
		}

		i++;
	}

	if (parser_state == PARSER_STATE_COMMAND)
	{
		*command = pico_con_input_get_token(input, start, i);
		if (!*command)
		{
			pico_con_parser_cleanup(command, argc, argv);
			return -1;
		}
	}
	else if (parser_state == PARSER_STATE_ARG)
	{
		arg_capacity = pico_con_parser_expand_args(*argc, argv, arg_capacity);
		if (!arg_capacity)
		{
			pico_con_parser_cleanup(command, argc, argv);
			return -1;
		}

		(*argv)[*argc] = pico_con_input_get_token(input, start, i);
		if (!(*argv)[*argc])
		{
			pico_con_parser_cleanup(command, argc, argv);
			return -1;
		}
		(*argc)++;
	}

	return 0;
}

static int pico_con_input_handle_char(struct pico_con_input *input, int ch, struct pico_con_command *commands)
{
	switch (input->state)
	{
	case INPUT_STATE_ESC:
		if (ch == '[')
		{
			input->state = INPUT_STATE_CSI;
		}
		else
		{
			input->state = INPUT_STATE_CHAR;
		}
		break;
	case INPUT_STATE_CSI:
		putchar('\a');
		input->state = INPUT_STATE_CHAR;
		break;
	default:
		if (isprint(ch))
		{
			if (input->idx < input->buffer_size)
			{
				input->buffer[input->idx] = ch;
				input->idx++;
				putchar(ch);
			}
			else
			{
				putchar('\a');
			}
		}
		else if (ch == '\b')
		{
			if (input->idx > 0)
			{
				input->idx--;
				putchar(ch);
				putchar(' ');
				putchar(ch);
			}
			else
			{
				putchar('\a');
			}
		}
		else if (ch == '\n' || ch == '\r')
		{
			input->buffer[input->idx] = '\0';
			input->idx = 0;

			char *command = NULL;
			size_t argc = 0;
			char **argv = NULL;
			if (pico_con_parse(input, &command, &argc, &argv) != 0)
			{
				printf("Failed to parse input \"%s\"\n> ", input->buffer);
				return -1;
			}

#ifdef DEBUG
			printf("Command: \"%s\"\n", command);
#endif

			size_t i = 0;
			while (commands[i].handler)
			{
				if (commands[i].name && !strcmp(command, commands[i].name))
				{
					int r = commands[i].handler(argc, argv);

					pico_con_parser_cleanup(&command, &argc, &argv);
					if (r != PICO_CON_COMMAND_SUCCESS)
					{
						return -1;
					}

					putchar('>');
					putchar(' ');
					return 0;
				}

				i++;
			}

			if (command[0]) printf("Unknown command: \"%s\"\n", command);
			putchar('>');
			putchar(' ');
			pico_con_parser_cleanup(&command, &argc, &argv);
		}
		else if (ch == CH_ASCII_ETX)
		{
			input->idx = 0;
			printf(" Ctrl+C\n> ");
		}
		else if (ch == CH_ASCII_ESC)
		{
			input->state = INPUT_STATE_ESC;
		}
		else
		{
#ifdef DEBUG
			input->buffer[input->idx] = '\0';
			printf("\ngot: 0x%02X\n> %s", ch, input->buffer);
#else
			putchar('\a');
#endif
		}
	}

	return 0;
}

int pico_con_loop(struct pico_con_command *commands, size_t input_buffer_size)
{
	struct pico_con_input input;
	if (pico_con_input_init(&input, input_buffer_size) != 0)
	{
		return -1;
	}

	printf("> ");

	while (1)
	{
		int ch = getchar();
		if (pico_con_input_handle_char(&input, ch, commands) != 0)
		{
			pico_con_input_free(&input);
			return -1;
		}
	}

	pico_con_input_free(&input);
	return 0;
}
