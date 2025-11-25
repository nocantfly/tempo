#ifndef __ICM_SPI_H__
#define __ICM_SPI_H__

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdio.h>

/* ======== Cấu hình chân CS ======== */
#define ICM20948_CS_PORT       		GPIOA
#define ICM20948_CS_PIN        		GPIO_PIN_4
#define ICM20948_CS_LOW()      		HAL_GPIO_WritePin(ICM20948_CS_PORT, ICM20948_CS_PIN, GPIO_PIN_RESET)
#define ICM20948_CS_HIGH()     		HAL_GPIO_WritePin(ICM20948_CS_PORT, ICM20948_CS_PIN, GPIO_PIN_SET)

/* ======== Địa chỉ thanh ghi chính ======== */
#define ICM20948_REG_WHO_AM_I      	0x00
#define ICM20948_REG_USER_CTRL     	0x03
#define ICM20948_REG_PWR_MGMT_1    	0x06
#define ICM20948_REG_PWR_MGMT_2    	0x07
#define ICM20948_REG_FIFO_EN_1     	0x66
#define ICM20948_REG_FIFO_EN_2     	0x67
#define ICM20948_REG_FIFO_RST      	0x68
#define ICM20948_REG_FIFO_MODE     	0x69
#define ICM20948_REG_FIFO_COUNTH   	0x70
#define ICM20948_REG_FIFO_COUNTL   	0x71
#define ICM20948_REG_FIFO_R_W      	0x72
#define ICM20948_BANK_SEL          	0x7F

/* -------- Thanh ghi trong Bank 3 liên quan I2C Master -------- */
#define REG_I2C_MST_CTRL       		0x01
#define REG_I2C_SLV0_ADDR      		0x03
#define REG_I2C_SLV0_REG       		0x04
#define REG_I2C_SLV0_CTRL      		0x05
#define REG_I2C_SLV0_DO        		0x06
#define REG_INT_PIN_CFG				0x0F
#define EXT_SLV_SENS_DATA_00		0x3B
#define EXT_SLV_SENS_DATA_08		0x43
#define I2C_MST_STATUS_REG 			0x17
#define REG_LP_CONFIG				0x05


/* -------- Thanh ghi đọc dữ liệu mag ở Bank 0 -------- */
#define REG_EXT_SLV_SENS_DATA_00  	0x3B  // bắt đầu vùng dữ liệu đọc I2C slave

/* -------- Địa chỉ AK09916 -------- */
#define AK09916_I2C_ADDR       		0x0C
#define AK09916_WHO_AM_I       		0x01
#define AK09916_ST1            		0x10
#define AK09916_HXL            		0x11
#define AK09916_CNTL2          		0x31
#define AK09916_CNTL3          		0x32


/* ======== Bank định danh ======== */
#define ICM20948_BANK_0    			0x00
#define ICM20948_BANK_1    			0x10
#define ICM20948_BANK_2    			0x20
#define ICM20948_BANK_3    			0x30

/* ======== FIFO_DEFAULT ============ */
#define FIFO_FRAME_SIZE 			14

/* ======== Cấu trúc dữ liệu ======== */
typedef struct {
    SPI_HandleTypeDef *hspi;
    UART_HandleTypeDef *huart;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} ICM20948_t;

typedef struct {
    int16_t accel[3];
    int16_t gyro[3];
    int16_t temp;
} ICM20948_Data_t;

typedef struct {
	float gyro[3];
	float accel[3];
}Filter_Data_t;


/* ======== Prototype các hàm ======== */
void ICM20948_Init(ICM20948_t *dev);
void ICM20948_WriteReg(ICM20948_t *dev, uint8_t reg, uint8_t val);
uint8_t ICM20948_ReadReg(ICM20948_t *dev, uint8_t reg);
void ICM20948_ReadRegs(ICM20948_t *dev, uint8_t reg, uint8_t *data, uint16_t len);
void ICM20948_Read_All(ICM20948_t *dev);


void ICM20948_EnableFIFO(ICM20948_t *dev);

void ICM20948_ReadFIFO(ICM20948_t *dev, ICM20948_Data_t *out, Filter_Data_t *fin);

/* ======== Hàm SPI thấp cấp ======== */
void ICM20948_SelectBank(ICM20948_t *dev, uint8_t bank);

/*void print_and_fix_gyro_config(ICM20948_t *dev);*/

void ICM20948_ClearUserCtrlBits(ICM20948_t *dev, uint8_t mask);
void ICM20948_SetUserCtrlBits(ICM20948_t *dev, uint8_t mask);
void ICM20948_AutoCalibrate(ICM20948_t *dev, ICM20948_Data_t *out, Filter_Data_t *fin, Filter_Data_t *offset, uint16_t samples);
void ICM20948_ApplyCalibration(Filter_Data_t *fin, Filter_Data_t *offset);
#endif /* __ICM_SPI_H__ */
