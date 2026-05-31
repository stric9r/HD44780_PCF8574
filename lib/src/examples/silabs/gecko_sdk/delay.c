/**
  ******************************************************************************
  * @file           : delay.c
  * @brief          : Gecko SDK implementation of the delay platform interface
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
  *
  * SDK dependencies:
  *   sl_udelay.h   Silicon Labs udelay service
  *                 platform/service/udelay/inc/sl_udelay.h
  *                 Gecko SDK component: udelay (tested: 4.4.5)
  *
  * sl_udelay_wait() is a blocking busy-wait implemented in assembly for
  * Cortex-M. It is safe to call from any non-interrupt context. The udelay
  * component requires no explicit initialisation; delay_init() is a no-op.
  * Do not call delay_ms() with large values from time-critical paths.
  ******************************************************************************
  */

#include "implement/delay.h"
#include "sl_udelay.h"

void delay_init(void)
{
    /* No setup required. sl_udelay_wait() is ready after system init. */
}

void delay_us(uint32_t us)
{
    sl_udelay_wait(us);
}

void delay_ms(uint32_t ms)
{
    sl_udelay_wait(ms * 1000UL);
}
