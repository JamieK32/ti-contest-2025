#include "gray_detection.h"
#include "pca9555.h"

uint16_t gray_byte = 0x00;
#define SEPARATED_PATTERN_OUTPUT  3.0
static float gray_status_backup = 0.0f; // 初始备份值设为0

#ifdef USE_PCA9555 
// I2C 硬件配置 - 适配新的软件I2C结构体
static soft_iic_info_struct pca9555_i2c = {
    .sclPort = PORTA_PORT,
    .sdaPort = PORTA_PORT,
    .sclPin = PORTA_SCL1_PIN,
    .sdaPin = PORTA_SDA1_PIN,
    .sclIOMUX = PORTA_SCL1_IOMUX,
    .sdaIOMUX = PORTA_SDA1_IOMUX,
    .delay_time = 10,
    .addr = 0x20,
};
#elif defined(USE_GW_GRAY)
static soft_iic_info_struct gw_i2c = {
    .sclPort = PORTA_PORT,
    .sdaPort = PORTA_PORT,
    .sclPin = PORTA_SCL1_PIN,
    .sdaPin = PORTA_SDA1_PIN,
    .sclIOMUX = PORTA_SCL1_IOMUX,
    .sdaIOMUX = PORTA_SDA1_IOMUX,
    .delay_time = 45,
    .addr = GW_GRAY_ADDR_DEF,
};
#endif

#ifdef USE_GPIO
static gpio_struct_t gray_gpio[TRACK_SENSOR_COUNT] = {
    TRACK_PIN_0_PORT, TRACK_PIN_0_PIN,
    TRACK_PIN_1_PORT, TRACK_PIN_1_PIN,
    TRACK_PIN_2_PORT, TRACK_PIN_2_PIN,
    TRACK_PIN_3_PORT, TRACK_PIN_3_PIN,
    TRACK_PIN_4_PORT, TRACK_PIN_4_PIN,
    TRACK_PIN_5_PORT, TRACK_PIN_5_PIN,
    TRACK_PIN_6_PORT, TRACK_PIN_6_PIN,
    TRACK_PIN_7_PORT, TRACK_PIN_7_PIN,
};
#endif

void gray_detection_init(void) {
#ifdef USE_PCA9555
    soft_iic_init(&pca9555_i2c);
#elif defined(USE_GW_GRAY) 
    soft_iic_init(&gw_i2c);
#endif
}

uint16_t gray_read_byte(void) {
#ifdef USE_GPIO
    uint16_t data = 0;
    for (int i = 0; i < TRACK_SENSOR_COUNT; i++) {
        uint8_t bit = !DL_GPIO_readPins(gray_gpio[i].port, gray_gpio[i].pin);
        data = (data << 1) | bit;
    }
    return data;    
#elif defined(USE_PCA9555)
    return pca9555_read_bit12(&pca9555_i2c, PCA9555_ADDR);
#elif defined(USE_GW_GRAY)
    uint8_t digital_value;
    
    
    // 使用新的I2C读取函数
    digital_value = soft_iic_read_8bit_register(&gw_i2c, GW_GRAY_DIGITAL_MODE);
    

    
    digital_value = ~digital_value;
    digital_value = ((digital_value & 0x01) << 7) | ((digital_value & 0x02) << 5) |
                    ((digital_value & 0x04) << 3) | ((digital_value & 0x08) << 1) |
                    ((digital_value & 0x10) >> 1) | ((digital_value & 0x20) >> 3) |
                    ((digital_value & 0x40) >> 5) | ((digital_value & 0x80) >> 7);
    return (uint16_t)digital_value;
#elif defined(USE_CAM)
		#include "task21f_config.h"
    extern maixCam_t maix_cam;
    return maix_cam.track_data;
#endif 
}

float gray_get_position(void) {
    gray_byte = gray_read_byte();
    
    const int table_size = sizeof(lookup_table) / sizeof(lookup_table[0]);
    
    // 查找匹配的输入值
    for (int i = 0; i < table_size; i++) {
        if (lookup_table[i].input == gray_byte) {
            gray_status_backup = lookup_table[i].output;
            return lookup_table[i].output;
        }
    }
    return gray_status_backup;
}

float gray_get_position_22c_ti_contest(bool flag) {
    gray_byte = gray_read_byte();
    
    const int table_size = sizeof(lookup_table) / sizeof(lookup_table[0]);
    
    uint8_t ret;
    if (is_separated_pattern(gray_byte)) {
        ret = (flag == true) ? 0.0 : SEPARATED_PATTERN_OUTPUT;
        return ret;
    }

    // 查找匹配的输入值
    for (int i = 0; i < table_size; i++) {
        if (lookup_table[i].input == gray_byte) {
            gray_status_backup = lookup_table[i].output;
            return lookup_table[i].output;
        }
    }
    return gray_status_backup;
}

static inline bool is_separated_pattern(uint8_t sensor_pattern) {
    int table_size = sizeof(separated_pattern_table_8bit) / sizeof(separated_pattern_table_8bit[0]);
    
    for (int i = 0; i < table_size; i++) {
        if (separated_pattern_table_8bit[i] == sensor_pattern) {
            return true;
        }
    }
    return false;
}