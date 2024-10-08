# pico-con library

## About

The pico-con is a library to create simple CLI for Raspberry PI Pico.

## Installation

- Create a submodule in your project directory `git submodule add git@github.com:vasili-v/pico-con.git`
- Add `include("pico-con/pico-con.cmake")` to your CMakeLists.txt
- List the library in `target_link_libraries` command: `target_link_libraries(<target> ... pico_con ...)`

## API

### CLI loop

#### Name

__pico_con_loop__

#### Synopsis

```C
#include "pico-con.h"

int
pico_con_loop(struct pico_con_command *commands, size_t input_buffer_size, int mode);
```

#### Description

The pico\_con\_loop runs CLI loop processing user input. A set of CLI commands should be passed as an array of pico\_con\_command structures:
```C
typedef int (*pico_con_commnad_handler_t)(size_t argc, char *argv[]);

struct pico_con_command
{
    const char                 *name;
    pico_con_commnad_handler_t handler;
};
```
where the name field is a case-sensetive command name and the handler is a pointer to a command function. When the CLI gets a command, it calls a respective handler with a number of arguments in argc and their values in argv. The handler should return zero (PICO\_CON\_COMMAND\_SUCCESS) to continue running pico\_con\_loop or -1 (PICO\_CON\_COMMAND\_ABORT) to stop it. The last entry in the array must have NULL handler. The pico\_con\_loop uses at most input\_buffer\_size characters for a single command (excluding terminating zero charater).

The function supports two modes - batch mode (MODE\_BATCH) and human readable mode (MODE\_HUMAN\_READABLE). The latest is more human friendly - echoes input to serial ouput, uses "> " as prompt, and prints "human readable" error messages, while the first one doesn't echo input, uses ASCII ENQ (0x05) as prompt, sends ASCII NAK (0x15) in case of error and ASCII ACK (0x06) if command has been recognized and its handler is about to be called.

#### Return Values

The function doesn't return until error occurs. In that case it returns -1.
