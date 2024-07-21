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
pico_con_loop(size_t input_buffer_size);
```

#### Description

The pico\_con\_loop runs CLI loop processing user input. It uses at most input\_buffer\_size characters for a single command (excluding terminating zero charater).

#### Return Values

The function doesn't return until error occurs. In that case it returns -1.
