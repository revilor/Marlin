/* **************************************************************************

 Marlin 3D Printer Firmware
 Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 Copyright (c) 2016 Bob Cousins bobcousins42@googlemail.com

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#include "../HAL_SPI.h"
#include <Arduino.h>
#include "../../core/serial.h"

#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

// define SPI_AVR_EIMSK for AVR boards with external interrupt pins
#if defined(EIMSK)
  #define SPI_AVR_EIMSK  EIMSK
#elif defined(GICR)
  #define SPI_AVR_EIMSK  GICR
#elif defined(GIMSK)
  #define SPI_AVR_EIMSK  GIMSK
#endif


namespace HAL {
    namespace SPI {

        bool initialized = false;
        uint8_t interruptMode; // 0=none, 1=mask, 2=global
        uint8_t interruptMask; // which interrupts to mask
        uint8_t interruptSave; // temp storage, to restore state
        uint8_t currentSS;
        bool inTransaction = false;

        /**
        * Initialize and enable the SPI bus.
        */
        void init() {
            uint8_t sreg = SREG;
            noInterrupts(); // Protect from a scheduler and prevent transactionBegin
            if (!initialized) {

                SPCR |= _BV(MSTR);
                SPCR |= _BV(SPE);
                            
              // Set direction register for SCK and MOSI pin.
              // MISO pin automatically overrides to INPUT.
              // By doing this AFTER enabling SPI, we avoid accidentally
              // clocking in a single bit since the lines go directly
              // from "input" to SPI control.
              // http://code.google.com/p/arduino/issues/detail?id=888
              pinMode(SCK, OUTPUT);
              pinMode(MOSI, OUTPUT);
            }
            initialized = true;
            SREG = sreg;            
        }
        
        /**
         * Disable the SPI bus.
         */
        void end() {

        }

        /**
         * Gain exclusive access to the SPI bus.
         * 
         * Return:  true    if access to the SPI bus was granted
         *          false   if SPI bus acquisition failed
         */
        bool beginTransaction(uint8_t ss, uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {

            if (inTransaction) {
                return false;
            }
              // if the SS pin is not already configured as an output
              // then set it high (to enable the internal pull-up resistor)
                digitalWrite(ss, HIGH);
          
              // When the SS pin is set as OUTPUT, it can be used as
              // a general purpose output port (it doesn't influence
              // SPI operations).
              pinMode(ss, OUTPUT);
          

              if (interruptMode > 0) {
                uint8_t sreg = SREG;
                noInterrupts();
          
                #ifdef SPI_AVR_EIMSK
                if (interruptMode == 1) {
                  interruptSave = SPI_AVR_EIMSK;
                  SPI_AVR_EIMSK &= ~interruptMask;
                  SREG = sreg;
                } else
                #endif
                {
                  interruptSave = sreg;
                }
              }

                // Clock settings are defined as follows. Note that this shows SPI2X
                // inverted, so the bits form increasing numbers. Also note that
                // fosc/64 appears twice
                // SPR1 SPR0 ~SPI2X Freq
                //   0    0     0   fosc/2
                //   0    0     1   fosc/4
                //   0    1     0   fosc/8
                //   0    1     1   fosc/16
                //   1    0     0   fosc/32
                //   1    0     1   fosc/64
                //   1    1     0   fosc/64
                //   1    1     1   fosc/128

                // We find the fastest clock that is less than or equal to the
                // given clock rate. The clock divider that results in clock_setting
                // is 2 ^^ (clock_div + 1). If nothing is slow enough, we'll use the
                // slowest (128 == 2 ^^ 7, so clock_div = 6).
                uint8_t clockDiv;
                
                // When the clock is known at compiletime, use this if-then-else
                // cascade, which the compiler knows how to completely optimize
                // away. When clock is not known, use a loop instead, which generates
                // shorter code.
                if (__builtin_constant_p(clock)) {
                if (clock >= F_CPU / 2) {
                    clockDiv = 0;
                } else if (clock >= F_CPU / 4) {
                    clockDiv = 1;
                } else if (clock >= F_CPU / 8) {
                    clockDiv = 2;
                } else if (clock >= F_CPU / 16) {
                    clockDiv = 3;
                } else if (clock >= F_CPU / 32) {
                    clockDiv = 4;
                } else if (clock >= F_CPU / 64) {
                    clockDiv = 5;
                } else {
                    clockDiv = 6;
                }
                } else {
                uint32_t clockSetting = F_CPU / 2;
                clockDiv = 0;
                while (clockDiv < 6 && clock < clockSetting) {
                    clockSetting /= 2;
                    clockDiv++;
                }
                }
                SERIAL_ECHOLN(clockDiv);
                // Compensate for the duplicate fosc/64
                if (clockDiv == 6)
                clockDiv = 7;
            
                // Invert the SPI2X bit
                clockDiv ^= 0x1;
                  
              SPCR = _BV(SPE) | _BV(MSTR) | ((bitOrder == LSBFIRST) ? _BV(DORD) : 0) |
              (dataMode & SPI_MODE_MASK) | ((clockDiv >> 1) & SPI_CLOCK_MASK);
              SPSR = clockDiv & SPI_2XCLOCK_MASK;
              
              // chip select
              digitalWrite(ss, LOW);
              currentSS = ss;
              inTransaction = true;

              return true;
        };

        /**
         * Write to the SPI bus and also receive data.
         */
    uint8_t transfer(uint8_t data) {
        SPDR = data;
        /*
        * The following NOP introduces a small delay that can prevent the wait
        * loop form iterating when running at the maximum speed. This gives
        * about 10% more speed, even if it seems counter-intuitive. At lower
        * speeds it is unnoticed.
        */
        asm volatile("nop");
        while (!(SPSR & _BV(SPIF))) ; // wait
        return SPDR;
    }
  
    uint16_t transfer16(uint16_t data) {
        union { uint16_t val; struct { uint8_t lsb; uint8_t msb; }; } in, out;
        in.val = data;
        if (!(SPCR & _BV(DORD))) {
        SPDR = in.msb;
        asm volatile("nop"); // See transfer(uint8_t) function
        while (!(SPSR & _BV(SPIF))) ;
        out.msb = SPDR;
        SPDR = in.lsb;
        asm volatile("nop");
        while (!(SPSR & _BV(SPIF))) ;
        out.lsb = SPDR;
        } else {
        SPDR = in.lsb;
        asm volatile("nop");
        while (!(SPSR & _BV(SPIF))) ;
        out.lsb = SPDR;
        SPDR = in.msb;
        asm volatile("nop");
        while (!(SPSR & _BV(SPIF))) ;
        out.msb = SPDR;
        }
        return out.val;
    }

    void transfer(void *buf, size_t count) {
        if (count == 0) return;
        uint8_t *p = (uint8_t *)buf;
        SPDR = *p;
        while (--count > 0) {
        uint8_t out = *(p + 1);
        while (!(SPSR & _BV(SPIF))) ;
        uint8_t in = SPDR;
        SPDR = out;
        *p++ = in;
        }
        while (!(SPSR & _BV(SPIF))) ;
        *p = SPDR;
    }

    /**
     * Release the SPI bus and allow others to use the bus.
     */
    void endTransaction(void) {
            if (interruptMode > 0) {
                #ifdef SPI_AVR_EIMSK
                uint8_t sreg = SREG;
                #endif
                noInterrupts();
                #ifdef SPI_AVR_EIMSK
                if (interruptMode == 1) {
                  SPI_AVR_EIMSK = interruptSave;
                  SREG = sreg;
                } else
                #endif
                {
                  SREG = interruptSave;
                }
              }

            digitalWrite(currentSS, HIGH);
            inTransaction = false;
        }

    }
}

