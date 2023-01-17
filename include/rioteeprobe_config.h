/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
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

#ifndef RIOTEEPROBE_H_
#define RIOTEEPROBE_H_

#define PROBE_PIN_LED 2

#define PROBE_PIN_SWDCLK 19
#define PROBE_PIN_SWDIO 12
#define PROBE_PIN_SWDDIR 11

#define PROBE_PIN_SBWCLK 21
#define PROBE_PIN_SBWIO 16
#define PROBE_PIN_SBWDIR 22

#define PROBE_PIN_TARGET_POWER 0
#define PROBE_PIN_TRANS_CLOCK_EN 18
#define PROBE_PIN_TRANS_SWD_UARTRX_EN 5
#define PROBE_PIN_TRANS_SBW_UARTTX_EN 20

// UART config
#define PROBE_UART_TX 24
#define PROBE_UART_RX 25

#define PROBE_UART_INTERFACE uart1
#define PROBE_UART_BAUDRATE 115200

#endif /* RIOTEEPROBE_H_ */
