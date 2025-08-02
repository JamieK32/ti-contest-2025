#include "lsm6dsv16x.h"
#include "lsm6dsv16x_reg.h"
#include "string.h"
#include "delay.h"
#include "hal_soft_i2c.h"  // 包含你的软件I2C头文件

#define M_PI 3.14

#define BOOT_TIME         (10)
#define I2C_TIMEOUT_MS    (10)
#define RAD_TO_DEG        (180.0f / M_PI)

#define LSM6DSV16X_ADDR   (0x6A)

// 使用你的全局软件I2C实例
soft_iic_info_struct lsm6dsv16x_i2c = {
    .sclIOMUX = PORTA_SCL2_IOMUX,
		.sclPin = PORTA_SCL2_PIN,
    .sclPort = PORTA_PORT,
    .sdaIOMUX = PORTA_SDA2_IOMUX,
    .sdaPin = PORTA_SDA2_PIN,
    .sdaPort = PORTA_PORT,
    .delay_time = 1,
    .addr = 0x6A  // LSM6DSV16X地址
};
static uint8_t whoamI;
static lsm6dsv16x_fifo_sflp_raw_t fifo_sflp;

lsm6dsv16x_fifo_status_t fifo_status;
stmdev_ctx_t dev_ctx;
lsm6dsv16x_reset_t rst;
lsm6dsv16x_pin_int_route_t pin_int;

float_t lsm6dsv16x_quat[4];
short lsm6dsv16x_gyro[3], lsm6dsv16x_accel[3];
float lsm6dsv16x_pitch, lsm6dsv16x_roll, lsm6dsv16x_yaw;

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void platform_delay(uint32_t ms);

// 四元数转欧拉角函数保持不变
static void quat_to_euler(const float q[4], float *roll, float *pitch, float *yaw)
{
    // q = [x, y, z, w] 格式
    float x = q[0];
    float y = q[1];
    float z = q[2];
    float w = q[3];

    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (w * x + y * z);
    float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
    *roll = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (w * y - z * x);
    if (fabsf(sinp) >= 1.0f)
        *pitch = copysignf(M_PI / 2.0f, sinp); // use 90 degrees if out of range
    else
        *pitch = asinf(sinp);

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (w * z + x * y);
    float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
    *yaw = atan2f(siny_cosp, cosy_cosp);

    *roll *= RAD_TO_DEG;
    *pitch *= RAD_TO_DEG;
    *yaw *= RAD_TO_DEG;
}

static float_t npy_half_to_float(uint16_t h)
{
    union { float_t ret; uint32_t retbits; } conv;
    conv.retbits = lsm6dsv16x_from_f16_to_f32(h);
    return conv.ret;
}

static void sflp2q(float_t quat[4], uint8_t raw[6])
{
    float_t sumsq = 0;
    uint16_t sflp[3];

    memcpy(&sflp[0], &raw[0], 2);
    memcpy(&sflp[1], &raw[2], 2);
    memcpy(&sflp[2], &raw[4], 2);

    quat[0] = npy_half_to_float(sflp[0]);
    quat[1] = npy_half_to_float(sflp[1]);
    quat[2] = npy_half_to_float(sflp[2]);

    for (uint8_t i = 0; i < 3; i++)
        sumsq += quat[i] * quat[i];

    if (sumsq > 1.0f) {
        float_t n = sqrtf(sumsq);
        quat[0] /= n;
        quat[1] /= n;
        quat[2] /= n;
        sumsq = 1.0f;
    }

    quat[3] = sqrtf(1.0f - sumsq);
}

void LSM6DSV16X_Init(void)
{
    // 设置软件I2C的设备地址
    lsm6dsv16x_i2c.addr = LSM6DSV16X_ADDR;
    
    // 初始化软件I2C
    soft_iic_init(&lsm6dsv16x_i2c);
    
    /* Initialize mems driver interface */
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.mdelay = platform_delay;

    /* Wait sensor boot time */
    platform_delay(BOOT_TIME);

    /* Check device ID */
    lsm6dsv16x_device_id_get(&dev_ctx, &whoamI);

    if (whoamI != LSM6DSV16X_ID)
    {
        return;
    }

    /* Restore default configuration */
    lsm6dsv16x_reset_set(&dev_ctx, LSM6DSV16X_GLOBAL_RST);
    do {
        lsm6dsv16x_reset_get(&dev_ctx, &rst);
    } while (rst != LSM6DSV16X_READY);

    /* Enable Block Data Update */
    lsm6dsv16x_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
    /* Set full scale */
    lsm6dsv16x_xl_full_scale_set(&dev_ctx, LSM6DSV16X_4g);
    lsm6dsv16x_gy_full_scale_set(&dev_ctx, LSM6DSV16X_2000dps);

    /* Set FIFO batch of sflp data */
    fifo_sflp.game_rotation = 1;
    fifo_sflp.gravity = 0;
    fifo_sflp.gbias = 0;
    lsm6dsv16x_fifo_sflp_batch_set(&dev_ctx, fifo_sflp);
    lsm6dsv16x_fifo_xl_batch_set(&dev_ctx, LSM6DSV16X_XL_BATCHED_AT_60Hz);
    lsm6dsv16x_fifo_gy_batch_set(&dev_ctx, LSM6DSV16X_GY_BATCHED_AT_60Hz);

    /* Set FIFO mode to Stream mode (aka Continuous Mode) */
    lsm6dsv16x_fifo_mode_set(&dev_ctx, LSM6DSV16X_STREAM_MODE);

    /* Set Output Data Rate */
    lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_60Hz);
    lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_60Hz);
    lsm6dsv16x_sflp_data_rate_set(&dev_ctx, LSM6DSV16X_SFLP_60Hz);
    lsm6dsv16x_sflp_game_rotation_set(&dev_ctx, PROPERTY_ENABLE);

    pin_int.drdy_xl = PROPERTY_ENABLE;
    lsm6dsv16x_pin_int2_route_set(&dev_ctx, &pin_int);
    lsm6dsv16x_data_ready_mode_set(&dev_ctx, LSM6DSV16X_DRDY_PULSED);
}

void Read_LSM6DSV16X(void)
{
    uint16_t num;
    int16_t datax, datay, dataz;
    lsm6dsv16x_fifo_status_t fifo_status;
    
    /* Read watermark flag */
    lsm6dsv16x_fifo_status_get(&dev_ctx, &fifo_status);
    num = fifo_status.fifo_level;
    
    while (num--) 
    {
        lsm6dsv16x_fifo_out_raw_t f_data;
        
        /* Read FIFO sensor value */
        lsm6dsv16x_fifo_out_raw_get(&dev_ctx, &f_data);
        memcpy(&datax, &f_data.data[0], 2);
        memcpy(&datay, &f_data.data[2], 2);
        memcpy(&dataz, &f_data.data[4], 2);
        
        switch (f_data.tag) 
        {
            case LSM6DSV16X_SFLP_GAME_ROTATION_VECTOR_TAG:
                sflp2q(lsm6dsv16x_quat, &f_data.data[0]);
                quat_to_euler(lsm6dsv16x_quat, &lsm6dsv16x_roll, &lsm6dsv16x_pitch, &lsm6dsv16x_yaw);
                break;
            case LSM6DSV16X_XL_NC_TAG:
                lsm6dsv16x_accel[0] = lsm6dsv16x_from_fs4_to_mg(datax);
                lsm6dsv16x_accel[1] = lsm6dsv16x_from_fs4_to_mg(datay);
                lsm6dsv16x_accel[2] = lsm6dsv16x_from_fs4_to_mg(dataz);
                break;
            case LSM6DSV16X_GY_NC_TAG:
                lsm6dsv16x_gyro[0] = lsm6dsv16x_from_fs2000_to_mdps(datax)/1000;
                lsm6dsv16x_gyro[1] = lsm6dsv16x_from_fs2000_to_mdps(datay)/1000;
                lsm6dsv16x_gyro[2] = lsm6dsv16x_from_fs2000_to_mdps(dataz)/1000;
                break;
            default:
                break;
        }
    }
}

/*
 * @brief  使用软件I2C写寄存器 (platform dependent)
 *
 * @param  handle    customizable argument. 
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    if (!len) return 0;
    
    // 使用软件I2C写寄存器
    soft_iic_write_8bit_registers(&lsm6dsv16x_i2c, reg, bufp, len);
    
    return 0;
}

/*
 * @brief  使用软件I2C读寄存器 (platform dependent)
 *
 * @param  handle    customizable argument.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    if (!len) return 0;
    
    // 使用软件I2C读寄存器
    soft_iic_read_8bit_registers(&lsm6dsv16x_i2c, reg, bufp, len);
    
    return 0;
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
   delay_ms(ms);
}