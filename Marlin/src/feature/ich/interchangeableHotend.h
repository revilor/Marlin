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

#ifndef __ICH_HOTEND_H__
#define __ICH_HOTEND_H__

#include "../../module/thermistor/thermistors.h"
#include "../../inc/MarlinConfig.h"

  extern void* ich_ttbl_map[];
  extern uint8_t ich_ttbllen_map[];
  extern uint8_t ich_ttblid_map[];

  extern char ich_label[];
  extern float ich_nozzle;
//  static char ich_label[13] = "E3D 0.5mm123";
//  static float ich_nozzle = 0.4f;

  #if ENABLED(EEPROM_SETTINGS)
    void readICHTag(uint8_t hotend);
    void writeICHTag(uint8_t hotend);
  #endif
#endif // __ICH_HOTEND_H__
