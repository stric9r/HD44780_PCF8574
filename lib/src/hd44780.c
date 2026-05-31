/**
  ******************************************************************************
  * @file           : hd44780.c
  * @brief          : HD44780 character LCD driver via PCF8574 I2C backpack
  ******************************************************************************
  * SPDX-License-Identifier: GPL-3.0-or-later
  * Copyright (C) 2026 Stric Roberts.
  * Copyright (C) 2020 Marian Hrinko (mato.hrinko@gmail.com) — upstream original.
  *
  * This file is a derivative of HD44780_PCF8574 and is licensed under the
  * GNU General Public License v3 or later. See LICENSE for the full text,
  * or <https://www.gnu.org/licenses/>.
  *
  * Derived from: https://github.com/Matiasus/HD44780_PCF8574
  * AVR TWI transport replaced with the platform-agnostic pcf8574_i2c layer.
  * Timing values preserved from the original; source is the HD44780 datasheet.
  ******************************************************************************
  */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "hd44780.h"
#include "implement/pcf8574_i2c.h"
#include "implement/delay.h"

/* PCF8574 output bit to HD44780 pin mapping.
 * Matches common commercially available I2C LCD backpacks. */
#define LCD_RS   0x01u   /* P0: Register Select (0 = instruction, 1 = data) */
#define LCD_RW   0x02u   /* P1: Read/Write      (0 = write, held low always) */
#define LCD_E    0x04u   /* P2: Enable          (falling edge latches data)  */
#define LCD_BL   0x08u   /* P3: Backlight       (1 = on)                     */
#define LCD_DB4  0x10u   /* P4: Data bit 4                                    */
#define LCD_DB5  0x20u   /* P5: Data bit 5                                    */
#define LCD_DB6  0x40u   /* P6: Data bit 6                                    */
#define LCD_DB7  0x80u   /* P7: Data bit 7                                    */

/* HD44780 instruction bytes. */
#define CMD_DISP_CLEAR    0x01u  /* Clear display; cursor to home.           */
#define CMD_ENTRY_MODE    0x06u  /* Cursor right, no display shift.          */
#define CMD_DISP_OFF      0x08u  /* Display off.                             */
#define CMD_DISP_ON       0x0Cu  /* Display on, cursor off, blink off.       */
#define CMD_CURSOR_ON     0x0Eu  /* Display on, cursor on, blink off.        */
#define CMD_CURSOR_BLINK  0x0Fu  /* Display on, cursor on, blink on.         */
#define CMD_FUNC_4BIT     0x20u  /* Function set: 4-bit, 1 row, 5x8.        */
#define CMD_FUNC_4B_2R    0x28u  /* Function set: 4-bit, 2 rows, 5x8.       */
#define CMD_DDRAM_ADDR    0x80u  /* Set DDRAM address (OR with address).     */

#define DDRAM_ROW0        0x00u  /* DDRAM start address for row 0.           */
#define DDRAM_ROW1        0x40u  /* DDRAM start address for row 1.           */

/* Conservative post-instruction busy delay. The HD44780 datasheet
 * requires 37 µs for most commands and 1.52 ms for Clear Display.
 * 50 ms covers all cases without per-command branching. LCD updates
 * arrive at human speed over BLE, so the extra latency is acceptable. */
#define INSTR_DELAY_MS   50u

/* ---------------------------------------------------------------------------
 * Internal helpers — not part of the public API
 * ---------------------------------------------------------------------------*/

/**
 * @brief Generate an Enable (E) pulse to latch data into the HD44780.
 *
 * At 100 kbit/s I2C each byte transaction takes ~200 µs, which already
 * exceeds the 450 ns PWeh minimum. The explicit 1 µs delays provide
 * additional margin when a faster I2C clock rate is configured.
 *
 * @param addr  7-bit I2C address of the PCF8574.
 * @param data  Current byte on the PCF8574 output latch (E bit clear).
 */
static void e_pulse(uint8_t addr, uint8_t data)
{
    pcf8574_write(addr, data | LCD_E);
    delay_us(1u);
    pcf8574_write(addr, data & (uint8_t)(~LCD_E));
    delay_us(1u);
}

/**
 * @brief Send a single 4-bit nibble to the HD44780.
 *
 * Used during the initialisation sequence before 4-bit mode is
 * confirmed; no busy-flag check is possible at this stage.
 *
 * @param addr    7-bit I2C address of the PCF8574.
 * @param nibble  Upper four bits are the data; lower four must be 0.
 */
static void send_4bits(uint8_t addr, uint8_t nibble)
{
    pcf8574_write(addr, nibble);
    e_pulse(addr, nibble);
}

/**
 * @brief Send one full byte to the HD44780 as two 4-bit nibbles.
 *
 * @param addr   7-bit I2C address of the PCF8574.
 * @param data   Byte to send (instruction or data).
 * @param flags  Control/backlight bits ORed into each nibble transfer
 *               (e.g. LCD_BL for instructions, LCD_RS | LCD_BL for data).
 */
static void send_8bits(uint8_t addr, uint8_t data, uint8_t flags)
{
    uint8_t upper = (data & 0xF0u) | flags;
    uint8_t lower = ((data << 4u) & 0xF0u) | flags;

    pcf8574_write(addr, upper);
    e_pulse(addr, upper);
    pcf8574_write(addr, lower);
    e_pulse(addr, lower);
}

/**
 * @brief Send an HD44780 instruction and wait for it to execute.
 *
 * RS is held low (instruction register). Backlight is kept on.
 *
 * @param addr   7-bit I2C address of the PCF8574.
 * @param instr  Instruction byte (see CMD_* defines).
 */
static void send_instruction(uint8_t addr, uint8_t instr)
{
    send_8bits(addr, instr, LCD_BL);
    delay_ms(INSTR_DELAY_MS);
}

/**
 * @brief Write one character data byte to the HD44780 DDRAM.
 *
 * RS is held high (data register). Backlight is kept on.
 * No additional delay is required; the I2C transaction time exceeds
 * the 43 µs HD44780 data-write execution time at 100 kbit/s.
 *
 * @param addr  7-bit I2C address of the PCF8574.
 * @param data  Character code to write.
 */
static void send_data(uint8_t addr, uint8_t data)
{
    send_8bits(addr, data, LCD_RS | LCD_BL);
}

/* ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------*/

int hd44780_init(uint8_t addr)
{
    /* HD44780 power-on initialisation (datasheet, p.45).
     * The first three function-set pulses are sent as 4-bit nibbles while
     * the display is still in its default 8-bit mode; the busy flag cannot
     * be checked during this phase, so fixed delays are used. */
    delay_ms(16u);   /* VCC rise: datasheet requires > 15 ms. */

    send_4bits(addr, LCD_DB4 | LCD_DB5);   /* Attempt 1: function set.  */
    delay_ms(5u);                           /* Datasheet: > 4.1 ms.      */

    send_4bits(addr, LCD_DB4 | LCD_DB5);   /* Attempt 2: function set.  */
    delay_us(110u);                         /* Datasheet: > 100 µs.      */

    send_4bits(addr, LCD_DB4 | LCD_DB5);   /* Attempt 3: function set.  */
    delay_us(50u);                          /* Datasheet: > 45 µs.       */

    send_4bits(addr, LCD_DB5);             /* Switch to 4-bit interface. */
    delay_us(50u);

    /* Display is now in 4-bit mode; full 8-bit commands work. */
    send_instruction(addr, CMD_FUNC_4B_2R); /* 4-bit, 2 rows, 5x8 font. */
    send_instruction(addr, CMD_DISP_OFF);
    send_instruction(addr, CMD_DISP_CLEAR);
    send_instruction(addr, CMD_ENTRY_MODE); /* Cursor right, no shift.   */
    send_instruction(addr, CMD_DISP_ON);    /* Display on, cursor off.   */

    return 0;
}

void hd44780_clear(uint8_t addr)
{
    send_instruction(addr, CMD_DISP_CLEAR);
}

void hd44780_display_on(uint8_t addr)
{
    send_instruction(addr, CMD_DISP_ON);
}

void hd44780_cursor_on(uint8_t addr)
{
    send_instruction(addr, CMD_CURSOR_ON);
}

void hd44780_cursor_blink(uint8_t addr)
{
    send_instruction(addr, CMD_CURSOR_BLINK);
}

int hd44780_set_cursor(uint8_t addr, uint8_t col, uint8_t row)
{
    uint8_t ddram_addr;
    bool b_status;
    int result;

    b_status = (col < HD44780_COLS) && (row < HD44780_ROWS);

    if (b_status)
    {
        ddram_addr = (0u == row)
                     ? (uint8_t)(DDRAM_ROW0 + col)
                     : (uint8_t)(DDRAM_ROW1 + col);

        send_instruction(addr, CMD_DDRAM_ADDR | ddram_addr);
    }

    result = b_status ? 0 : -1;

    return result;
}

void hd44780_write_char(uint8_t addr, char c)
{
    send_data(addr, (uint8_t)c);
}

void hd44780_write_string(uint8_t addr, char const *p_str)
{
    bool b_status;
    uint8_t i;

    assert(NULL != p_str);

    b_status = (NULL != p_str);

    if (b_status)
    {
        i = 0u;
        while ('\0' != p_str[i])
        {
            send_data(addr, (uint8_t)p_str[i]);
            i++;
        }
    }
}
