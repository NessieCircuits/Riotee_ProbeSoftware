#include <pico/stdlib.h>

#include "probe_gpio.h"
#include "rioteeprobe_config.h"
#include "sbw_protocol.h"

#ifdef BOARD_RIOTEE_PROBE

unsigned int probe_gpios[PROBE_GPIO_NUM] = {PROBE_PIN_GPIO0, PROBE_PIN_GPIO1,
                                            PROBE_PIN_GPIO2, PROBE_PIN_GPIO3};

int probe_ioget(uint16_t *dst, unsigned int pin_no) {
  *dst = (uint16_t)gpio_get(probe_gpios[pin_no]);
  return SBW_RC_OK;
}

int probe_ioset(unsigned int pin_no, probe_io_state_t state) {
  switch (state) {
  case IOSET_IN:
    gpio_set_dir(probe_gpios[pin_no], GPIO_IN);
    return SBW_RC_OK;
  case IOSET_OUT_HIGH:
    gpio_put(probe_gpios[pin_no], 1);
    gpio_set_dir(probe_gpios[pin_no], GPIO_OUT);
    return SBW_RC_OK;
  case IOSET_OUT_LOW:
    gpio_put(probe_gpios[pin_no], 0);
    gpio_set_dir(probe_gpios[pin_no], GPIO_OUT);
    return SBW_RC_OK;
  default:
    return SBW_RC_ERR_GENERIC;
  }
}

#else

int probe_ioget(uint16_t *dst, unsigned int pin_no) {
  return SBW_RC_ERR_UNSUPPORTED;
}

int probe_ioset(unsigned int pin_no, probe_io_state_t state) {
  return SBW_RC_ERR_UNSUPPORTED;
}
#endif