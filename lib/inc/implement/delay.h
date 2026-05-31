/**
  ******************************************************************************
  * @file           : delay.h
  * @brief          : Platform-agnostic blocking delay interface
  ******************************************************************************
  * SPDX-License-Identifier: MIT
  * Copyright (C) 2026 Stric Roberts.
  *
  * Permission is hereby granted, free of charge, to any person obtaining a
  * copy of this software and associated documentation files (the "Software"),
  * to deal in the Software without restriction, including without limitation
  * the rights to use, copy, modify, merge, publish, distribute, sublicense,
  * and/or sell copies of the Software, and to permit persons to whom the
  * Software is furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  * IN THE SOFTWARE.
  ******************************************************************************
  */

#ifndef DELAY_H
#define DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @defgroup delay Delay Interface
 * @brief Platform-agnostic blocking delay.
 *
 * Provide one translation unit that implements all three functions for the
 * target platform. delay_init() is called once at startup; the delay
 * functions must busy-wait for at least the requested duration.
 * Overshooting is acceptable; undershooting is not.
 *
 * Do not call these from interrupt context or from any path where blocking
 * is prohibited by an RTOS.
 *
 * See lib/src/gecko_sdk/delay.c for a Gecko SDK 4.4.5 reference.
 * @{
 */

/**
 * @brief One-time platform delay initialisation.
 *
 * Call once at startup before any other delay function. Platforms that
 * require SysTick or peripheral setup before delay functions work should
 * perform that setup here. Leave the body empty if no setup is needed.
 */
void delay_init(void);

/**
 * @brief Block for at least @p us microseconds.
 *
 * @param us  Microseconds to wait. 0 is a no-op.
 */
void delay_us(uint32_t us);

/**
 * @brief Block for at least @p ms milliseconds.
 *
 * Convenience wrapper; equivalent to delay_us(ms * 1000).
 *
 * @param ms  Milliseconds to wait. 0 is a no-op.
 */
void delay_ms(uint32_t ms);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DELAY_H */
