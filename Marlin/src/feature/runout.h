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

/**
 * feature/runout.h - Runout sensor support
 */

#define FILAMENT_RUNOUT_SENSOR_DEBUG

#include "../sd/cardreader.h"
#include "../module/printcounter.h"
#include "../module/stepper.h"

#if ENABLED(EXTENSIBLE_UI)
  #include "../lcd/extensible_ui/ui_api.h"
#endif

#if ENABLED(MK3_FILAMENT_SENSOR)
  #include "runout/PAT9125.h"
#elif ENABLED(FILAMENT_MOTION_SENSOR)
  #include "runout/EncoderSensor.h"
#else
  #include "runout/SwitchSensor.h"
#endif

class FilamentMonitorBase {
  public:
    static bool enabled;
    static bool filament_ran_out;
};

template<class RESPONSE_T, class SENSOR_T>
class TFilamentMonitor : public FilamentMonitorBase {
  private:
    typedef RESPONSE_T response_t;
    typedef SENSOR_T   sensor_t;
    static  response_t response;
    static  sensor_t   sensor;

  public:
    static inline void setup() {
      sensor.setup();
      reset();
    }

    static inline void reset() {
      filament_ran_out = false;
      response.reset();
    }

    // Call this method when filament is present,
    // so the response can reset its counter.
    static inline void filament_present(const uint8_t extruder) {
      response.filament_present(extruder);
    }

    // Handle a block completion. RunoutResponseDelayed uses this to
    // add up the length of filament moved while the filament is out.
    static inline void block_completed(const block_t* const b) {
      if (enabled) {
        response.block_completed(b);
        sensor.block_completed(b);
      }
    }

    // Give the response a chance to update its counter.
    static inline void run() {
      if (enabled && !filament_ran_out && (IS_SD_PRINTING() || print_job_timer.isRunning())) {
        #if FILAMENT_RUNOUT_DISTANCE_MM > 0
          cli(); // Prevent RunoutResponseDelayed::block_completed from accumulating here
        #endif
        response.run();
        sensor.run();
        const bool ran_out = response.has_run_out();
        #if FILAMENT_RUNOUT_DISTANCE_MM > 0
          sei();
        #endif
        if (ran_out) {
          filament_ran_out = true;
          #if ENABLED(EXTENSIBLE_UI)
            ExtUI::onFilamentRunout();
          #endif
          enqueue_and_echo_commands_P(PSTR(FILAMENT_RUNOUT_SCRIPT));
          planner.synchronize();
        }
      }
    }
};

/********************************* RESPONSE TYPE *********************************/

#if FILAMENT_RUNOUT_DISTANCE_MM > 0

  // RunoutResponseDelayed triggers a runout event only if the length
  // of filament specified by FILAMENT_RUNOUT_DISTANCE_MM has been fed
  // during a runout condition.
  class RunoutResponseDelayed {
    private:
      static volatile float runout_mm_countdown[EXTRUDERS];

    public:
      static float runout_distance_mm;

      static inline void reset() {
        LOOP_L_N(i, EXTRUDERS) filament_present(i);
      }

      static inline void run() {
        #ifdef FILAMENT_RUNOUT_SENSOR_DEBUG
          static millis_t t = 0;
          const millis_t ms = millis();
          if (ELAPSED(ms, t)) {
            t = millis() + 1000UL;
            LOOP_L_N(i, EXTRUDERS) {
              serialprintPGM(i ? PSTR(", ") : PSTR("Remaining mm: "));
              SERIAL_ECHO(runout_mm_countdown[i]);
            }
            SERIAL_EOL();
          }
        #endif
      }

      static inline bool has_run_out() {
        return runout_mm_countdown[active_extruder] < 0;
      }

      static inline void filament_present(const uint8_t extruder) {
        runout_mm_countdown[extruder] = runout_distance_mm;
      }

      static inline void block_completed(const block_t* const b) {
        if (b->steps[X_AXIS] || b->steps[Y_AXIS] || b->steps[Z_AXIS]) {
          // Only trigger on extrusion with XYZ movement to allow filament change and retract/recover.
          const uint8_t e = b->extruder;
          const int32_t steps = b->steps[E_AXIS];
          runout_mm_countdown[e] -= (TEST(b->direction_bits, E_AXIS) ? -steps : steps) * planner.steps_to_mm[E_AXIS_N(e)];
        }
      }
  };

#else // !FILAMENT_RUNOUT_DISTANCE_MM

  // RunoutResponseDebounced triggers a runout event after a runout
  // condition has been detected runout_threshold times in a row.

  class RunoutResponseDebounced {
    private:
      static constexpr int8_t runout_threshold = 5;
      static int8_t runout_count;
    public:
      static inline void reset()                                  { runout_count = runout_threshold; }
      static inline void run()                                    { runout_count--; }
      static inline bool has_run_out()                            { return runout_count < 0; }
      static inline void block_completed(const block_t* const b)  { UNUSED(b); }
      static inline void filament_present(const uint8_t extruder) { runout_count = runout_threshold; UNUSED(extruder); }
  };

#endif // !FILAMENT_RUNOUT_DISTANCE_MM

/********************************* TEMPLATE SPECIALIZATION *********************************/

typedef TFilamentMonitor<
  #if FILAMENT_RUNOUT_DISTANCE_MM > 0
    RunoutResponseDelayed,
  #else
    RunoutResponseDebounced,
  #endif
  #if ENABLED(MK3_FILAMENT_SENSOR)
    FilamentSensorPAT9125
  #elif ENABLED(FILAMENT_MOTION_SENSOR)
    FilamentSensorEncoder
  #else
    FilamentSensorSwitch
  #endif
> FilamentMonitor;

extern FilamentMonitor runout;
