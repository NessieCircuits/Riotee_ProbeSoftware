#include "sbw_device.h"
#include "sbw_jtag.h"

#define SAFE_FRAM_PC 0x0004
#define FR4xx_LOCKREGISTER 0x160

/**
 * Checks if device is protected from JTAG access
 *
 * @returns true if locked, else false
 *
 * @see SLAU320AJ 2.4.3
 */
static bool is_lock_key_programmed(void) {
  uint16_t i;

  for (i = 3; i > 0; i--) //  First trial could be wrong
  {
    tap_ir_shift(IR_CNTRL_SIG_CAPTURE);
    if (tap_dr_shift16(0xAAAA) == 0x5555) {
      return true;
    }
  }
  return false;
}

/**
 * Reads one word from a given address in memory
 *
 * @param dst pointer to destination buffer
 * @param addr address of data to be read
 *
 * @returns Data from device
 */
static int mem_read_word(uint16_t *dst, uint32_t addr) {

  //  Check Init State at the beginning
  tap_ir_shift(IR_CNTRL_SIG_CAPTURE);
  if (!(tap_dr_shift16(0) & 0x0301))
    return SBW_ERR_GENERIC;

  // Read Memory
  clr_tclk_sbw();
  /* enables setting of the complete JTAG control signal register with the
   * next 16-bit JTAG data access.*/
  tap_ir_shift(IR_CNTRL_SIG_16BIT);

  tap_dr_shift16(0x0501); // Set uint16_t read

  tap_ir_shift(IR_ADDR_16BIT);
  tap_dr_shift20(addr); // Set address
  tap_ir_shift(IR_DATA_TO_ADDR);
  set_tclk_sbw();
  clr_tclk_sbw();
  *dst = tap_dr_shift16(0x0000); // Shift out 16 bits

  set_tclk_sbw();
  // one or more cycle, so CPU is driving correct MAB
  clr_tclk_sbw();
  set_tclk_sbw();
  // Processor is now again in Init State

  return SBW_ERR_NONE;
}

/**
 * Writes one uint16_t at a given address
 *
 * @param addr address of data to be written
 * @param data data to be written
 */
static int mem_write_word(uint32_t addr, uint16_t data) {
  // Check Init State at the beginning
  tap_ir_shift(IR_CNTRL_SIG_CAPTURE);
  if (!(tap_dr_shift16(0) & 0x0301))
    return SBW_ERR_GENERIC;

  clr_tclk_sbw();
  tap_ir_shift(IR_CNTRL_SIG_16BIT);

  tap_dr_shift16(0x0500);
  tap_ir_shift(IR_ADDR_16BIT);
  tap_dr_shift20(addr);

  set_tclk_sbw();
  // New style: Only apply data during clock high phase
  tap_ir_shift(IR_DATA_TO_ADDR);
  tap_dr_shift16(data); // Shift in 16 bits
  clr_tclk_sbw();
  tap_ir_shift(IR_CNTRL_SIG_16BIT);
  tap_dr_shift16(0x0501);
  set_tclk_sbw();
  // one or more cycle, so CPU is driving correct MAB
  clr_tclk_sbw();
  set_tclk_sbw();
  // Processor is now again in Init State

  return SBW_ERR_NONE;
}

/**
 * Execute a Power-On Reset (POR) using JTAG CNTRL SIG register
 *
 * @returns SBW_ERR_NONE if target is in Full-Emulation-State afterwards,
 * SBW_ERR_GENERIC otherwise
 *
 * @see SLAU320AJ 2.3.2.2.3
 */
int sbw_dev_reset(void) {
  // provide one clock cycle to empty the pipe
  clr_tclk_sbw();
  set_tclk_sbw();

  // prepare access to the JTAG CNTRL SIG register
  tap_ir_shift(IR_CNTRL_SIG_16BIT);
  // release CPUSUSP signal and apply POR signal
  tap_dr_shift16(0x0C01);
  // release POR signal again
  tap_dr_shift16(0x0401);

  // Set PC to 'safe' memory location
  tap_ir_shift(IR_DATA_16BIT);
  clr_tclk_sbw();
  set_tclk_sbw();
  clr_tclk_sbw();
  set_tclk_sbw();
  tap_dr_shift16(SAFE_FRAM_PC);
  // PC is set to 0x4 - MAB value can be 0x6 or 0x8

  // drive safe address into PC
  clr_tclk_sbw();
  set_tclk_sbw();

  tap_ir_shift(IR_DATA_CAPTURE);

  // two more to release CPU internal POR delay signals
  clr_tclk_sbw();
  set_tclk_sbw();
  clr_tclk_sbw();
  set_tclk_sbw();

  // now set CPUSUSP signal again
  tap_ir_shift(IR_CNTRL_SIG_16BIT);
  tap_dr_shift16(0x0501);
  // and provide one more clock
  clr_tclk_sbw();
  set_tclk_sbw();
  // the CPU is now in 'Full-Emulation-State'

  // disable Watchdog Timer on target device now by setting the HOLD signal
  // in the WDT_CNTRL register
  uint16_t id = tap_ir_shift(IR_CNTRL_SIG_CAPTURE);
  if (id == JTAG_ID98) {
    mem_write_word(0x01CC, 0x5A80);
  } else {
    mem_write_word(0x015C, 0x5A80);
  }

  // Initialize Test Memory with default values to ensure consistency
  // between PC value and MAB (MAB is +2 after sync)
  if (id == JTAG_ID91 || id == JTAG_ID99) {
    mem_write_word(0x06, 0x3FFF);
    mem_write_word(0x08, 0x3FFF);
  }

  // Check if device is in Full-Emulation-State again and return status
  tap_ir_shift(IR_CNTRL_SIG_CAPTURE);
  if (tap_dr_shift16(0) & 0x0301) {
    return SBW_ERR_NONE;
  }

  return SBW_ERR_GENERIC;
}

int sbw_dev_mem_read(uint16_t *dst, uint32_t addr, size_t n_words) {
  int rc;
  for (unsigned int i = 0; i < n_words; i++) {
    if ((rc = mem_read_word(dst + i, addr + 2 * i)) != SBW_ERR_NONE)
      return rc;
  }
  return SBW_ERR_NONE;
}

int sbw_dev_mem_write(uint32_t addr, uint16_t *data, size_t n_words) {
  int rc;
  for (unsigned int i = 0; i < n_words; i++) {
    if ((rc = mem_write_word(addr + 2 * i, data[i])) != SBW_ERR_NONE)
      return rc;
  }
  return SBW_ERR_NONE;
}

int sbw_dev_reg_set(uint8_t reg, uint32_t data) {
  uint16_t Mova;
  uint16_t data_lower;

  /* MOVA #imm20, PC, see SLAUF391F 1.6.1*/
  Mova = 0x0080 | reg;
  Mova += (uint16_t)((data >> 8) & 0x00000F00);
  data_lower = (uint16_t)((data & 0xFFFF));

  // Check Full-Emulation-State at the beginning
  tap_ir_shift(IR_CNTRL_SIG_CAPTURE);
  if (!(tap_dr_shift16(0) & 0x0301))
    return SBW_ERR_GENERIC;

  clr_tclk_sbw();
  // take over bus control during clock LOW phase
  tap_ir_shift(IR_DATA_16BIT);
  set_tclk_sbw();
  tap_dr_shift16(Mova);
  clr_tclk_sbw();
  tap_ir_shift(IR_CNTRL_SIG_16BIT);
  tap_dr_shift16(0x1400); // Release low byte
  tap_ir_shift(IR_DATA_16BIT);
  clr_tclk_sbw();
  set_tclk_sbw();
  tap_dr_shift16(data_lower);
  clr_tclk_sbw();
  set_tclk_sbw();
  tap_dr_shift16(0x4303); // insert NOP
  clr_tclk_sbw();
  tap_ir_shift(IR_ADDR_CAPTURE);
  tap_dr_shift20(0x00000);
  return SBW_ERR_NONE;
}

int sbw_dev_pc_set(uint32_t addr) {
  sbw_dev_reg_set(0, addr);
  return SBW_ERR_NONE;
}

int sbw_dev_halt(void) {
  /* Set to instruction fetch mode */
  tap_ir_shift(IR_DATA_16BIT);
  tap_dr_shift16(0x3FFF); // JMP $+0

  clr_tclk_sbw();

  tap_ir_shift(IR_CNTRL_SIG_16BIT);
  tap_dr_shift16(0x2409); // set JTAG_HALT bit
  set_tclk_sbw();
  return SBW_ERR_NONE;
}

int sbw_dev_release(void) {
  clr_tclk_sbw();

  // debugstr("Releasing target MSP430.");

  tap_ir_shift(IR_CNTRL_SIG_16BIT);
  tap_dr_shift16(0x2C01);
  tap_dr_shift16(0x2401); // Release reset.
  tap_ir_shift(IR_CNTRL_SIG_RELEASE);
  set_tclk_sbw();
  return SBW_ERR_NONE;
}

int sbw_dev_get_coreip_id(uint16_t *coreip_id) {
  tap_ir_shift(IR_COREIP_ID);
  *coreip_id = tap_dr_shift16(0);
  if (*coreip_id == 0) {
    return SBW_ERR_GENERIC;
  }

  // The ID pointer is an un-scrambled 20bit value
  return SBW_ERR_NONE;
}

int sbw_dev_erase(void) { return -1; }

int sbw_dev_get_device_id(uint16_t *device_id_ptr) {
  tap_ir_shift(IR_DEVICE_ID);
  *device_id_ptr = tap_dr_shift20(0);
  return SBW_ERR_NONE;
}

int sbw_dev_setup(sbw_pins_t *sbw_pins) {
  int rc = sbw_jtag_setup(sbw_pins);
  return rc;
}

int sbw_dev_connect(void) {
  int rc;

  uint16_t core_id;

  if ((rc = sbw_jtag_connect()) != SBW_ERR_NONE)
    return rc;
  if (is_lock_key_programmed())
    return SBW_ERR_GENERIC;
  if ((rc = sbw_dev_get_coreip_id(&core_id)) != SBW_ERR_NONE)
    return rc;
  if ((rc = sbw_jtag_sync()) != SBW_ERR_NONE)
    return rc;
  if ((rc = sbw_dev_reset()) != SBW_ERR_NONE)
    return rc;
  return SBW_ERR_NONE;
}

int sbw_dev_disconnect(void) {
  tap_ir_shift(IR_CNTRL_SIG_16BIT);
  tap_dr_shift16(0x2C01);
  tap_dr_shift16(0x2401);
  tap_ir_shift(IR_CNTRL_SIG_RELEASE);
  int rc = sbw_jtag_disconnect();
  return rc;
}
