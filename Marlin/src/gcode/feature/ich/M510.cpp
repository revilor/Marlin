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

#include "../../gcode.h"
#include "../../parser.h"
#include "../../../feature/ich/interchangeableHotend.h"
#include "../../../inc/MarlinConfig.h"
#include "../../../core/serial.h"

#if ENABLED(INTERCHANGEABLE_HOTEND)
/**
 * M510: Set hotend label and nozzle info
 * L    label
 * N    nozzle
 */
void GcodeSuite::M510() {
    char * args = parser.string_arg;
    size_t len = strlen(args);

    for (uint8_t i=0; i < len; i++) {
        if (args[i] == 'L') {
            // read label
            i++;
            uint8_t j = 0;

            while (i < len) {                                          
                if (args[i] == ' ') {
                    if (j < 12) {
                        ich_label[j] = '\0';
                    }
                    break;
                }
                if (j < 12) {
                    ich_label[j] = args[i];
                } else if (j == 12) {
                    ich_label[j] = '\0';
                }
                j++;    
                i++;
            }
        } else if (args[i] == 'N') {
            ich_nozzle = strtod(args + i + 1, NULL);
        }
    }
}

#endif