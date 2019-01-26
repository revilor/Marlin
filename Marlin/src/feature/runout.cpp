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

/**
 * feature/runout.cpp - Runout sensor support
 */

#include "../inc/MarlinConfigPre.h"

#if ENABLED(FILAMENT_RUNOUT_SENSOR)

#include "runout.h"

FilamentMonitor runout;

bool FilamentMonitorBase::enabled = true,
     FilamentMonitorBase::filament_ran_out; // = false

/**
 * Called by FilamentSensorPAT9125::block_completed when motion is detected.
 * Called by FilamentSensorSwitch::run when filament is detected.
 * Called by FilamentSensorEncoder::block_completed when motion is detected.
 */
void FilamentSensorBase::filament_present(const uint8_t extruder) {
  runout.filament_present(extruder); // calls response.filament_present(extruder)
}

#if FILAMENT_RUNOUT_DISTANCE_MM > 0
  float RunoutResponseDelayed::runout_distance_mm = FILAMENT_RUNOUT_DISTANCE_MM;
  volatile float RunoutResponseDelayed::runout_mm_countdown[EXTRUDERS];
#elif ENABLED(MK3_FILAMENT_SENSOR)
  volatile bool RunoutResponsePAT9125::runout;
  volatile int16_t FilamentSensorPAT9125::oldY;
  volatile uint16_t FilamentSensorPAT9125::e_steps;
#else
  int8_t RunoutResponseDebounced::runout_count; // = 0
#endif

#endif // FILAMENT_RUNOUT_SENSOR
