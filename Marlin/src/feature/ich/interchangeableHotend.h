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

  static void* ich_ttbl_map[5] = { (void*)ICH_0_TEMPTABLE, (void*)ICH_1_TEMPTABLE, (void*)ICH_2_TEMPTABLE, (void*)ICH_3_TEMPTABLE, (void*)ICH_4_TEMPTABLE };
  static uint8_t ich_ttbllen_map[5] = { ICH_0_TEMPTABLE_LEN, ICH_1_TEMPTABLE_LEN, ICH_2_TEMPTABLE_LEN, ICH_3_TEMPTABLE_LEN, ICH_4_TEMPTABLE_LEN };
  static uint8_t ich_ttblid_map[5] = { INTERCHANGEABLE_HOTEND_THERM0, INTERCHANGEABLE_HOTEND_THERM1, INTERCHANGEABLE_HOTEND_THERM2, INTERCHANGEABLE_HOTEND_THERM3, INTERCHANGEABLE_HOTEND_THERM4 };

  static char ich_label[13] = "            "; // "E3D 0.5mm123";
  static float ich_nozzle = 0.5f;
//  static char ich_label[13] = "E3D 0.5mm123";
//  static float ich_nozzle = 0.4f;


  void readICHTag(uint8_t hotend);
  void writeICHTag(uint8_t hotend);
#endif // __ICH_HOTEND_H__
