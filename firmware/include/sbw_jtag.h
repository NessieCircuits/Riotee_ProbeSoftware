#ifndef __SBW_JTAG_H_
#define __SBW_JTAG_H_

#include "sbw_transport.h"
#include <stdint.h>

#define SBW_ERR_NONE 0
#define SBW_ERR_GENERIC 1

// Constants for data formats, dedicated addresses
#define F_BYTE 8
#define F_WORD 16
#define F_ADDR 20
#define F_LONG 32
#define V_RESET 0xFFFE
#define V_BOR 0x1B08

// Set the JTAG control signal register
#define IR_CNTRL_SIG_16BIT 0xC8 // original value: 0x13
// Read out the JTAG control signal register
#define IR_CNTRL_SIG_CAPTURE 0x28 // original value: 0x14
// Release the CPU from JTAG control
#define IR_CNTRL_SIG_RELEASE 0xA8 // original value: 0x15

// Instructions for the JTAG fuse
// Prepare for JTAG fuse blow
#define IR_PREPARE_BLOW 0x44 // original value: 0x22
// Perform JTAG fuse blow
#define IR_EX_BLOW 0x24 // original value: 0x24

// Instructions for the JTAG data register
// Set the MSP430 MDB to a specific 16-bit value with the next
// 16-bit data access
#define IR_DATA_16BIT 0x82 // original value: 0x41
// Set the MSP430 MDB to a specific 16-bit value (RAM only)
#define IR_DATA_QUICK 0xC2 // original value: 0x43

// Instructions for the JTAG address register
// set the MSP430 MAB to a specific 16-bit value
// Use the 20-bit macro for 430X and 430Xv2 architectures
#define IR_ADDR_16BIT 0xC1 // original value: 0x83
// Read out the MAB data on the next 16/20-bit data access
#define IR_ADDR_CAPTURE 0x21 // original value: 0x84
// Set the MSP430 MDB with a specific 16-bit value and write
// it to the memory address which is currently on the MAB
#define IR_DATA_TO_ADDR 0xA1 // original value: 0x85
// Bypass instruction - TDI input is shifted to TDO as an output
#define IR_BYPASS 0xFF // original value: 0xFF
#define IR_DATA_CAPTURE 0x42

// JTAG identification values for all existing Flash-based MSP430 devices
// JTAG identification value for 430X architecture devices
#define JTAG_ID 0x89
// JTAG identification value for 430Xv2 architecture devices
#define JTAG_ID91 0x91
// JTAG identification value for 430Xv2 architecture FR4XX/FR2xx devices
#define JTAG_ID98 0x98
// JTAG identification value for 430Xv2 architecture FR59XX devices
#define JTAG_ID99 0x99
// Additional instructions for JTAG_ID91 architectures
// Instruction to determine device's CoreIP
#define IR_COREIP_ID 0xE8 // original value: 0x17
// Instruction to determine device's DeviceID
#define IR_DEVICE_ID 0xE1 // original value: 0x87

// Request a JTAG mailbox exchange
#define IR_JMB_EXCHANGE 0x86 // original value: 0x61
// Instruction for test register in 5xx
#define IR_TEST_REG 0x54 // original value: 0x2A
// Instruction for 3 volt test register in 5xx
#define IR_TEST_3V_REG 0xF4 // original value: 0x2F

// JTAG mailbox constant -
#define OUT1RDY 0x0008
// JTAG mailbox constant -
#define IN0RDY 0x0001
// JTAG mailbox constant -
#define JMB32B 0x0010
// JTAG mailbox constant -
#define OUTREQ 0x0004
// JTAG mailbox constant -
#define INREQ 0x0001
// JTAG mailbox mode 32 bit -
#define MAIL_BOX_32BIT 0x10
// JTAG mailbox moede 16 bit -
#define MAIL_BOX_16BIT 0x00

#define STOP_DEVICE 0xA55A

/**
 * Resets the TAP controller state machine
 *
 * @see SLAU320AJ 2.3.1.2
 * */
void tap_reset(void);

/**
 * Loads a JTAG instruction into the JTAG instruction register (IR) of the
 * target device
 *
 * @param instruction JTAG instruction
 */
uint32_t tap_ir_shift(uint8_t instruction);

/**
 * Loads a 16-bit word into the JTAG data register
 *
 * @param data register content
 */
uint16_t tap_dr_shift16(uint16_t data);

/**
 * Loads a 20-bit address into the JTAG data register
 *
 * @param address address to load
 */
uint32_t tap_dr_shift20(uint32_t address);

/**
 * Initializes SBW pins
 *
 * @param sbw_pins list of SBW pins
 */
int sbw_jtag_setup(sbw_pins_t *sbw_pins);

/* Connect the JTAG/SBW Signals and execute delay */
int sbw_jtag_connect(void);

/* Stop JTAG/SBW by disabling the pins and executing delay */
int sbw_jtag_disconnect(void);

/**
 * Writes a 16bit value into the JTAG mailbox system. The function timeouts if
 * the mailbox is not empty after a certain number of retries.
 *
 * @param data data to be shifted into mailbox
 */
int sbw_jtag_write_jmb_in16(uint16_t data);

/**
 * Resync the JTAG connection
 *
 * @returns SBW_ERR_NONE if operation was successful, SBW_ERR_GENERIC otherwise
 *
 * @see SLAU320AJ 2.3.2.2.1
 */
int sbw_jtag_sync(void);

#endif /* __SBW_JTAG_H_ */
