#ifndef __AK09916_H__
#define __AK09916_H__

#include "icm_spi.h"
#include "stm32f4xx_hal.h"

#define I2C_SLV0_ADDR	0x03
#define I2C_SLV0_REG	0x04
#define I2C_SLV0_DO		0x06
#define I2C_SLV0_CTRL   0x05

#define AK09916_ADDR	0x0c
#define MAG_CNTL_2		0x31
#define MAG_CNTL_3		0x32
#define MAG_DATA_ONSET	0x11

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} AK09916_Data_t;

void AK09916_Init(ICM20948_t *dev);
void AK09916_ReadData(ICM20948_t *dev, AK09916_Data_t *data);
void scan_internal_i2c(ICM20948_t *dev);

void AK09916_ReadMagData(ICM20948_t *dev, AK09916_Data_t *data);

void ICM20948_I2CMaster_Enable(ICM20948_t *dev);
void AK09916_SetContinuousMode(ICM20948_t *dev);
void AK09916_ReadMag(ICM20948_t *dev, AK09916_Data_t *mag);
void check(ICM20948_t *dev);

typedef struct
{
	int16_t gyro[3];
	int8_t accel[3];
	int16_t mag[3];
}Compressed_t;
void Compress(Filter_Data_t *out1,AK09916_Data_t *out2,Compressed_t *out3 );
#endif
