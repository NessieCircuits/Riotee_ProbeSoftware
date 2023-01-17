/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2021 Peter Lawrence
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
#include "sbw_protocol.h"

// UART0 for Rioteeprobe debug
// UART1 for Rioteeprobe to target device

#define UART_TASK_PRIO (tskIDLE_PRIORITY + 3)
#define TUD_TASK_PRIO (tskIDLE_PRIORITY + 2)
#define DAP_TASK_PRIO (tskIDLE_PRIORITY + 1)
#define SBW_TASK_PRIO (tskIDLE_PRIORITY + 1)

static TaskHandle_t dap_taskhandle, sbw_taskhandle, tud_taskhandle;
static MessageBufferHandle_t dap_req_buf, sbw_req_buf;

static SemaphoreHandle_t sbw_uarttx_mutex;
static SemaphoreHandle_t swd_uartrx_mutex;

static SemaphoreHandle_t programming_sem;

void programming_enable(void) {
  xSemaphoreGive(programming_sem);
  gpio_put(PROBE_PIN_TARGET_POWER, 1);
  gpio_put(PROBE_PIN_TRANS_CLOCK_EN, 1);
}

void programming_disable(void) {
  xSemaphoreTake(programming_sem, 0);
  if (uxSemaphoreGetCount(programming_sem) == 0) {
    gpio_put(PROBE_PIN_TARGET_POWER, 0);
    gpio_put(PROBE_PIN_TRANS_CLOCK_EN, 0);
  }
}

int swd_uartrx_take(void) {
  if (xSemaphoreTake(swd_uartrx_mutex, 0) != pdTRUE)
    return -1;
  gpio_put(PROBE_PIN_TRANS_SWD_UARTRX_EN, 1);
  return 0;
}

void swd_uartrx_give(void) {
  gpio_put(PROBE_PIN_TRANS_SWD_UARTRX_EN, 0);
  xSemaphoreGive(swd_uartrx_mutex);
}

int sbw_uarttx_take(void) {
  if (xSemaphoreTake(sbw_uarttx_mutex, 0) != pdTRUE)
    return -1;
  gpio_put(PROBE_PIN_TRANS_SBW_UARTTX_EN, 1);
  return 0;
}

void sbw_uarttx_give(void) {
  gpio_put(PROBE_PIN_TRANS_SBW_UARTTX_EN, 0);
  xSemaphoreGive(sbw_uarttx_mutex);
}

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

  DAP_Setup();

  while (1) {
    xMessageBufferReceive(dap_req_buf, req_buf, CFG_TUD_VENDOR_EPSIZE,
                          portMAX_DELAY);

    if (req_buf[0] == ID_DAP_Connect) {
      if (swd_uartrx_take() != 0) {
        rsp_buf[0] = DAP_ERROR;
        tud_vendor_write(rsp_buf, ((4U << 16) | 1U));
        continue;
      }
      programming_enable();
    } else if (req_buf[0] == ID_DAP_Disconnect) {
      swd_uartrx_give();
      programming_disable();
    }

    resp_len = DAP_ProcessCommand(req_buf, rsp_buf);
    tud_vendor_write(rsp_buf, resp_len);
  }
}

/* This callback is invoked when the RX endpoint has data */
void tud_cdc_rx_cb(uint8_t itf) {
  uint32_t req_buf[CFG_TUD_CDC_RX_BUFSIZE];

  if (itf != 1)
    return;

  int req_len = tud_cdc_n_read(itf, req_buf, sizeof(req_buf));
  xMessageBufferSend(sbw_req_buf, req_buf, req_len, portMAX_DELAY);
}

/* Processes requests and prepares response. Returns number of bytes in
 * response. */
int SBW_ProcessCommand(sbw_req_t *request, sbw_rsp_t *response) {

  switch (request->req_type) {
  case SBW_REQ_START:
    if (sbw_uarttx_take() != 0) {
      response->rc = SBW_RC_ERR_GENERIC;
      return 1;
    }
    programming_enable();
    response->rc = sbw_dev_connect();
    return 1;
  case SBW_REQ_STOP:
    response->rc = sbw_dev_disconnect();
    sbw_uarttx_give();
    programming_disable();
    return 1;
  case SBW_REQ_HALT:
    response->rc = sbw_dev_halt();
    return 1;
  case SBW_REQ_RELEASE:
    response->rc = sbw_dev_release();
    return 1;
  case SBW_REQ_WRITE:
    response->rc =
        sbw_dev_mem_write(request->address, request->data, request->len);
    return 1;
  case SBW_REQ_READ:
    response->rc =
        sbw_dev_mem_read(response->data, request->address, request->len);
    response->len = request->len;
    return 2 + (response->len * 2);
  default:
    response->rc = SBW_RC_ERR_UNKNOWN_REQ;
    return 1;
  }
}

/* Processes commands received via USB */
void sbw_thread(void *ptr) {
  sbw_req_t request;
  sbw_rsp_t response;
  sbw_pins_t pins = {.sbw_tck = PROBE_PIN_SBWCLK,
                     .sbw_tdio = PROBE_PIN_SBWIO,
                     .sbw_dir = PROBE_PIN_SBWDIR};

  sbw_dev_setup(&pins);

  do {
    /* Fetch command from buffer */
    xMessageBufferReceive(sbw_req_buf, &request, sizeof(sbw_req_t),
                          portMAX_DELAY);

    int rsp_len = SBW_ProcessCommand(&request, &response);

    /* Send the response packet */
    tud_cdc_n_write(1, &response, rsp_len);
    tud_cdc_n_write_flush(1);
  } while (1);
}

int main(void) {

  board_init();
  usb_serial_init();
  cdc_uart_init();
  tusb_init();

  gpio_init(PROBE_PIN_TARGET_POWER);
  gpio_set_dir(PROBE_PIN_TARGET_POWER, GPIO_OUT);
  gpio_init(PROBE_PIN_TRANS_CLOCK_EN);
  gpio_set_dir(PROBE_PIN_TRANS_CLOCK_EN, GPIO_OUT);
  gpio_init(PROBE_PIN_TRANS_SWD_UARTRX_EN);
  gpio_set_dir(PROBE_PIN_TRANS_SWD_UARTRX_EN, GPIO_OUT);
  gpio_init(PROBE_PIN_TRANS_SBW_UARTTX_EN);
  gpio_set_dir(PROBE_PIN_TRANS_SBW_UARTTX_EN, GPIO_OUT);

  printf("Welcome to Rioteeprobe!\n");

  dap_req_buf = xMessageBufferCreate(256);
  sbw_req_buf = xMessageBufferCreate(256);

  sbw_uarttx_mutex = xSemaphoreCreateMutex();
  swd_uartrx_mutex = xSemaphoreCreateMutex();

  programming_sem = xSemaphoreCreateCounting(2, 0);

  /* UART needs to preempt USB as if we don't, characters get lost */
  xTaskCreate(cdc_thread, "UART", configMINIMAL_STACK_SIZE, NULL,
              UART_TASK_PRIO, &uart_taskhandle);
  xTaskCreate(usb_thread, "TUD", configMINIMAL_STACK_SIZE, NULL, TUD_TASK_PRIO,
              &tud_taskhandle);

  xTaskCreate(dap_thread, "DAP", configMINIMAL_STACK_SIZE, NULL, DAP_TASK_PRIO,
              &dap_taskhandle);
  xTaskCreate(sbw_thread, "SBW", configMINIMAL_STACK_SIZE, NULL, SBW_TASK_PRIO,
              &sbw_taskhandle);

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
