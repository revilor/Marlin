
#include "../../Marlin.h"
#include "PN532_HAL_SPI.h"
#include "../../core/serial.h"
#include "../../HAL/SPI.h"

#ifdef ICH_DEBUG
#define DMSG(args...)       SERIAL_ECHO(args)
#define DMSG_STR(str)       SERIAL_ECHO(str)
#define DMSG_HEX(num)       SERIAL_ECHO(' '); SERIAL_ECHO_F((num>>4)&0x0F, HEX); SERIAL_ECHO_F(num&0x0F, HEX)
#define DMSG_INT(num)       SERIAL_ECHO(' '); SERIAL_ECHO(num)
#else
#define DMSG(args...)
#define DMSG_STR(str)
#define DMSG_HEX(num)
#define DMSG_INT(num)
#endif


#define STATUS_READ     2
#define DATA_WRITE      1
#define DATA_READ       3

PN532_HAL_SPI::PN532_HAL_SPI(uint8_t ss)
{
    command = 0;
    _ss  = ss;
}

void PN532_HAL_SPI::begin()
{
    spiBegin();
    spiBeginTransaction(1000000, SPI_LSBFIRST, SPI_DATAMODE_0);
    pinMode(_ss, OUTPUT);
}

void PN532_HAL_SPI::wakeup()
{
    DMSG("Wakeup\n");
    digitalWrite(_ss, LOW);
    delay(2);
    digitalWrite(_ss, HIGH);
    
}



int8_t PN532_HAL_SPI::writeCommand(const uint8_t *header, uint8_t hlen, const uint8_t *body, uint8_t blen)
{
    command = header[0];
    DMSG("writeCommand ");
    writeFrame(header, hlen, body, blen);
    DMSG("\n");

    uint8_t timeout = PN532_ACK_WAIT_TIME;
    while (!isReady()) {
        delay(1);
        timeout--;
        if (0 == timeout) {
            DMSG("Time out when waiting for ACK\n");
            return -2;
        }
    }
    if (readAckFrame()) {
        DMSG("Invalid ACK\n");
        return PN532_INVALID_ACK;
    }
    return 0;
}

int16_t PN532_HAL_SPI::readResponse(uint8_t buf[], uint8_t len, uint16_t timeout)
{
    uint16_t time = 0;
    while (!isReady()) {
        delay(1);
        time++;
        if (timeout > 0 && time > timeout) {
            return PN532_TIMEOUT;
        }
    }

    digitalWrite(_ss, LOW);
    delay(1);

    int16_t result;
    do {
        write(DATA_READ);

        if (0x00 != read()      ||       // PREAMBLE
                0x00 != read()  ||       // STARTCODE1
                0xFF != read()           // STARTCODE2
           ) {

            result = PN532_INVALID_FRAME;
            break;
        }

        uint8_t length = read();
        if (0 != (uint8_t)(length + read())) {   // checksum of length
            result = PN532_INVALID_FRAME;
            break;
        }

        uint8_t cmd = command + 1;               // response command
        if (PN532_PN532TOHOST != read() || (cmd) != read()) {
            result = PN532_INVALID_FRAME;
            break;
        }

        DMSG("read:  ");
        DMSG_HEX(cmd);

        length -= 2;
        if (length > len) {
            for (uint8_t i = 0; i < length; i++) {
                DMSG_HEX(read());                 // dump message
            }
            DMSG("\nNot enough space\n");
            read();
            read();
            result = PN532_NO_SPACE;  // not enough space
            break;
        }

        uint8_t sum = PN532_PN532TOHOST + cmd;
        for (uint8_t i = 0; i < length; i++) {
            buf[i] = read();
            sum += buf[i];

            DMSG_HEX(buf[i]);
        }
        DMSG('\n');

        uint8_t checksum = read();
        if (0 != (uint8_t)(sum + checksum)) {
            DMSG("checksum is not ok\n");
            result = PN532_INVALID_FRAME;
            break;
        }
        read();         // POSTAMBLE

        result = length;
    } while (0);

    digitalWrite(_ss, HIGH);

    return result;
}

bool PN532_HAL_SPI::isReady()
{
    digitalWrite(_ss, LOW);

    write(STATUS_READ);
    uint8_t status = read() & 1;
    digitalWrite(_ss, HIGH);
    return status;
}

void PN532_HAL_SPI::writeFrame(const uint8_t *header, uint8_t hlen, const uint8_t *body, uint8_t blen)
{
    digitalWrite(_ss, LOW);
    delay(2);               // wake up PN532

    write(DATA_WRITE);
    write(PN532_PREAMBLE);
    write(PN532_STARTCODE1);
    write(PN532_STARTCODE2);

    uint8_t length = hlen + blen + 1;   // length of data field: TFI + DATA
    write(length);
    write(~length + 1);         // checksum of length

    write(PN532_HOSTTOPN532);
    uint8_t sum = PN532_HOSTTOPN532;    // sum of TFI + DATA

    DMSG("write: ");

    for (uint8_t i = 0; i < hlen; i++) {
        write(header[i]);
        sum += header[i];

        DMSG_HEX(header[i]);
    }
    for (uint8_t i = 0; i < blen; i++) {
        write(body[i]);
        sum += body[i];

        DMSG_HEX(body[i]);
    }

    uint8_t checksum = ~sum + 1;        // checksum of TFI + DATA
    write(checksum);
    write(PN532_POSTAMBLE);

    digitalWrite(_ss, HIGH);

    DMSG('\n');
}

int8_t PN532_HAL_SPI::readAckFrame()
{
    const uint8_t PN532_ACK[] = {0, 0, 0xFF, 0, 0xFF, 0};

    uint8_t ackBuf[sizeof(PN532_ACK)];

    digitalWrite(_ss, LOW);
    delay(1);
    write(DATA_READ);

    for (uint8_t i = 0; i < sizeof(PN532_ACK); i++) {
        ackBuf[i] = read();
        DMSG_HEX(ackBuf[i]);
    }

    digitalWrite(_ss, HIGH);


    return memcmp(ackBuf, PN532_ACK, sizeof(PN532_ACK));
}


void PN532_HAL_SPI::write(uint8_t data) {
    DMSG_HEX(data);
    spiSend(data);
};

uint8_t PN532_HAL_SPI::read() {
    uint8_t read = spiRec();
    DMSG_HEX(read);
    return read;
//    return spiRec();
}; 
