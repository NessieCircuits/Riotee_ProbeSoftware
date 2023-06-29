
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"

#include "DAP.h"
#include "cdc_uart.h"
#include "get_serial.h"
#include "rioteeprobe_config.h"
#include "sbw_device.h"
#include "sbw_protocol.h"

/* Used to identify FW version. Updated with bumpversion. */
const char version_string[] = "0.0.5";

#define ID_DAP_VENDOR_VERSION ID_DAP_Vendor0
#define ID_DAP_VENDOR_POWER ID_DAP_Vendor1
#define ID_DAP_VENDOR_SBW_CONNECT ID_DAP_Vendor2
#define ID_DAP_VENDOR_SBW_DISCONNECT ID_DAP_Vendor3
#define ID_DAP_VENDOR_SBW_RESET ID_DAP_Vendor4
#define ID_DAP_VENDOR_SBW_HALT ID_DAP_Vendor5
#define ID_DAP_VENDOR_SBW_RESUME ID_DAP_Vendor6
#define ID_DAP_VENDOR_SBW_READ ID_DAP_Vendor7
#define ID_DAP_VENDOR_SBW_WRITE ID_DAP_Vendor8
#define ID_DAP_VENDOR_GPIO_SET ID_DAP_Vendor9
#define ID_DAP_VENDOR_GPIO_GET ID_DAP_Vendor10
#define ID_DAP_VENDOR_BYPASS ID_DAP_Vendor11

static int power_access_cnt = 0;
static int prog_access_cnt = 0;

#ifdef BOARD_RIOTEE_PROBE

unsigned int probe_gpios[PROBE_GPIO_NUM] = {PROBE_PIN_GPIO0, PROBE_PIN_GPIO1,
                                            PROBE_PIN_GPIO2, PROBE_PIN_GPIO3};

int probe_ioget(uint8_t *dst, unsigned int pin_no) {
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

int probe_ioget(uint8_t *dst, unsigned int pin_no) {
  return SBW_RC_ERR_UNSUPPORTED;
}

int probe_ioset(unsigned int pin_no, probe_io_state_t state) {
  return SBW_RC_ERR_UNSUPPORTED;
}
#endif

void target_power_enable(void) {
  if (power_access_cnt++ == 0)
    gpio_put(PROBE_PIN_TARGET_POWER, 1);
}

int target_power_disable(void) {
  /* Only switch off power if we're the last one using it*/
  if (power_access_cnt <= 0)
    return -1;
  if (--power_access_cnt == 0)
    gpio_put(PROBE_PIN_TARGET_POWER, 0);
  return 0;
}

int bypass_enable(void) {
#ifdef BOARD_RIOTEE_PROBE
  return -1;
#else
  gpio_put(PROBE_PIN_BYPASS_ENABLE, true);
  return 0;
#endif
}

int bypass_disable(void) {
#ifdef BOARD_RIOTEE_PROBE
  return -1;
#else
  gpio_put(PROBE_PIN_BYPASS_ENABLE, false);
  return 0;
#endif
}

int programming_enable(void) {
  target_power_enable();
  if (prog_access_cnt++ == 0)
    gpio_put(PROBE_PIN_TRANS_PROG_EN, 1);

  return 0;
}

int programming_disable(void) {

  if (prog_access_cnt <= 0)
    return -1;
  target_power_disable();
  if (--prog_access_cnt == 0)
    gpio_put(PROBE_PIN_TRANS_PROG_EN, 0);
  return 0;
}

//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
// First byte in response is request
uint32_t DAP_ProcessVendorCommand(const uint8_t *request, uint8_t *response) {
  uint32_t addr;
  /* Reply with same ID as request */
  response[0] = request[0];
  /* Reply with one byte encoding return code */
  response[1] = DAP_OK;
  uint32_t rsp_len = 2;

  uint32_t req_len = 1;

  switch (request[0]) {
  case ID_DAP_VENDOR_VERSION:
    memcpy(response + 2, version_string, sizeof(version_string));
    rsp_len += sizeof(version_string);
    break;
  case ID_DAP_VENDOR_GPIO_SET:
    probe_ioset(request[1], request[2]);
    break;
  case ID_DAP_VENDOR_GPIO_GET:
    probe_ioget(&response[2], request[1]);
    rsp_len += 1;
    break;
  case ID_DAP_VENDOR_POWER:
    if (request[1] == TARGET_POWER_ON) {
      target_power_enable();
    } else if (request[1] == TARGET_POWER_OFF) {
      if (target_power_disable() < 0)
        response[1] = DAP_ERROR;
    }
    break;
  case ID_DAP_VENDOR_BYPASS:
    if (request[1] == BYPASS_ON) {
      bypass_enable();
    } else if (request[1] == BYPASS_OFF) {
      if (bypass_disable() < 0)
        response[1] = DAP_ERROR;
    }
    break;
  case ID_DAP_VENDOR_SBW_RESUME:
    if (sbw_dev_release() < 0)
      response[1] = DAP_ERROR;
    break;
  case ID_DAP_VENDOR_SBW_RESET:
    if (sbw_dev_reset() < 0)
      response[1] = DAP_ERROR;
    break;
  case ID_DAP_VENDOR_SBW_HALT:
    if (sbw_dev_halt() < 0)
      response[1] = DAP_ERROR;
    break;
  case ID_DAP_VENDOR_SBW_CONNECT:
    if (programming_enable() < 0)
      response[1] = DAP_ERROR;
    if (sbw_dev_connect() < 0)
      response[1] = DAP_ERROR;
    break;
  case ID_DAP_VENDOR_SBW_DISCONNECT:
    if (sbw_dev_disconnect() < 0)
      response[1] = DAP_ERROR;
    if (programming_disable() < 0)
      response[1] = DAP_ERROR;
    break;
  case ID_DAP_VENDOR_SBW_READ:
    /* Request: [Request (1B) | Address (4B) | NWords (1B)]*/
    /* Response: [Request (1B) | ReturnCode (1B) | Data (NWords*2B)]*/
    memcpy(&addr, &request[1], sizeof(addr));

    uint8_t n_words_r = request[5];
    if (sbw_dev_mem_read((uint16_t *)&response[2], addr, n_words_r) == 0) {
      rsp_len += n_words_r * 2;
    } else
      response[1] = DAP_ERROR;
    break;
  case ID_DAP_VENDOR_SBW_WRITE:
    /* [Request (1B) | Address (4B) | NWords (1B) | Data (NWords * 2B)]*/
    memcpy(&addr, &request[1], sizeof(addr));

    uint8_t n_words_w = request[5];
    if (sbw_dev_mem_write(addr, (uint16_t *)&request[6], n_words_w) < 0)
      response[1] = DAP_ERROR;
    break;
  default:
    response[0] = ID_DAP_Invalid;
    response[1] = DAP_ERROR;
  }

  return ((req_len << 16) + rsp_len);
}
