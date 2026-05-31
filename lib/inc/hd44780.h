/**
  ******************************************************************************
  * @file           : hd44780.h
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
  ******************************************************************************
  */

#ifndef HD44780_H
#define HD44780_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @defgroup hd44780 HD44780 LCD Driver
 * @brief Driver for HD44780-compatible character LCDs connected via a
 *        PCF8574 I2C backpack in 4-bit mode.
 *
 * Hardware assumptions:
 *  - 16 columns x 2 rows
 *  - PCF8574 wiring: RS=P0, RW=P1, E=P2, backlight=P3, DB4-DB7=P4-P7
 *  - I2C address passed to every call (supports multiple displays)
 *
 * Dependencies: implement/pcf8574_i2c.h (I/O expander), implement/delay.h (timing).
 * Both require platform implementations to be linked into the build.
 * @{
 */

/** @brief Number of character columns on the display. */
#define HD44780_COLS  16u

/** @brief Number of character rows on the display. */
#define HD44780_ROWS  2u

/**
 * @brief Execute the HD44780 power-on initialisation sequence.
 *
 * Follows the recommended initialisation procedure from the HD44780
 * datasheet (p.45): three 8-bit function-set attempts, transition to
 * 4-bit mode, function configuration, display off, clear, entry mode
 * set, then display on with cursor and blink disabled.
 *
 * Blocks for approximately 20 ms on entry for the VCC rise-time delay.
 * Call once after power-on, before any other function in this module.
 *
 * @param addr  7-bit I2C address of the PCF8574 backpack.
 * @return      0 on success.
 */
int hd44780_init(uint8_t addr);

/**
 * @brief Clear all characters and move the cursor to (0, 0).
 *
 * Issues the HD44780 Clear Display command. Blocks for 50 ms.
 *
 * @param addr  7-bit I2C address of the PCF8574 backpack.
 */
void hd44780_clear(uint8_t addr);

/**
 * @brief Turn the display on with cursor and blink both disabled.
 *
 * @param addr  7-bit I2C address of the PCF8574 backpack.
 */
void hd44780_display_on(uint8_t addr);

/**
 * @brief Turn the display on with the cursor visible (blink disabled).
 *
 * @param addr  7-bit I2C address of the PCF8574 backpack.
 */
void hd44780_cursor_on(uint8_t addr);

/**
 * @brief Turn the display on with the cursor visible and blinking.
 *
 * @param addr  7-bit I2C address of the PCF8574 backpack.
 */
void hd44780_cursor_blink(uint8_t addr);

/**
 * @brief Move the cursor to a specific column and row.
 *
 * Coordinates are zero-indexed: (0, 0) is the top-left character.
 *
 * @param addr  7-bit I2C address of the PCF8574 backpack.
 * @param col   Column, 0 to HD44780_COLS - 1.
 * @param row   Row, 0 to HD44780_ROWS - 1.
 * @return      0 on success, -1 if col or row is out of range.
 */
int hd44780_set_cursor(uint8_t addr, uint8_t col, uint8_t row);

/**
 * @brief Write one character at the current cursor position.
 *
 * The cursor advances one position after the write. Characters written
 * past column 15 wrap into DDRAM addresses not visible on a 16-column
 * display; use hd44780_set_cursor() to reposition explicitly.
 *
 * @param addr  7-bit I2C address of the PCF8574 backpack.
 * @param c     Character to display.
 */
void hd44780_write_char(uint8_t addr, char c);

/**
 * @brief Write a null-terminated string at the current cursor position.
 *
 * Writes characters sequentially until the null terminator. Does not
 * clip at column 16; the caller is responsible for keeping strings
 * within the visible area.
 *
 * @param addr   7-bit I2C address of the PCF8574 backpack.
 * @param p_str  Null-terminated string to display. Must not be NULL.
 */
void hd44780_write_string(uint8_t addr, char const *p_str);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* HD44780_H */
