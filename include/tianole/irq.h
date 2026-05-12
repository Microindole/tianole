#ifndef TIANOLE_IRQ_H
#define TIANOLE_IRQ_H

#include <stdint.h>

typedef void (*irq_handler_t)(uint8_t irq, void *data);

/**
 * irq_register() - Register a handler for an external IRQ line.
 * @irq: IRQ line number in the generic IRQ namespace.
 * @handler: Function called when the IRQ is dispatched.
 * @data: Opaque handler data passed back during dispatch.
 *
 * Return: 0 on success, negative value on failure.
 */
int irq_register(uint8_t irq, irq_handler_t handler, void *data);

#endif
