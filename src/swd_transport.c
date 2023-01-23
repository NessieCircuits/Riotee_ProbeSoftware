/*
 * Copyright (c) 2013-2022 ARM Limited. All rights reserved.
 * Copyright (c) 2022 Raspberry Pi Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is a shim between the SW_DP functions and the PIO
 * implementation used for Picoprobe. Instead of calling bitbash functions,
 * hand off the bit sequences to a SM for asynchronous completion.
 */

#include <stdio.h>

#include "DAP_config.h"

#include "DAP.h"
#include "delay.h"
#include "swd_transport.h"

/* SWD write bit routine */
static inline void iow(unsigned int val) {
  gpio_put(PROBE_PIN_SWDIO, val);
  gpio_put(PROBE_PIN_SWDCLK, 0);
  __delay_cycles(DAP_Data.clock_delay / 2);
  gpio_put(PROBE_PIN_SWDCLK, 1);
  __delay_cycles(DAP_Data.clock_delay / 2);
}

/* SWD read bit routine */
static inline int ior(void) {
  gpio_put(PROBE_PIN_SWDCLK, 0);
  __delay_cycles(DAP_Data.clock_delay / 2);
  int ret = gpio_get(PROBE_PIN_SWDIO);
  gpio_put(PROBE_PIN_SWDCLK, 1);
  __delay_cycles(DAP_Data.clock_delay / 2);

  return ret;
}

static void io_write_bits(uint bit_count, uint32_t data_byte) {
  for (unsigned int i = 0; i < bit_count; i++) {
    iow((data_byte >> i) & 1UL);
  }
}

static uint32_t io_read_bits(uint bit_count) {
  uint32_t data_shifted = 0x0;
  for (unsigned int i = 0; i < bit_count; i++) {
    if (ior())
      data_shifted |= 1UL << i;
  }
  return data_shifted;
}

// Generate SWJ Sequence
//   count:  sequence bit count
//   data:   pointer to sequence bit data
//   return: none
#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
void SWJ_Sequence(uint32_t count, const uint8_t *data) {
  uint32_t bits;
  uint32_t n;

  n = count;
  while (n > 0) {
    if (n > 8)
      bits = 8;
    else
      bits = n;
    io_write_bits(bits, *data++);
    n -= bits;
  }
}
#endif

// Generate SWD Sequence
//   info:   sequence information
//   swdo:   pointer to SWDIO generated data
//   swdi:   pointer to SWDIO captured data
//   return: none
#if (DAP_SWD != 0)
void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi) {
  uint32_t bits;
  uint32_t n;

  n = info & SWD_SEQUENCE_CLK;
  if (n == 0U) {
    n = 64U;
  }
  bits = n;
  if (info & SWD_SEQUENCE_DIN) {
    while (n > 0) {
      if (n > 8)
        bits = 8;
      else
        bits = n;
      *swdi++ = io_read_bits(bits);
      n -= bits;
    }
  } else {
    while (n > 0) {
      if (n > 8)
        bits = 8;
      else
        bits = n;
      io_write_bits(bits, *swdo++);
      n -= bits;
    }
  }
}
#endif

#if (DAP_SWD != 0)
// SWD Transfer I/O
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
uint8_t SWD_Transfer(uint32_t request, uint32_t *data) {
  uint8_t prq = 0;
  uint8_t ack;
  uint8_t bit;
  uint32_t val = 0;
  uint32_t parity = 0;
  uint32_t n;

  /* Generate the request packet */
  prq |= (1 << 0); /* Start Bit */
  for (n = 1; n < 5; n++) {
    bit = (request >> (n - 1)) & 0x1;
    prq |= bit << n;
    parity += bit;
  }
  prq |= (parity & 0x1) << 5; /* Parity Bit */
  prq |= (0 << 6);            /* Stop Bit */
  prq |= (1 << 7);            /* Park bit */
  io_write_bits(8, prq);

  /* Turnaround (ignore read bits) */
  swd_transport_read_mode();

  ack = io_read_bits(DAP_Data.swd_conf.turnaround + 3);
  ack >>= DAP_Data.swd_conf.turnaround;

  if (ack == DAP_TRANSFER_OK) {
    /* Data transfer phase */
    if (request & DAP_TRANSFER_RnW) {
      /* Read RDATA[0:31] - note io_read shifts to LSBs */
      val = io_read_bits(32);
      bit = io_read_bits(1);
      parity = __builtin_popcount(val);
      if ((parity ^ bit) & 1U) {
        /* Parity error */
        ack = DAP_TRANSFER_ERROR;
      }
      if (data)
        *data = val;
      /* Turnaround for line idle */
      io_read_bits(DAP_Data.swd_conf.turnaround);
      swd_transport_write_mode();
    } else {
      /* Turnaround for write */
      io_read_bits(DAP_Data.swd_conf.turnaround);
      swd_transport_write_mode();

      /* Write WDATA[0:31] */
      val = *data;
      io_write_bits(32, val);
      parity = __builtin_popcount(val);
      /* Write Parity Bit */
      io_write_bits(1, parity & 0x1);
    }
    /* Capture Timestamp */
    if (request & DAP_TRANSFER_TIMESTAMP) {
      DAP_Data.timestamp = time_us_32();
    }

    /* Idle cycles - drive 0 for N clocks */
    if (DAP_Data.transfer.idle_cycles) {
      for (n = DAP_Data.transfer.idle_cycles; n;) {
        if (n > 32) {
          io_write_bits(32, 0);
          n -= 32;
        } else {
          io_write_bits(n, 0);
          n -= n;
        }
      }
    }
    return ((uint8_t)ack);
  }

  if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {
    if (DAP_Data.swd_conf.data_phase && ((request & DAP_TRANSFER_RnW) != 0U)) {
      /* Dummy Read RDATA[0:31] + Parity */
      io_read_bits(33);
    }
    io_read_bits(DAP_Data.swd_conf.turnaround);
    swd_transport_write_mode();
    if (DAP_Data.swd_conf.data_phase && ((request & DAP_TRANSFER_RnW) == 0U)) {
      /* Dummy Write WDATA[0:31] + Parity */
      io_write_bits(32, 0);
      io_write_bits(1, 0);
    }
    return ((uint8_t)ack);
  }

  /* Protocol error */
  n = DAP_Data.swd_conf.turnaround + 32U + 1U;
  /* Back off data phase */
  io_read_bits(n);
  swd_transport_write_mode();
  return ((uint8_t)ack);
}

void swd_transport_read_mode(void) {
  gpio_put(PROBE_PIN_PROG_DIR, 0);
  gpio_set_dir(PROBE_PIN_SWDIO, GPIO_IN);
}

void swd_transport_write_mode(void) {
  gpio_put(PROBE_PIN_PROG_DIR, 1);
  gpio_set_dir(PROBE_PIN_SWDIO, GPIO_OUT);
}

void swd_transport_init() {

  gpio_init(PROBE_PIN_SWDCLK);
  gpio_set_dir(PROBE_PIN_SWDCLK, GPIO_IN);
  gpio_disable_pulls(PROBE_PIN_SWDCLK);

  gpio_init(PROBE_PIN_SWDIO);
  gpio_set_dir(PROBE_PIN_SWDIO, GPIO_IN);
  gpio_disable_pulls(PROBE_PIN_SWDIO);
}

void swd_transport_connect() {

  gpio_set_dir(PROBE_PIN_SWDCLK, GPIO_OUT);

  // Make sure SWDIO has a pullup on it. Idle state is high
  gpio_pull_up(PROBE_PIN_SWDIO);
  gpio_put(PROBE_PIN_PROG_DIR, 1);

  swd_transport_write_mode();
}

void swd_transport_disconnect() {
  gpio_set_dir(PROBE_PIN_SWDCLK, GPIO_IN);
  gpio_disable_pulls(PROBE_PIN_SWDIO);

  gpio_set_dir(PROBE_PIN_SWDIO, GPIO_IN);
  gpio_disable_pulls(PROBE_PIN_SWDCLK);

  gpio_put(PROBE_PIN_PROG_DIR, 0);
}

#endif /* (DAP_SWD != 0) */
