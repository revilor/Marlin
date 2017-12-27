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

#include "../../../Marlin.h"
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
 * R    test required nozzle vs installed one
 */
void GcodeSuite::M510() {

    if (parser.seen('L')) {
        const char * args = parser.string_arg;
        size_t len = strlen(args);

        SERIAL_ECHOLN(args);
        for (uint8_t i=1; i < len; i++) {
                uint8_t j = 0;
    
                while (i < len) {   
                    SERIAL_ECHO(args[i]);                                       
                    if (args[i] == ' ') {
                        if (j < 12) {
                            ich_label[j] = '\0';
                            SERIAL_ECHO("\\0");                                       
                        }
                        break;
                    }
                    if (j < 12) {
                        ich_label[j] = args[i];
                    } else if (j == 12) {
                        ich_label[j] = '\0';
                        SERIAL_ECHO("\\0");                                       
                    }
                    j++;    
                    i++;
                }
                if (j < 12) {
                    ich_label[j] = '\0';
                    SERIAL_ECHO("\\0");                                       
                }
        
            }  
            SERIAL_ECHOLN(ich_label)  ;
    }

    if (parser.seen('N')) {
        ich_nozzle = (parser.floatval('N', ich_nozzle));
    }

    if (parser.seen('R')) {
            if (parser.floatval('R', -1.0) != ich_nozzle) {
                SERIAL_ERROR_START();
                SERIAL_ERRORPGM(MSG_WRONG_NOZZLE);
                SERIAL_ERRORLN((parser.floatval('R', -1.0)));
                stop();
            }
    }
}

#endif