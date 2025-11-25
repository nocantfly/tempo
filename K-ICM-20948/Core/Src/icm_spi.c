#include "icm_spi.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "kalman_filter.h"

/* ----------------- SPI Basic ------------------ */
extern void ICM20948_WriteReg(ICM20948_t *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg & 0x7F, val };
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(dev->hspi, tx, 2, 100);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

extern uint8_t ICM20948_ReadReg(ICM20948_t *dev, uint8_t reg)
{
    uint8_t tx[2] = { reg | 0x80, 0x00 };
    uint8_t rx[2] = { 0 };
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 2, 100);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
    return rx[1];
}

void ICM20948_SetUserCtrlBits(ICM20948_t *dev, uint8_t mask)
{
    uint8_t val = ICM20948_ReadReg(dev, ICM20948_REG_USER_CTRL);
    val |= mask;
    ICM20948_WriteReg(dev, ICM20948_REG_USER_CTRL, val);
}

void ICM20948_ClearUserCtrlBits(ICM20948_t *dev, uint8_t mask)
{
    uint8_t val = ICM20948_ReadReg(dev, ICM20948_REG_USER_CTRL);
    val &= ~mask;
    ICM20948_WriteReg(dev, ICM20948_REG_USER_CTRL, val);
}

extern void ICM20948_SelectBank(ICM20948_t *dev, uint8_t bank)
{
    ICM20948_WriteReg(dev, ICM20948_BANK_SEL, bank);
}

void ICM20948_ReadRegs(ICM20948_t *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    reg |= 0x80; // Bit 7 = 1 → chế độ đọc SPI

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(dev->hspi, &reg, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(dev->hspi, data, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}


/* ----------------- INIT ------------------ */
void ICM20948_Init(ICM20948_t *dev)
{
    HAL_Delay(100);

    ICM20948_SelectBank(dev, ICM20948_BANK_0);
    /* Reset chip */
    ICM20948_WriteReg(dev, ICM20948_REG_PWR_MGMT_1, 0x80);
    HAL_Delay(100);

    /* Clock source = Auto (PLL) */
    ICM20948_WriteReg(dev, ICM20948_REG_PWR_MGMT_1, 0x01);
    HAL_Delay(10);

    /* Enable Accel + Gyro */
    ICM20948_WriteReg(dev, ICM20948_REG_PWR_MGMT_2, 0x00);
    HAL_Delay(10);

    /* --- BANK 2: Sensor configuration --- */
    ICM20948_SelectBank(dev, ICM20948_BANK_2);

    // ACCEL_CONFIG (DLPF enable, ±16g, BW = 111Hz)
    ICM20948_WriteReg(dev, 0x14, 0x07);

    // GYRO_CONFIG_1 (DLPF enable, ±2000dps, BW = 92Hz)
    ICM20948_WriteReg(dev, 0x01, 0x07);

    // Sample rate divider: 225Hz (1kHz / (1+4))
    ICM20948_SelectBank(dev, ICM20948_BANK_2);
    ICM20948_WriteReg(dev, 0x00, 4);   // GYRO_SMPLRT_DIV = 9  (≈1 kHz / (1 + 9) = 100 Hz)
    ICM20948_WriteReg(dev, 0x11, 4);   // ACCEL_SMPLRT_DIV = 9 (≈1125 Hz / 10 = 112 Hz)

    printf("ICM20948 initialized.\r\n");

    /* --- Reset FIFO --- */
    ICM20948_SelectBank(dev, ICM20948_BANK_0);
    ICM20948_WriteReg(dev, ICM20948_REG_USER_CTRL, 0x00);
    ICM20948_WriteReg(dev, ICM20948_REG_FIFO_RST, 0x1F);
    HAL_Delay(10);

    ICM20948_SetUserCtrlBits(dev, 0x40); // FIFO_EN

}

/* ----------------- FIFO ENABLE ------------------ */
void ICM20948_EnableFIFO(ICM20948_t *dev)
{

    /* --- Enable FIFO + sensor routing --- */
    ICM20948_SelectBank(dev, ICM20948_BANK_0);

    HAL_Delay(10);

    /* FIFO Mode continuous */
    ICM20948_WriteReg(dev, ICM20948_REG_FIFO_MODE, 0x01);

    /* FIFO_EN_1: TEMP + GYRO + ACCEL */
    ICM20948_WriteReg(dev, ICM20948_REG_FIFO_EN_1, 0x00);
    ICM20948_WriteReg(dev, ICM20948_REG_FIFO_EN_2, 0x1F);
    HAL_Delay(10);

    /* --- Reset FIFO counter one last time --- */
    ICM20948_SelectBank(dev, ICM20948_BANK_0);
    ICM20948_WriteReg(dev, ICM20948_REG_FIFO_RST, 0x1F);
    HAL_Delay(10);
    ICM20948_WriteReg(dev, ICM20948_REG_FIFO_RST, 0x00);

    uint8_t fifo_en2 = ICM20948_ReadReg(dev, ICM20948_REG_FIFO_EN_2);
    printf("FIFO_EN_2 = 0x%02X\r\n", fifo_en2);
    HAL_Delay(10);
}



/* ----------------- FIFO READ ------------------ */
void ICM20948_ReadFIFO(ICM20948_t *dev, ICM20948_Data_t *out, Filter_Data_t *fin)
{

    const float accel_sens = 2048.0f;   // ±16g
    const float gyro_sens  = 16.4f;     // ±2000 dps

	ICM20948_SelectBank(dev, 0);

	    // 1️⃣ Đọc FIFO count
	    uint8_t cnt_h = ICM20948_ReadReg(dev, ICM20948_REG_FIFO_COUNTH);
	    uint8_t cnt_l = ICM20948_ReadReg(dev, ICM20948_REG_FIFO_COUNTL);
	    uint16_t fifo_count = ((uint16_t)cnt_h << 8) | cnt_l;

	    if (fifo_count > FIFO_FRAME_SIZE) {



	    // 2️⃣ Đọc đúng 14 byte dữ liệu (1 frame)
	    uint8_t reg = ICM20948_REG_FIFO_R_W | 0x80;
	    uint8_t data[FIFO_FRAME_SIZE] = {0};

	    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
	    HAL_SPI_Transmit(dev->hspi, &reg, 1, 100);
	    HAL_SPI_Receive(dev->hspi, data, FIFO_FRAME_SIZE, 100);
	    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

	    // 3️⃣ Giải mã dữ liệu (theo định dạng FIFO output)
	    out->accel[0] = (int16_t)((data[0] << 8) | data[1])/ accel_sens;
	    out->accel[1] = (int16_t)((data[2] << 8) | data[3])/ accel_sens;
	    out->accel[2] = (int16_t)((data[4] << 8) | data[5])/ accel_sens;
	    out->gyro[0]  = (int16_t)((data[6] << 8) | data[7])/ gyro_sens;
	    out->gyro[1]  = (int16_t)((data[8] << 8) | data[9])/ gyro_sens;
	    out->gyro[2]  = (int16_t)((data[10] << 8) | data[11])/ gyro_sens;
	    out->temp     = (int16_t)((data[12] << 8) | data[13]);
	    }
	    static KalmanFilter_t kf_Gx, kf_Gy, kf_Gz,kf_Ax, kf_Ay, kf_Az;
	    static uint8_t kalman_init = 0;

	    if (!kalman_init) {
	        Kalman_Init(&kf_Gx, 0.0002, 0.0081, 1.5, 0);
	        Kalman_Init(&kf_Gy, 0.0002, 0.0081, 1.5, 0);
	        Kalman_Init(&kf_Gz, 0.0002, 0.0081, 1.5, 0);
	        Kalman_Init(&kf_Ax, 0.0001, 0.0055, 1.5, 0);
	        Kalman_Init(&kf_Ay, 0.0001, 0.0055, 1.5, 0);
	        Kalman_Init(&kf_Az, 0.0001, 0.0055, 1.5, 0);

	        printf("Init Gy: q=%.5f, r=%.5f, p=%.5f, x=%.5f\r\n",
	               kf_Gy.q, kf_Gy.r, kf_Gy.p, kf_Gy.x);
	        kalman_init = 1;
	    }

	    float Gx_flt = Kalman_Update(&kf_Gx, (float)out->gyro[0]);
	    float Gy_flt = Kalman_Update(&kf_Gy, (float)out->gyro[1]);
	    float Gz_flt = Kalman_Update(&kf_Gz, (float)out->gyro[2]);
	    float Ax_flt = Kalman_Update(&kf_Ax, (float)out->accel[0]);
	    float Ay_flt = Kalman_Update(&kf_Ay, (float)out->accel[1]);
	    float Az_flt = Kalman_Update(&kf_Az, (float)out->accel[2]);
	    /*// 5️⃣ Debug hiển thị
	    printf("FIFO: Gx:%d    Gy:%d    Gz:%d    | Ax:%d    Ay:%d    Az:%d    |\r\n",

	           out->gyro[0], out->gyro[1], out->gyro[2],
	           out->accel[0], out->accel[1], out->accel[2]
	          );
	    printf("Fitl: Gx:%.2f Gy:%.2f Gz:%.2f | Ax:%.2f Ay:%.2f Az:%.2f |\r\n",
	    		Gx_flt, Gy_flt, Gz_flt,
				Ax_flt, Ay_flt, Az_flt
	    	          );*/
	    fin-> gyro[0] = Gx_flt;
	    fin-> gyro[1] = Gy_flt;
	    fin-> gyro[2] = Gz_flt;
	    fin->accel[0] = Ax_flt;
	    fin->accel[1] = Ay_flt;
	    fin->accel[2] = Az_flt;

	    // 6️⃣ Reset FIFO (đảm bảo luôn đọc batch mới, không bị lệch)
	    ICM20948_WriteReg(dev, ICM20948_REG_FIFO_RST, 0x1F);
	    HAL_Delay(1);
	    ICM20948_WriteReg(dev, ICM20948_REG_FIFO_RST, 0x00);
	    HAL_Delay(1);

}

/*
void ICM20948_Read_All(ICM20948_t *dev)
{
    uint8_t data[14];

    // Chuyển về Bank 0 để đọc dữ liệu
    ICM20948_SelectBank(dev, ICM20948_BANK_0);

    // Đọc 14 byte bắt đầu từ thanh ghi ACCEL_XOUT_H (0x2D)
    ICM20948_ReadRegs(dev, 0x2D, data, 14);

    // Giải mã dữ liệu
    int16_t accel_x = ((int16_t)data[0] << 8) | data[1];
    int16_t accel_y = ((int16_t)data[2] << 8) | data[3];
    int16_t accel_z = ((int16_t)data[4] << 8) | data[5];
    int16_t gyro_x = ((int16_t)data[6] << 8) | data[7];
    int16_t gyro_y  = ((int16_t)data[8] << 8) | data[9];
    int16_t gyro_z  = ((int16_t)data[10] << 8) | data[11];
    int16_t temp_raw  = ((int16_t)data[12] << 8) | data[13];

    // In giá trị ra UART
    printf("Sens: Gx:%d    Gy:%d    Gz:%d    | Ax:%d    Ay:%d    Az:%d   \r\n",
            gyro_x, gyro_y, gyro_z, accel_x, accel_y, accel_z);
}
*/

void ICM20948_AutoCalibrate(ICM20948_t *dev, ICM20948_Data_t *out, Filter_Data_t *fin, Filter_Data_t *offset, uint16_t samples)
{



    float sum_acc[3] = {0};
    float sum_gyro[3] = {0};
    int valid = 0;

    printf("Starting auto-calibration... Keep the sensor still!\r\n");

    for (int i = 0; i < samples; i++)
    {
        ICM20948_ReadFIFO(dev, out, fin);  // hoặc hàm bạn đang dùng để đọc dữ liệu thô
        HAL_Delay(100);

        float ax = fin->accel[0] ;
        float ay = fin->accel[1] ;
        float az = fin->accel[2] ;
        float gx = fin->gyro[0] ;
        float gy = fin->gyro[1] ;
        float gz = fin->gyro[2] ;

        // Kiểm tra NaN hoặc giá trị quá lớn (nhiễu)
        if (isnanf(ax) || isnanf(ay) || isnanf(az) ||
            isnanf(gx) || isnanf(gy) || isnanf(gz) ||
            fabsf(ax) > 20 || fabsf(ay) > 20 || fabsf(az) > 20)
        {
            printf("⚠️ Invalid sample %d skipped\r\n", i);
            continue;
        }

        sum_acc[0] += ax;
        sum_acc[1] += ay;
        sum_acc[2] += az;
        sum_gyro[0] += gx;
        sum_gyro[1] += gy;
        sum_gyro[2] += gz;
        valid++;

        HAL_Delay(5); // mỗi mẫu cách nhau 5ms
    }

    if (valid == 0)
    {
        printf("❌ No valid samples! Calibration aborted.\r\n");
        return;
    }

    offset->accel[0] = sum_acc[0] / valid;
    offset->accel[1] = sum_acc[1] / valid;
    offset->accel[2] = (sum_acc[2] / valid) - 1.0f; // trừ 1g theo trục Z

    offset->gyro[0] = sum_gyro[0] / valid;
    offset->gyro[1] = sum_gyro[1] / valid;
    offset->gyro[2] = sum_gyro[2] / valid;

    printf("Calibration done with %d valid samples\r\n", valid);
    printf("Accel offset: %.3f, %.3f, %.3f\r\n", offset->accel[0], offset->accel[1], offset->accel[2]);
    printf("Gyro offset : %.3f, %.3f, %.3f\r\n", offset->gyro[0], offset->gyro[1], offset->gyro[2]);
}

void ICM20948_ApplyCalibration(Filter_Data_t *fin, Filter_Data_t *offset)
{
    for (uint8_t i = 0; i < 3; i++) {
        fin->gyro[i]  -= offset->gyro[i];
        fin->accel[i] -= offset->accel[i];
    }
}

