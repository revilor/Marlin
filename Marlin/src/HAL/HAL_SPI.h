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


#ifndef _HAL_SPI_H
#define _HAL_SPI_H


#include <stdint.h>
#include <stdlib.h>


#define SPI_LSBFIRST 0
#define SPI_MSBFIRST 1

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C


namespace HAL {
    namespace SPI {

        /**
        * Initialize and enable the SPI bus.
        */
        void init();

        /**
         * Disable the SPI bus.
         */
        void end();

        /**
         * Gain exclusive access to the SPI bus.
         * 
         * Return:  true    if access to the SPI bus was granted
         *          false   if SPI bus acquisition failed
         */
        bool beginTransaction(uint8_t ss, uint32_t clock, uint8_t bitOrder, uint8_t dataMode);

        /**
         * Write to the SPI bus and also receive data.
         */
        uint8_t transfer(uint8_t data);

        uint16_t transfer16(uint16_t data);

        void transfer(void *buf, size_t count);

        /**
         * Release the SPI bus and allow others to use the bus.
         */
        void endTransaction(void);
    }
}

#endif // _HAL_SPI_H