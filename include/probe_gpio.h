#ifndef __PROBE_GPIO_H_
#define __PROBE_GPIO_H_

#include <stdint.h>

typedef uint16_t probe_io_state_t;

int probe_ioget(uint16_t *dst, unsigned int pin_no);
int probe_ioset(unsigned int pin_no, probe_io_state_t state);

#endif /* __PROBE_GPIO_H_ */
