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

#include "../../inc/MarlinConfigPre.h"

#if ENABLED(MK3_FILAMENT_SENSOR)

#include "PAT9125.h"

bool FilamentSensorBase::operative;
int16_t FilamentSensorBase::x,
        FilamentSensorBase::y;
uint8_t FilamentSensorBase::frame,
        FilamentSensorBase::shutter;

bool FilamentSensorPAT9125::motion_detected;

bool FilamentSensorBase::update(void) {
  if (!operative) return false;

  uint8_t ucMotion = readRegister(PAT9125_MOTION);
  frame = readRegister(PAT9125_FRAME);
  shutter = readRegister(PAT9125_SHUTTER);
  if (!operative) return false;

  if (ucMotion & 0x80) {
    uint8_t ucXL = readRegister(PAT9125_DELTA_XL);
    uint8_t ucYL = readRegister(PAT9125_DELTA_YL);
    uint8_t ucXYH = readRegister(PAT9125_DELTA_XYH);
    if (!operative) return false;

    int16_t iDX = ucXL | ((ucXYH << 4) & 0xf00);
    int16_t iDY = ucYL | ((ucXYH << 8) & 0xf00);
    if (iDX & 0x800) iDX -= 4096;
    if (iDY & 0x800) iDY -= 4096;
    x += iDX;
    y -= iDY; // negative number, because direction switching does not work
  }

  return true;
}

bool FilamentSensorBase::updateY(void) {
  if (!operative) return false;

  uint8_t ucMotion = readRegister(PAT9125_MOTION);
  if (!operative) return false;

  if (ucMotion & 0x80) {
    uint8_t ucYL = readRegister(PAT9125_DELTA_YL);
    uint8_t ucXYH = readRegister(PAT9125_DELTA_XYH);
    if (!operative) return false;

    int16_t iDY = ucYL | ((ucXYH << 8) & 0xf00);
    if (iDY & 0x800) iDY -= 4096;
    y -= iDY; // negative number, because direction switching does not work
  }

  return true;
}

uint8_t FilamentSensorBase::readRegister(const uint8_t addr) {
  Wire.beginTransmission(PAT9125_I2C_ADDR);
  Wire.write(addr);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(PAT9125_I2C_ADDR, 1, true) != 1) {
    operative = false;
    return 0;
  }

  return Wire.read();
}

void FilamentSensorBase::writeRegister(const uint8_t addr, const uint8_t data) {
  Wire.beginTransmission(PAT9125_I2C_ADDR);
  Wire.write(addr);
  Wire.write(data);
  if (Wire.endTransmission() != 0)
    operative = false;
}

#endif // MK3_FILAMENT_SENSOR
