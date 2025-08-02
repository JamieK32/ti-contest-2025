#include "tests.h"
#include "common_include.h"
#include "log_config.h"
#include "log.h"
#include "ganv_calibration.h"

void lsm6dsv16x_test(void) {

	LSM6DSV16X_Init();
	extern float lsm6dsv16x_pitch, lsm6dsv16x_roll, lsm6dsv16x_yaw;
	while(1) {
			Read_LSM6DSV16X();
			log_i("%.2lf,%.2lf,%.2lf\n", lsm6dsv16x_yaw, lsm6dsv16x_roll, lsm6dsv16x_pitch);
			delay_ms(10);
	}

}