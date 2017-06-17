#include <ESP8266WiFi.h>
#include <SPI.h>
#include "ADXL345_SPI.h"

ADXL345::ADXL345() {
}

ADXL345::~ADXL345() {
}

void ADXL345::begin(int cs) {
    uint8_t _b;
    SPI.setDataMode(SPI_MODE2);

    adxl345_cs = cs;
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);

    writeRegister(POWER_CTL, 0x00);  // 
    writeRegister(POWER_CTL, 0x10);  // 
    writeRegister(POWER_CTL, 0x08);  // 測定モード

    readRegister(DATA_FORMAT, 1, &_b);
    writeRegister(DATA_FORMAT, ((_b & B11101100)) | B00001011); // ±16g 最大分解能モード
}

char ADXL345::devid() {
    uint8_t value;

    readRegister(DEVID, 1, &value);
    return value;
}

void ADXL345::readAccel(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t values[6];

    readRegister(DATAX0, 6, values);

    *x  = ((int16_t)values[1]<<8)|(int16_t)values[0];
    *y  = ((int16_t)values[3]<<8)|(int16_t)values[2];
    *z  = ((int16_t)values[5]<<8)|(int16_t)values[4];
}

void ADXL345::writeRegister(uint8_t registerAddress, uint8_t value) {
    digitalWrite(adxl345_cs, LOW);
    SPI.transfer(registerAddress);
    SPI.transfer(value);
    digitalWrite(adxl345_cs, HIGH);
}

void ADXL345::readRegister(uint8_t registerAddress, int numBytes, uint8_t * values) {
    uint8_t address = 0x80 | registerAddress;
    if(numBytes > 1)address = address | 0x40;
    digitalWrite(adxl345_cs, LOW);
    SPI.transfer(address);
    for(int i=0; i<numBytes; i++) {
        values[i] = SPI.transfer(0x00);
    }
    digitalWrite(adxl345_cs, HIGH);
}

