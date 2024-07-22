#ifndef PICO_CON_H
#define PICO_CON_H

#define PICO_CON_COMMAND_SUCCESS  0
#define PICO_CON_COMMAND_ABORT   -1

typedef int (*pico_con_commnad_handler_t)(void);

struct pico_con_command
{
	const char                 *name;
	pico_con_commnad_handler_t handler;
};

int pico_con_loop(struct pico_con_command *commands, size_t input_buffer_size);

#endif //PICO_CON_H
