#ifndef TIANOLE_IRQ_H
#define TIANOLE_IRQ_H

#include <stdint.h>

typedef void (*irq_handler_t)(uint8_t irq, void *data);

int irq_register(uint8_t irq, irq_handler_t handler, void *data);

#endif
