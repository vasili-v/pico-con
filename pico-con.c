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

#define PARSER_STATE_PRE_COMMAND 0
#define PARSER_STATE_COMMAND     1

static int pico_con_parse(struct pico_con_input *input, char **command)
{
#ifdef DEBUG
	printf("\ninput: \"%s\"\n", input->buffer);
#else
	putchar('\n');
#endif

	int parser_state = PARSER_STATE_PRE_COMMAND;
	size_t i = 0;
	size_t start = 0;
	char *token = NULL;
	while (i <= input->buffer_size && input->buffer[i] != '\0')
	{
		switch (parser_state)
		{
		case PARSER_STATE_COMMAND:
			if (isspace(input->buffer[i]))
			{
				size_t size = i - start;
				token = malloc(size + 1);
				if (!token)
				{
					return -1;
				}

				memcpy(token, input->buffer+start, size);
				token[size] = '\0';

				*command = token;

				return 0;
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
		size_t size = i - start;
		token = malloc(size + 1);
		if (!token)
		{
			return -1;
		}

		memcpy(token, input->buffer+start, size);
		token[size] = '\0';

		*command = token;
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
			if (pico_con_parse(input, &command) != 0)
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
					int r = commands[i].handler();

					free(command);
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

			printf("Unknown command: \"%s\"\n> ", command);
			free(command);
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
