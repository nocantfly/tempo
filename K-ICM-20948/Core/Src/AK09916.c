#include "AK09916.h"
#include "icm_spi.h"
/*static void ak09916_write_reg(int8_t reg, uint16_t data,ICM20948_t *dev )
{
    ICM20948_SelectBank(dev, ICM20948_BANK_3);
    ICM20948_WriteReg(dev, I2C_SLV0_ADDR, AK09916_ADDR);  // write mode
    ICM20948_WriteReg(dev, I2C_SLV0_REG, reg);
    ICM20948_WriteReg(dev, I2C_SLV0_DO, data);
    ICM20948_WriteReg(dev, I2C_SLV0_CTRL, 0x81); // enable, length=1
    HAL_Delay(10);  // chỉ cần 10ms
}*/

/*static void ak09916_read_reg(int8_t onset_reg, uint16_t len,ICM20948_t *dev )
{
	ICM20948_SelectBank(dev, ICM20948_BANK_3);
	ICM20948_WriteReg(dev, I2C_SLV0_ADDR, 0x80|AK09916_ADDR);
	ICM20948_WriteReg(dev, I2C_SLV0_REG, onset_reg);
	ICM20948_WriteReg(dev, I2C_SLV0_CTRL, 0x80|len);
	HAL_Delay(50);
}*/

void AK09916_SetContinuousMode(ICM20948_t *dev)
{
    ICM20948_SelectBank(dev, ICM20948_BANK_3);
    ICM20948_WriteReg(dev, REG_I2C_SLV0_ADDR, AK09916_ADDR); // write
    ICM20948_WriteReg(dev, REG_I2C_SLV0_REG, 0x31); // CNTL2
    ICM20948_WriteReg(dev, REG_I2C_SLV0_DO, 0x08); // Continuous 10Hz
    ICM20948_WriteReg(dev, REG_I2C_SLV0_CTRL, 0x81);
    HAL_Delay(50);
}


void AK09916_Init(ICM20948_t *dev)
{
    // Enable I2C master
    ICM20948_SelectBank(dev, ICM20948_BANK_0);
    ICM20948_SetUserCtrlBits(dev, 0x02);
    HAL_Delay(10);
    ICM20948_ClearUserCtrlBits(dev, 0x02);
    HAL_Delay(10);
    ICM20948_SetUserCtrlBits(dev, 0x20);

    HAL_Delay(10);
    ICM20948_SelectBank(dev, ICM20948_BANK_3);
    ICM20948_WriteReg(dev, REG_I2C_MST_CTRL, 0x07); // I2C_MST_CTRL: 400kHz
    HAL_Delay(10);
    ICM20948_SelectBank(dev, ICM20948_BANK_0);
    HAL_Delay(10);
    ICM20948_WriteReg(dev, 0x05, 0x40);
    HAL_Delay(10);
    ICM20948_SelectBank(dev, ICM20948_BANK_3);
    ICM20948_WriteReg(dev, 0x00, 0x03);
    HAL_Delay(10);
    ICM20948_SelectBank(dev, ICM20948_BANK_3);
    ICM20948_WriteReg(dev, REG_I2C_SLV0_ADDR, 0x8C); // Read mode
    ICM20948_WriteReg(dev, REG_I2C_SLV0_REG, 0x01);  // WHO_AM_I register
    ICM20948_WriteReg(dev, REG_I2C_SLV0_CTRL, 0x81); // enable, 1 byte
    HAL_Delay(10);
    ICM20948_SelectBank(dev, ICM20948_BANK_0);
    uint8_t whoami;
    whoami = ICM20948_ReadReg(dev, REG_EXT_SLV_SENS_DATA_00);
    printf("whoami: 0x%02X\r\n", whoami);
    AK09916_SetContinuousMode(dev);

}

void AK09916_ReadMagData(ICM20948_t *dev, AK09916_Data_t *data)
{

    // Cấu hình Slave0 để đọc từ AK09916
    ICM20948_SelectBank(dev, ICM20948_BANK_3);
    ICM20948_WriteReg(dev, I2C_SLV0_ADDR, 0x80 | 0x0C); // read mode
    ICM20948_WriteReg(dev, I2C_SLV0_REG, 0x10);         // start at ST1
    ICM20948_WriteReg(dev, I2C_SLV0_CTRL, 0x89);        // enable + 7 bytes
    HAL_Delay(50);

    ICM20948_SelectBank(dev, ICM20948_BANK_0);

    uint8_t ST1 = ICM20948_ReadReg(dev, EXT_SLV_SENS_DATA_00);
    printf("ST1 = 0x%02X\r\n", ST1);

    if (ST1 & 0x01)
    {
    	HAL_Delay(1);
		uint16_t raw[7];
		for (int i=1; i <= 6; i++)
		{
			raw[i]= ICM20948_ReadReg(dev, EXT_SLV_SENS_DATA_00 +i );
		}

		data->x = (int16_t)((raw[2] << 8) | raw[1]);
		data->y = (int16_t)((raw[4] << 8) | raw[3]);
		data->z = (int16_t)((raw[6] << 8) | raw[5]);
		printf("MAG: X=%d | Y=%d | Z=%d\r\n", data->x, data->y, data->z);

		HAL_Delay(50); // delay 100ms (10Hz)
		ICM20948_SelectBank(dev, ICM20948_BANK_3);
		ICM20948_WriteReg(dev, I2C_SLV0_ADDR, 0x80 | 0x0C); // read mode
		ICM20948_WriteReg(dev, I2C_SLV0_REG, 0x10);         // start at ST1
		ICM20948_WriteReg(dev, I2C_SLV0_CTRL, 0x87);        // enable + 7 bytes
		HAL_Delay(50);

		ICM20948_SelectBank(dev, ICM20948_BANK_0);

		uint8_t ST2 = ICM20948_ReadReg(dev, EXT_SLV_SENS_DATA_08);
		printf("ST2 = 0x%02X\r\n", ST2);
    }
}
void Compress(Filter_Data_t *out1,AK09916_Data_t *out2,Compressed_t *out3 )
{
	for(int i=0;i<3;i++)
	{
		out3->gyro[i] = out1->gyro[i]*10;
		out3->accel[i] = out1->accel[i]*10;
	}
out3->mag[0] = out2->x*10;
out3->mag[1] = out2->y*10;
out3->mag[2] = out2->z*10;
}
