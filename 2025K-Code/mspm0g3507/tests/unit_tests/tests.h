#ifndef TESTS_H__
#define TESTS_H__

// 巡线模块单独测试
void gd_test(void);
// 维特陀螺仪单独测试
void wit_test(void);
// 电机单独测试
void motor_test(void);
// mpu6050测试
void mpu_test(void);
// 蓝牙模块测试
void bluetooth_test(void);
// vl53l1模块测试
void vl53l1_test(void);
// 感为循迹模块测试
void no_mcu_ganv_test(void);
// 摄像头协议测试
int cam_test(void);
// lsm6dsv16x 测试
void lsm6dsv16x_test(void);
#endif
