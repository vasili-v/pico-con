#ifndef PICO_CON_H
#define PICO_CON_H

#ifndef PICO_CON_PARSER_ARG_CAPACITY_MAX
#define PICO_CON_PARSER_ARG_CAPACITY_MAX 256
#endif

#define PICO_CON_COMMAND_SUCCESS  0
#define PICO_CON_COMMAND_ABORT   -1

#define MODE_BATCH          0
#define MODE_HUMAN_READABLE 1

typedef int (*pico_con_commnad_handler_t)(size_t argc, char *argv[]);

struct pico_con_command
{
	const char                 *name;
	pico_con_commnad_handler_t handler;
};

int pico_con_loop(struct pico_con_command *commands, size_t input_buffer_size, int mode);

#endif //PICO_CON_H
