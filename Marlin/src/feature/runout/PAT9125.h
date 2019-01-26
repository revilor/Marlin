/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <Wire.h>
#include "../../inc/MarlinConfig.h"
#include "../../module/stepper.h"


#define PAT9125_TRIGGER_THRESHOLD   2

// PAT9125 registers
#define PAT9125_PID1            0x00
#define PAT9125_PID2            0x01
#define PAT9125_MOTION          0x02
#define PAT9125_DELTA_XL        0x03
#define PAT9125_DELTA_YL        0x04
#define PAT9125_CONFIG          0x06
#define PAT9125_WP              0x09
#define PAT9125_RES_X           0x0d
#define PAT9125_RES_Y           0x0e
#define PAT9125_DELTA_XYH       0x12
#define PAT9125_SHUTTER         0x14
#define PAT9125_FRAME           0x17
#define PAT9125_ORIENTATION     0x19
#define PAT9125_BANK_SELECTION  0x7f

// PAT9125 configuration
#define PAT9125_I2C_ADDR  0x75  //ID=LO
#define PAT9125_XRES      0
#define PAT9125_YRES      240

#ifdef PAT9125_NEW_INIT
  // Init sequence, address & value.
  const PROGMEM uint8_t pat9125_init_seq1[] = {
    // Disable write protect.
    PAT9125_WP, 0x5a,
    // Set the X resolution to zero to let the sensor know that it could safely ignore movement in the X axis.
    PAT9125_RES_X, PAT9125_XRES,
    // Set the Y resolution to a maximum (or nearly a maximum).
    PAT9125_RES_Y, PAT9125_YRES,
    // Set 12-bit X/Y data format.
    PAT9125_ORIENTATION, 0x04,
    // PAT9125_ORIENTATION, 0x04 | (xinv?0x08:0) | (yinv?0x10:0), //!? direction switching does not work
    // Now continues the magic sequence from the PAT912EL Application Note: Firmware Guides for Tracking Optimization.
    0x5e, 0x08,
    0x20, 0x64,
    0x2b, 0x6d,
    0x32, 0x2f,
    // stopper
    0xff
  };

  // Init sequence, address & value.
  const PROGMEM uint8_t pat9125_init_seq2[] = {
    // Magic sequence to enforce full frame rate of the sensor.
    0x06, 0x28,
    0x33, 0xd0,
    0x36, 0xc2,
    0x3e, 0x01,
    0x3f, 0x15,
    0x41, 0x32,
    0x42, 0x3b,
    0x43, 0xf2,
    0x44, 0x3b,
    0x45, 0xf2,
    0x46, 0x22,
    0x47, 0x3b,
    0x48, 0xf2,
    0x49, 0x3b,
    0x4a, 0xf0,
    0x58, 0x98,
    0x59, 0x0c,
    0x5a, 0x08,
    0x5b, 0x0c,
    0x5c, 0x08,
    0x61, 0x10,
    0x67, 0x9b,
    0x6e, 0x22,
    0x71, 0x07,
    0x72, 0x08,
    // stopper
    0xff
  };
#endif

class FilamentSensorBase {
  private:
    static bool operative;
    static int16_t x, y;
    static uint8_t frame, shutter;

    static uint8_t readRegister(const uint8_t addr);
    static void writeRegister(const uint8_t addr, const uint8_t data);

    inline static bool writeRegisterVerify(const uint8_t addr, const uint8_t data) {
      writeRegister(addr, data);

      return readRegister(addr) == data;
    }

  protected:
    static void filament_present(const uint8_t extruder);

  public:
    static inline bool setup() {
      operative = false;

      SERIAL_ECHOLN("PAT9125::init");
      Wire.begin();
      // Verify that the sensor responds with its correct product ID.
      uint8_t pat9125_PID1 = readRegister(PAT9125_PID1);
      uint8_t pat9125_PID2 = readRegister(PAT9125_PID2);
      if (pat9125_PID1 != 0x31 || pat9125_PID2 != 0x91) {
        pat9125_PID1 = readRegister(PAT9125_PID1);
        pat9125_PID2 = readRegister(PAT9125_PID2);
        if (pat9125_PID1 != 0x31 || pat9125_PID2 != 0x91)
          SERIAL_ECHOLN("PAT9125::init failed");
          SERIAL_ECHOLN(pat9125_PID1);
          SERIAL_ECHOLN(pat9125_PID2);
          return false;
      }

      #ifdef PAT9125_NEW_INIT
        // Switch to bank0, not allowed to perform OTS_RegWriteRead.
        writeRegister(PAT9125_BANK_SELECTION, 0);
        // Software reset (i.e. set bit7 to 1). It will reset to 0 automatically.
        // After the reset, OTS_RegWriteRead is not allowed.
        writeRegister(PAT9125_CONFIG, 0x97);
        // Wait until the sensor reboots.
        _delay_ms(1);
        uint8_t *ptr = pat9125_init_seq1;
        for (;;) {
          const uint8_t addr = pgm_read_byte_near(ptr++);
          if (addr == 0xff)
            break;
          if (!writeRegisterVerify(addr, pgm_read_byte_near(ptr++)))
            return false;  // Verification of the register write failed.
        }
        _delay_ms(10);
        // Switch to bank1, not allowed to perform OTS_RegWrite.
        writeRegister(PAT9125_BANK_SELECTION, 0x01);
        *ptr = pat9125_init_seq2;
        for (;;) {
          const uint8_t addr = pgm_read_byte_near(ptr++);
          if (addr == 0x0ff)
            break;
          if (!writeRegisterVerify(addr, pgm_read_byte_near(ptr++)))
            return false;   // Verification of the register write failed.
        }
        // Switch to bank0, not allowed to perform OTS_RegWriteRead.
        writeRegister(PAT9125_BANK_SELECTION, 0x00);
        // Enable write protect.
        writeRegister(PAT9125_WP, 0x00);

        pat9125_PID1 = readRegister(PAT9125_PID1);
        pat9125_PID2 = readRegister(PAT9125_PID2);
        if (pat9125_PID1 != 0x31 || pat9125_PID2 != 0x91)
            return false;
      #else
        writeRegister(PAT9125_RES_X, PAT9125_XRES);
        writeRegister(PAT9125_RES_Y, PAT9125_YRES);
      #endif // PAT9125_NEW_INIT
      operative = true;

      serialprintPGM(PSTR("PAT9125_RES_X=")); SERIAL_ECHO_F(readRegister(PAT9125_RES_X), DEC); SERIAL_ECHOLN("");
      serialprintPGM(PSTR("PAT9125_RES_Y=")); SERIAL_ECHO_F(readRegister(PAT9125_RES_Y), DEC); SERIAL_ECHOLN("");

      return true;
    }

    static bool update(void);
    static bool updateY(void);

    //static inline int16_t getX(void) { return x; }
    static inline int16_t getY(void) { return y; }
    //static inline uint8_t getFrame(void) { return frame; }
    //static inline uint8_t getShutter(void) { return shutter; }
};

/**
 * This is Průša laser sensor in the path of the filament.
 * It can detect filament runout, stripouts and jams.
 */
class FilamentSensorPAT9125 : public FilamentSensorBase {
  private:
    static bool motion_detected;

  public:

    static volatile int16_t oldY;
    static volatile uint16_t e_steps;

    static inline void block_completed(const block_t* const b) {

      if (b->steps[E_AXIS] > 0) {
        if (updateY()) {
          int16_t y = getY();
          motion_detected |= (y != oldY);
          oldY = y;
        } else {
          // sensor not working
          motion_detected = true;
        }


//        SERIAL_ECHO("block_completed ");
//        SERIAL_ECHO_F(b->steps[E_AXIS], DEC);
//        SERIAL_ECHOLN("");



//              SERIAL_ECHO_F(e_steps * planner.steps_to_mm[E_AXIS_N(b->extruder)], DEC);
//              SERIAL_ECHOLN("mm");


          if (motion_detected) {
//            SERIAL_ECHOLN("Motion!");
            e_steps = 0;
            filament_present(b->extruder);
          } else {
            if (e_steps * planner.steps_to_mm[E_AXIS_N(b->extruder)] < PAT9125_TRIGGER_THRESHOLD) {

              filament_present(b->extruder);
            } else {
              SERIAL_ECHO_F(e_steps * planner.steps_to_mm[E_AXIS_N(b->extruder)], DEC);
              SERIAL_ECHOLN("mm");
            }

            e_steps += b->steps[E_AXIS];
          }
      }       
      
      // Clear motion triggers for next block
      motion_detected = false;
    }

    static void run() {
    }  
};
