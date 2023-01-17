/*
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * This file provides routines to bring a device under JTAG control, to
 * interface with the TAP controller state machine and to read and write date
 * from the JTAG instruction and data registers via SBW. The implementation is
 * based on code provided by TI (slau320 and slaa754).
 */

#include <stdint.h>

#include "sbw_jtag.h"
#include "sbw_transport.h"

#include "FreeRTOS.h"
#include "task.h"

#include <pico/stdlib.h>

#define START_MAX_RETRY 5

void tap_reset(void) {
  /* Check fuse */
  for (int i = 6; i > 0; i--) {
    tmsh_tdih();
  }
  /* JTAG FSM is now in Test-Logic-Reset, move to Run/Test Idle */
  tmsl_tdih();
}

/**
 * Shifts data into and out of the JTAG Data and Instruction register.
 *
 * Assumes that the TAP controller is in Shift-DR or Shift-IR state and,
 * bit by bit, shifts data into and out of the register.
 *
 * @param format specifies length of the transfer
 * @param data data to be shifted into the register
 *
 * @returns data shifted out of the register
 *
 */
static uint32_t tap_shift(uint16_t format, uint32_t data) {
  uint32_t tdo_word = 0x00000000;
  uint32_t msb = 0x00000000;
  uint32_t i;

  bool tdo;

  switch (format) {
  case F_BYTE:
    msb = 0x00000080;
    break;
  case F_WORD:
    msb = 0x00008000;
    break;
  case F_ADDR:
    msb = 0x00080000;
    break;
  case F_LONG:
    msb = 0x80000000;
    break;
  default: // this is an unsupported format, function will just return 0
    return tdo_word;
  }
  // shift in bits
  for (i = format; i > 0; i--) {
    if (i == 1) // last bit requires TMS=1; TDO one bit before TDI
    {
      tdo = ((data & msb) == 0) ? tmsh_tdil_tdo_rd() : tmsh_tdih_tdo_rd();
    } else {
      tdo = ((data & msb) == 0) ? tmsl_tdil_tdo_rd() : tmsl_tdih_tdo_rd();
    }
    data <<= 1;
    if (tdo)
      tdo_word++;
    if (i > 1)
      tdo_word <<= 1; // TDO could be any port pin
  }
  tmsh_tdih(); // update IR
  if (get_tclk()) {
    tmsl_tdih();
  } else {
    tmsl_tdil();
  }

  // de-scramble bits on a 20bit shift
  if (format == F_ADDR) {
    tdo_word = ((tdo_word << 16) + (tdo_word >> 4)) & 0x000FFFFF;
  }

  return tdo_word;
}

uint32_t tap_ir_shift(uint8_t instruction) {
  // JTAG FSM state = Run-Test/Idle
  if (get_tclk()) {
    tmsh_tdih();
  } else {
    tmsh_tdil();
  }
  // JTAG FSM state = Select DR-Scan
  tmsh_tdih();

  // JTAG FSM state = Select IR-Scan
  tmsl_tdih();
  // JTAG FSM state = Capture-IR
  tmsl_tdih();
  // JTAG FSM state = Shift-IR, Shift in TDI (8-bit)
  return (tap_shift(F_BYTE, instruction));
  // JTAG FSM state = Run-Test/Idle
}

uint16_t tap_dr_shift16(uint16_t data) {
  // JTAG FSM state = Run-Test/Idle
  if (get_tclk()) {
    tmsh_tdih();
  } else {
    tmsh_tdil();
  }
  // JTAG FSM state = Select DR-Scan
  tmsl_tdih();
  // JTAG FSM state = Capture-DR
  tmsl_tdih();

  // JTAG FSM state = Shift-DR, Shift in TDI (16-bit)
  return (tap_shift(F_WORD, data));
  // JTAG FSM state = Run-Test/Idle
}

uint32_t tap_dr_shift20(uint32_t address) {
  // JTAG FSM state = Run-Test/Idle
  if (get_tclk()) {
    tmsh_tdih();
  } else {
    tmsh_tdil();
  }
  // JTAG FSM state = Select DR-Scan
  tmsl_tdih();
  // JTAG FSM state = Capture-DR
  tmsl_tdih();

  // JTAG FSM state = Shift-DR, Shift in TDI (16-bit)
  return (tap_shift(F_ADDR, address));
  // JTAG FSM state = Run-Test/Idle
}

int sbw_jtag_write_jmb_in16(uint16_t data) {
  uint16_t sJMBINCTL;
  uint16_t sJMBIN0;
  uint32_t Timeout = 0;
  sJMBIN0 = (uint16_t)(data & 0x0000FFFF);
  sJMBINCTL = INREQ;

  tap_ir_shift(IR_JMB_EXCHANGE);
  do {
    Timeout++;
    if (Timeout >= 3000) {
      return SBW_ERR_GENERIC;
    }
  } while (!(tap_dr_shift16(0x0000) & IN0RDY) && Timeout < 3000);
  if (Timeout < 3000) {
    tap_dr_shift16(sJMBINCTL);
    tap_dr_shift16(sJMBIN0);
  }
  return SBW_ERR_NONE;
}

/**
 * Enables JTAG access over SBW
 *
 * @see SLAU320AJ 2.3.1.1
 */
static int sbw_entry_sequence() {
  set_sbwtck(0);
  sleep_us(800); // delay min 800us - clr SBW controller
  set_sbwtck(1);
  sleep_us(50);

  // SpyBiWire entry sequence
  // Reset Test logic
  set_sbwtdio(0); // put device in normal operation: Reset = 0
  set_sbwtck(0);  // TEST pin = 0
  sleep_ms(1);    // wait 1ms (minimum: 100us)

  // SpyBiWire entry sequence
  set_sbwtdio(1); // Reset = 1
  sleep_us(5);
  set_sbwtck(1); // TEST pin = 1
  sleep_us(5);
  // initial 1 PIN_SBWTCKs to enter sbw-mode
  set_sbwtck(0);
  sleep_us(5);
  set_sbwtck(1);
  sleep_us(5);

  return SBW_ERR_NONE;
}

int sbw_jtag_sync(void) {
  int i = 0;

  tap_ir_shift(IR_CNTRL_SIG_16BIT);
  tap_dr_shift16(0x1501); // Set device into JTAG mode + read
  if ((tap_ir_shift(IR_CNTRL_SIG_CAPTURE) != JTAG_ID91) &&
      (tap_ir_shift(IR_CNTRL_SIG_CAPTURE) != JTAG_ID99) &&
      (tap_ir_shift(IR_CNTRL_SIG_CAPTURE) != JTAG_ID98)) {
    return SBW_ERR_GENERIC;
  }
  // wait for sync
  while (!(tap_dr_shift16(0) & 0x0200) && i < 50) {
    i++;
    sleep_us(5);
  };

  // continues if sync was successful
  if (i >= 50) {
    return SBW_ERR_GENERIC;
  }
  return SBW_ERR_NONE;
}

int sbw_jtag_connect(void) {

  int retries = START_MAX_RETRY;
  do {
    sbw_transport_connect();
    sleep_ms(15);
    sbw_entry_sequence();
    tap_reset();
    uint16_t jtag_id = (uint16_t)tap_ir_shift(IR_CNTRL_SIG_CAPTURE);
    if ((jtag_id == JTAG_ID91) || (jtag_id == JTAG_ID99) ||
        (jtag_id == JTAG_ID98))
      return SBW_ERR_NONE;
    sleep_us(500);
    sbw_transport_disconnect();
  } while (--retries > 0);
  return SBW_ERR_GENERIC;
}

int sbw_jtag_disconnect(void) {
  int rc = sbw_transport_disconnect();
  sleep_ms(15);
  return rc;
}

int sbw_jtag_setup(sbw_pins_t *sbw_pins) {
  return sbw_transport_setup(sbw_pins);
}