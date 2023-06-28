/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2021 Peter Lawrence
 * Copyright (c) 2023 Kai Geissdoerfer (Nessie Circuits)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "DAP.h"
#include "cdc_uart.h"
#include "get_serial.h"
#include "rioteeprobe_config.h"
#include "sbw_device.h"

// UART0 for Rioteeprobe debug
// UART1 for Rioteeprobe to target device

#define UART_TASK_PRIO (tskIDLE_PRIORITY + 3)
#define TUD_TASK_PRIO (tskIDLE_PRIORITY + 2)
#define DAP_TASK_PRIO (tskIDLE_PRIORITY + 1)

static TaskHandle_t dap_taskhandle, tud_taskhandle;
static MessageBufferHandle_t dap_req_buf;

int programming_enable(void);

int programming_disable(void);

void usb_thread(void *ptr) {
  do {
    tud_task();
    // Trivial delay to save power
    vTaskDelay(1);
  } while (1);
}

/* Gets DAP requests from USB vendor interface and queues them for processing
 */
void tud_vendor_rx_cb(uint8_t itf) {
  uint8_t req_buf[CFG_TUD_VENDOR_EPSIZE];

  uint32_t req_len = tud_vendor_n_read(itf, req_buf, sizeof(req_buf));
  xMessageBufferSend(dap_req_buf, req_buf, req_len, portMAX_DELAY);
}

/* Processes DAP requests */
void dap_thread(void *ptr) {
  uint32_t resp_len;
  uint8_t req_buf[CFG_TUD_VENDOR_EPSIZE];
  uint8_t rsp_buf[CFG_TUD_VENDOR_EPSIZE];

  sbw_pins_t pins = {.sbw_tck = PROBE_PIN_SBWCLK,
                     .sbw_tdio = PROBE_PIN_SBWIO,
                     .sbw_dir = PROBE_PIN_PROG_DIR};

  DAP_Setup();
  sbw_dev_setup(&pins);

  while (1) {
    xMessageBufferReceive(dap_req_buf, req_buf, CFG_TUD_VENDOR_EPSIZE,
                          portMAX_DELAY);

    if (req_buf[0] == ID_DAP_Connect) {
      if (programming_enable() != 0) {
        rsp_buf[0] = DAP_ERROR;
        tud_vendor_write(rsp_buf, ((4U << 16) | 1U));
        continue;
      }
    } else if (req_buf[0] == ID_DAP_Disconnect) {
      programming_disable();
      gpio_put(PROBE_PIN_LED, 0);
    } else {
      gpio_put(PROBE_PIN_LED, !gpio_get(PROBE_PIN_LED));
    }

    resp_len = DAP_ProcessCommand(req_buf, rsp_buf);
    tud_vendor_write(rsp_buf, resp_len);
    tud_vendor_flush();
  }
}

int main(void) {

  board_init();
  usb_serial_init();
  cdc_uart_init();
  tusb_init();

  gpio_init(PROBE_PIN_LED);
  gpio_set_dir(PROBE_PIN_LED, GPIO_OUT);
  gpio_put(PROBE_PIN_LED, 0);

  /* Enables/disables constant voltage supply for target */
  gpio_init(PROBE_PIN_TARGET_POWER);
  gpio_set_dir(PROBE_PIN_TARGET_POWER, GPIO_OUT);

  /* Enables/disables supply of programming level translators */
  gpio_init(PROBE_PIN_TRANS_PROG_EN);
  gpio_set_dir(PROBE_PIN_TRANS_PROG_EN, GPIO_OUT);

  /* Enables/disables supply of UARTRX level translator */
  gpio_init(PROBE_PIN_TRANS_UARTRX_EN);
  gpio_set_dir(PROBE_PIN_TRANS_UARTRX_EN, GPIO_OUT);
  gpio_put(PROBE_PIN_TRANS_UARTRX_EN, false);

  gpio_init(PROBE_PIN_TRANS_UARTRX_DIR);
  gpio_set_dir(PROBE_PIN_TRANS_UARTRX_DIR, GPIO_OUT);
  gpio_put(PROBE_PIN_TRANS_UARTRX_DIR, false);

  /* Enables/disables supply of UARTTX level translator */
  gpio_init(PROBE_PIN_TRANS_UARTTX_EN);
  gpio_set_dir(PROBE_PIN_TRANS_UARTTX_EN, GPIO_OUT);
  gpio_put(PROBE_PIN_TRANS_UARTTX_EN, 1);

  /* Controls direction of programming level translator */
  gpio_init(PROBE_PIN_PROG_DIR);
  gpio_set_dir(PROBE_PIN_PROG_DIR, GPIO_OUT);
  gpio_put(PROBE_PIN_PROG_DIR, false);

#ifdef BOARD_RIOTEE_PROBE
  gpio_init(PROBE_PIN_GPIO0);
  gpio_init(PROBE_PIN_GPIO1);
  gpio_init(PROBE_PIN_GPIO2);
  gpio_init(PROBE_PIN_GPIO3);
#endif

#ifdef BOARD_RIOTEE_BOARD
  gpio_init(PROBE_PIN_BYPASS_ENABLE);
  gpio_set_dir(PROBE_PIN_BYPASS_ENABLE, GPIO_OUT);
  gpio_put(PROBE_PIN_BYPASS_ENABLE, false);

#endif

  printf("Welcome to Rioteeprobe!\n");

  dap_req_buf = xMessageBufferCreate(256);

  /* UART needs to preempt USB as if we don't, characters get lost */
  xTaskCreate(cdc_thread, "UART", configMINIMAL_STACK_SIZE, NULL,
              UART_TASK_PRIO, &uart_taskhandle);
  xTaskCreate(usb_thread, "TUD", configMINIMAL_STACK_SIZE, NULL, TUD_TASK_PRIO,
              &tud_taskhandle);
  xTaskCreate(dap_thread, "DAP", configMINIMAL_STACK_SIZE, NULL, DAP_TASK_PRIO,
              &dap_taskhandle);

  vTaskStartScheduler();

  return 0;
}

extern uint8_t const desc_ms_os_20[];

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                tusb_control_request_t const *request) {
  // nothing to with DATA & ACK stage
  if (stage != CONTROL_STAGE_SETUP)
    return true;

  switch (request->bmRequestType_bit.type) {
  case TUSB_REQ_TYPE_VENDOR:
    switch (request->bRequest) {
    case 1:
      if (request->wIndex == 7) {
        // Get Microsoft OS 2.0 compatible descriptor
        uint16_t total_len;
        memcpy(&total_len, desc_ms_os_20 + 8, 2);

        return tud_control_xfer(rhport, request, (void *)desc_ms_os_20,
                                total_len);
      } else {
        return false;
      }

    default:
      break;
    }
    break;
  default:
    break;
  }

  // stall unknown request
  return false;
}

void vApplicationTickHook(void){};

void vApplicationStackOverflowHook(TaskHandle_t Task, char *pcTaskName) {
  panic("stack overflow (not the helpful kind) for %s\n", *pcTaskName);
}

void vApplicationMallocFailedHook(void) { panic("Malloc Failed\n"); };
