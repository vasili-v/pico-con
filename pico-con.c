#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

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

static void pico_con_input_handle_char(struct pico_con_input *input, int ch)
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
			printf("\ngot: \"%s\"\n> ", input->buffer);
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
			printf("\ngot: 0x%02X\n> %s", ch, input_buffer);
#else
			putchar('\a');
#endif
		}
	}
}

int pico_con_loop(size_t input_buffer_size)
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
		pico_con_input_handle_char(&input, ch);
	}

	pico_con_input_free(&input);
	return 0;
}
