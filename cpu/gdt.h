#ifndef GDT_H
#define GDT_H

#include "common.h"

void init_gdt_tss();
void tss_set_stack(uint32_t esp0);

#endif