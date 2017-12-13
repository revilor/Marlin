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
#include "../../Marlin.h"
#include "../../inc/MarlinConfig.h"
#include <stdint.h>
#include "interchangeableHotend.h"
#include "../../core/serial.h"
#include "PN532_Marlin.h"
#include <PN532.h>
#include "../../HAL/SPI.h"

  // TODO: handle multiple hotends
  //SET_OUTPUT(INTERCHANGEABLE_HOTEND0_CS);
  
  PN532_Marlin pn532_marlin0(INTERCHANGEABLE_HOTEND0_CS);
  PN532 nfc0(pn532_marlin0);


void readICHTag(uint8_t hotend) {
    // TODO: support multiple hotends
    nfc0.begin();
    nfc0.setPassiveActivationRetries(0xFF);
    
    // configure board to read RFID tags
    nfc0.SAMConfig();


    bool success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
      // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
      // 'uid' will be populated with the UID, and uidLength will indicate
      // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc0.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

    SERIAL_ECHOLN("NFC target ID");
    
    SERIAL_ECHOLN(success);

    if (success) {
        SERIAL_ECHOLN("Found a card!");
        SERIAL_ECHO("UID Length: ");SERIAL_PROTOCOL_F(uidLength, DEC);SERIAL_ECHO(" bytes");
        SERIAL_ECHO("UID Value: ");
        for (uint8_t i=0; i < uidLength; i++) {
            SERIAL_ECHO(" 0x");SERIAL_PROTOCOL_F(uid[i], HEX);
        }
    
    
        if (uidLength == 7) {
          // We probably have a Mifare Ultralight card ...
          SERIAL_ECHOLN("Seems to be a Mifare Ultralight tag (7 byte UID)");
    
          // Try to read the first general-purpose user page (#4)
          SERIAL_ECHOLN("Reading page 4");
          uint8_t data[32];
          success = nfc0.mifareultralight_ReadPage (4, data);
          if (success) {
            // Data seems to have been read ... spit it out
            nfc0.PrintHexChar(data, 4);
            SERIAL_ECHOLN("");
          } else {
            SERIAL_ECHOLN("Ooops ... unable to read the requested page!?");
          }
        }
    
        SERIAL_ECHOLN("");
        // Wait 1 second before continuing
    } else {
        // PN532 probably timed out waiting for a card
        SERIAL_ECHOLN("Timed out waiting for a card");
    }

    spiEndTransaction();
}
      
