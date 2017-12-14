#ifndef __DEBUG_H__
#define __DEBUG_H__

#define PN532_DEBUG

#include "Arduino.h"
#include "../../core/serial.h"

#ifdef PN532_DEBUG
#define DMSG(args...)       SERIAL_PROTOCOL(args)
#define DMSG_STR(str)       SERIAL_PROTOCOLLN(str)
#define DMSG_HEX(num)       SERIAL_PROTOCOL(' '); SERIAL_PROTOCOL_F((num>>4)&0x0F, HEX); SERIAL_PROTOCOL_F(num&0x0F, HEX)
#define DMSG_INT(num)       SERIAL_PROTOCOL(' '); SERIAL_PROTOCOL(num)
#else
#define DMSG(args...)
#define DMSG_STR(str)
#define DMSG_HEX(num)
#define DMSG_INT(num)
#endif

#endif
