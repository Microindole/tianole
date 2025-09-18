#ifndef SHELL_H
#define SHELL_H

#include "common.h"

void init_shell();
void shell_handle_key(uint16_t keycode);
void process_command(char *input);

#endif