#ifndef __SBW_DEVICE_H_
#define __SBW_DEVICE_H_

#include "sbw_jtag.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Initializes SBW pins
 *
 * @param sbw_pins list of SBW pins
 */
int sbw_dev_setup(sbw_pins_t *sbw_pins);

/* Brings device under JTAG control */
int sbw_dev_connect(void);
/* Releases device from JTAG control */
int sbw_dev_disconnect(void);

/**
 * Reads device coreip ID
 *
 * @param coreip_id destination address
 *
 * @see SLAU320AJ 2.3.2.2.1
 */
int sbw_dev_get_coreip_id(uint16_t *coreip_id);

/**
 * Retrieves pointer to device id memory location
 *
 * @param device_id_ptr destination address
 *
 * @see SLAU320AJ 2.3.2.2.1
 * @note Seems to return address shifted by four positions?!
 */
int sbw_dev_get_device_id(uint16_t *device_id_ptr);

/**
 * Reads data from a given address in memory
 *
 * @param dst pointer to destination buffer
 * @param addr address of data to be read
 * @param n_words number of 16-bit words to be read
 *
 * @returns Data from device
 */
int sbw_dev_mem_read(uint16_t *dst, uint32_t addr, size_t n_words);

/**
 * Writes data to a given address
 *
 * @param addr address of data to be written
 * @param data pointer to source buffer
 * @param n_words number of 16-bit words to be written
 *
 */
int sbw_dev_mem_write(uint32_t addr, uint16_t *data, size_t n_words);

/**
 * Brings CPU to halt
 *
 * @see SLAU320AJ 2.3.2.1.4
 */
int sbw_dev_halt(void);

/**
 * Releases CPU from halt and continues execution
 *
 * @see SLAU320AJ 2.3.2.1.4
 */
int sbw_dev_release(void);

/**
 * @brief Executes power on reset
 *
 * @return int 0 on success, <0 otherwise
 */
int sbw_dev_reset(void);

/**
 * Loads an address into the PC register
 *
 * @param addr address to load
 *
 * @see SLAU320AJ 2.3.2.2.2
 */
int sbw_dev_pc_set(uint32_t addr);

/**
 * Loads a value into a CPU register
 *
 * @param reg register number
 * @param data data to load
 *
 * @see SLAU320AJ 2.3.2.2.2
 */
int sbw_dev_reg_set(uint8_t reg, uint32_t data);

#endif /* __SBW_DEVICE_H_ */