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
 * This is a simple endstop switch in the path of the filament.
 * It can detect filament runout, but not stripouts or jams.
 */
class FilamentSensorSwitch : public FilamentSensorBase {
  private:
    static inline bool poll_runout_state(const uint8_t extruder) {
      const uint8_t runout_states = poll_runout_states();
      #if NUM_RUNOUT_SENSORS == 1
        UNUSED(extruder);
        return runout_states;                     // A single sensor applying to all extruders
      #else
        #if ENABLED(DUAL_X_CARRIAGE)
          if (dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_SCALED_DUPLICATION_MODE)
            return runout_states;                 // Any extruder
          else
        #elif ENABLED(DUAL_NOZZLE_DUPLICATION_MODE)
          if (extruder_duplication_enabled)
            return runout_states;                 // Any extruder
          else
        #endif
            return TEST(runout_states, extruder); // Specific extruder
      #endif
    }

  public:
    static inline void block_completed(const block_t* const b) { UNUSED(b); }

    static inline void run() {
      const bool out = poll_runout_state(active_extruder);
      if (!out) filament_present(active_extruder);
      #ifdef FILAMENT_RUNOUT_SENSOR_DEBUG
        static bool was_out = false;
        if (out != was_out) {
          was_out = out;
          SERIAL_ECHOPGM("Filament ");
          serialprintPGM(out ? PSTR("OUT\n") : PSTR("IN\n"));
        }
      #endif
    }
};
