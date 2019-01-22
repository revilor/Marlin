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

#include "DIOFilamentSensorBase.h"

/**
 * This sensor uses a magnetic encoder disc and a Hall effect
 * sensor (or a slotted disc and optical sensor). The state
 * will toggle between 0 and 1 on filament movement. It can detect
 * filament runout and stripouts or jams.
 */
class FilamentSensorEncoder : public FilamentSensorBase {
  private:
    static uint8_t motion_detected;

    static inline void poll_motion_sensor() {
      static uint8_t old_state;
      const uint8_t new_state = poll_runout_pins(),
                    change    = old_state ^ new_state;
      old_state = new_state;

      #ifdef FILAMENT_RUNOUT_SENSOR_DEBUG
        if (change) {
          SERIAL_ECHOPGM("Motion detected:");
          for (uint8_t e = 0; e < NUM_RUNOUT_SENSORS; e++)
            if (TEST(change, e)) { SERIAL_CHAR(' '); SERIAL_CHAR('0' + e); }
          SERIAL_EOL();
        }
      #endif

      motion_detected |= change;
    }

  public:
    static inline void block_completed(const block_t* const b) {
      // If the sensor wheel has moved since the last call to
      // this method reset the runout counter for the extruder.
      if (TEST(motion_detected, b->extruder))
        filament_present(b->extruder);

      // Clear motion triggers for next block
      motion_detected = 0;
    }

    static inline void run() { poll_motion_sensor(); }
};
