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
#include "../../inc/MarlinConfig.h"
#include "../../Marlin.h"
#include "../../core/utility.h"
#include "../../HAL/persistent_store_api.h"
#include <stdint.h>
#include "interchangeableHotend.h"
#include "../../core/serial.h"
#include "PN532_HAL_SPI.h"
#include <SPI.h>
// #include "../../libs/PN532_SPI/PN532_SPI.h"
#include "../../libs/PN532/PN532.h"
#include "../../module/temperature.h"

#include "../../HAL/SPI.h"

#if HAS_BED_PROBE
  #include "../module/probe.h"
#endif

#define NFC_EEPROM_VERSION 0x01

/**
 * V1 Mifare UL EEPROM Layout:
 *  
 *  Page + Offset in page
 *  4 + 0  Version                                  (uint8 _t)
 *  4 + 1  EEPROM CRC16                             (uint16_t)
 *  4 + 3  Thermistor ID                            (uint8_t)
 * 
 *  5 + 0  Label                                    (char x 12)
 *  6 + 0
 *  7 + 0
 * 
 *  8 + 0  Nozzle diameter                          (float)
 * 
 * HAS_BED_PROBE:                                   4 bytes
 *  9 + 0  M851      zprobe_zoffset                 (float)
 * 
 * DELTA:                                           4 bytes
 *  10 + 0  M666 H    delta_height                   (float)
 *
 * PIDTEMP:                                         16 bytes
 *  11 + 0  M301 En PIDC  Kp                         (float)
 *  12 + 0  M301 En PIDC  Ki                         (float)
 *  13 + 0  M301 En PIDC Kd                         (float)
 *  14 + 0  M301 En PIDC  Kc                        (float)
 * 
 * 
 */

#if ENABLED(INTERCHANGEABLE_HOTEND)

  #if ENABLED(EEPROM_SETTINGS)


void* ich_ttbl_map[5] = { (void*)ICH_0_TEMPTABLE, (void*)ICH_1_TEMPTABLE, (void*)ICH_2_TEMPTABLE, (void*)ICH_3_TEMPTABLE, (void*)ICH_4_TEMPTABLE };
uint8_t ich_ttbllen_map[5] = { ICH_0_TEMPTABLE_LEN, ICH_1_TEMPTABLE_LEN, ICH_2_TEMPTABLE_LEN, ICH_3_TEMPTABLE_LEN, ICH_4_TEMPTABLE_LEN };
uint8_t ich_ttblid_map[5] = { INTERCHANGEABLE_HOTEND_THERM0, INTERCHANGEABLE_HOTEND_THERM1, INTERCHANGEABLE_HOTEND_THERM2, INTERCHANGEABLE_HOTEND_THERM3, INTERCHANGEABLE_HOTEND_THERM4 };

char ich_label[13] = "            "; 
float ich_nozzle = 0.5f;

  
  //PN532_SPI pn532_marlin0(SPI, INTERCHANGEABLE_HOTEND0_CS);
  PN532_HAL_SPI pn532_marlin0(INTERCHANGEABLE_HOTEND0_CS);
  PN532 nfc0(pn532_marlin0);



  void dumpICHTag(uint8_t hotend) {
  
      // TODO: handle multiple hotends?
  SET_OUTPUT(INTERCHANGEABLE_HOTEND0_CS);
  
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
          SERIAL_ECHOLN("Reading pages");
          uint8_t data[32];
          for (uint8_t i = 4; i < 16; i++) {
          success = nfc0.mifareultralight_ReadPage (i, data);
          if (success) {
            // Data seems to have been read ... spit it out
            nfc0.PrintHexChar(data, 4);
            SERIAL_ECHOLN("");
          } else {
            SERIAL_ECHOLN("Ooops ... unable to read the requested page!?");
          }
        }
        }
    
        SERIAL_ECHOLN("");
        // Wait 1 second before continuing
    } else {
        // PN532 probably timed out waiting for a card
        SERIAL_ECHOLN("Timed out waiting for a card");
    }
  
  //    spiEndTransaction();
  }
  


  void readPage(uint8_t page, uint8_t *value, uint16_t *crc) {
    uint8_t size = 4; // 4 bytes per page
    uint8_t * val = value;
    uint8_t data[32];
    
    nfc0.mifareultralight_ReadPage(page, data);
    
    for (uint8_t i=0; i < size; i++) {
      *val = data[i];
      crc16(crc, val, 1);
      val++;
    };
  

  }
  
  void writePage(uint8_t page, uint8_t *value, uint16_t *crc) {
    uint8_t size = 4; // 4 bytes per page
    uint8_t * val = value;
  
    while (size--) {
      uint8_t v = *val;
      crc16(crc, &v, 1);
      val++;
    };
  
    nfc0.mifareultralight_WritePage(page, value);
  }
  

void readICHTag(uint8_t hotend) {

  // TODO: handle multiple hotends?
  SET_OUTPUT(INTERCHANGEABLE_HOTEND0_CS);
  
    nfc0.begin();
    nfc0.setPassiveActivationRetries(0xFF);
    nfc0.SAMConfig();
    
  
    bool success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
      
    success = nfc0.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
    if (! success) {
      // no tag present
      // TODO: should this cause an error?
      SERIAL_ECHOLN("No NFC tag");
      return;
    }

    #if ENABLED(DEBUG_ICH)
      dumpICHTag(hotend);
    #endif
    
    uint16_t working_crc = 0;
    uint16_t stored_crc = 0;
    uint8_t page4[4];
    
    readPage(4, (uint8_t*)&page4, &working_crc);
    working_crc = 0;

    uint8_t version = page4[0];
    stored_crc |= page4[1] << 8;
    stored_crc |= page4[2];
    uint8_t thermistorIndex = page4[3];

    // TODO where to store thermistor setting?

    if (version != NFC_EEPROM_VERSION) {
        SERIAL_ECHO("NFC storage version mismatch: ");
        SERIAL_ECHO(version);
        SERIAL_ECHO(" != ");
        SERIAL_ECHOLN(NFC_EEPROM_VERSION);
//        return;
    }
  
    readPage(5, (uint8_t*)&ich_label, &working_crc);
    readPage(6, ((uint8_t*)&ich_label) + 4, &working_crc);
    readPage(7, ((uint8_t*)&ich_label) + 8, &working_crc);
    readPage(8, (uint8_t*)&ich_nozzle, &working_crc);
  
    #if !HAS_BED_PROBE
      const float zprobe_zoffset = 0;
    #endif
  
    readPage(9, (uint8_t*)&zprobe_zoffset, &working_crc);
  
    #if ENABLED(DELTA)
    readPage(10, &delta_height, working_crc);
    #endif
  
    readPage(11, (uint8_t*)&PID_PARAM(Kp, hotend), &working_crc);
    readPage(12, (uint8_t*)&PID_PARAM(Ki, hotend), &working_crc);
    readPage(13, (uint8_t*)&PID_PARAM(Kd, hotend), &working_crc);
    #if ENABLED(PID_EXTRUSION_SCALING)
    readPage(14, (uint8_t*)&PID_PARAM(Kc, hotend), working_crc);
    #else
      const float dummy = 1.0f; // 1.0 = default kc
      readPage(14, (uint8_t*)&dummy, &working_crc);
    #endif
  
    crc16(&working_crc, (uint8_t*)&thermistorIndex, 1);
    
    if (working_crc != stored_crc) {
      SERIAL_ECHO("NFC storage CRC mismatch: ");
      SERIAL_ECHO(working_crc);
      SERIAL_ECHO(" != ");
      SERIAL_ECHOLN(stored_crc);
    }
  }
  

void writeICHTag(uint8_t hotend) {
  // TODO: handle multiple hotends?
  SET_OUTPUT(INTERCHANGEABLE_HOTEND0_CS);

  nfc0.begin();
  nfc0.setPassiveActivationRetries(0xFF);
  nfc0.SAMConfig();

  

  bool success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  success = nfc0.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (! success) {
    // no tag present
    // TODO: should this cause an error?
    SERIAL_ECHOLN("No NFC tag");    
    return;
  }

  uint16_t working_crc = 0;
  // TODO where to get thermistor setting?
  uint8_t page4[4] = {NFC_EEPROM_VERSION, 0, 0, ich_ttblid_map[hotend]};

  writePage(5, (uint8_t*)&ich_label, &working_crc);
  writePage(6, ((uint8_t*)&ich_label) + 4, &working_crc);
  writePage(7, ((uint8_t*)&ich_label) + 8, &working_crc);
  writePage(8, (uint8_t*)&ich_nozzle, &working_crc);

  #if !HAS_BED_PROBE
    const float zprobe_zoffset = 0;
  #endif

  writePage(9, (uint8_t*)&zprobe_zoffset, &working_crc);

  #if ENABLED(DELTA)
    writePage(10, &delta_height, working_crc);
  #endif

  writePage(11, (uint8_t*)&PID_PARAM(Kp, hotend), &working_crc);
  writePage(12, (uint8_t*)&PID_PARAM(Ki, hotend), &working_crc);
  writePage(13, (uint8_t*)&PID_PARAM(Kd, hotend), &working_crc);
  #if ENABLED(PID_EXTRUSION_SCALING)
    writePage(14, (uint8_t*)&PID_PARAM(Kc, hotend), working_crc);
  #else
    const float dummy = 1.0f; // 1.0 = default kc
    writePage(14, (uint8_t*)&dummy, &working_crc);
  #endif

  crc16(&working_crc, (uint8_t*)&ich_ttblid_map[hotend], 1);
  page4[1] =  (uint8_t)((working_crc >> 8) & 0XFF);
  page4[2] = (uint8_t)((working_crc & 0XFF));

  writePage(4, (uint8_t*)&page4, &working_crc);
}


  #endif
#endif
