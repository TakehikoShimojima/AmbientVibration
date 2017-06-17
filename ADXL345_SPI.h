#ifndef ADXL345_SPI_H
#define ADXL345_SPI_H

#define DEVID           0x00    //Device ID
#define ACT_INACT_CTL   0x27    // Axis enable control for activity and inactivity detection
#define BW_RATE         0x2C    //Data rate and power mode control
#define POWER_CTL       0x2D    //Power Control Register
#define DATA_FORMAT     0x31    //Data format control
#define DATAX0          0x32    //X-Axis Data 0

class ADXL345
{
public:
    ADXL345(void);

    virtual ~ADXL345();

    void begin(int cs);

    char devid();

    void readAccel(int16_t *x, int16_t *y, int16_t *z);

private:
    void writeRegister(uint8_t registerAddress, uint8_t value);

    void readRegister(uint8_t registerAddress, int numBytes, uint8_t * values);

    int adxl345_cs;
};

#endif // ADXL345_SPI_H

