#include "common_defines.h"
#if (CURRENT_TASK_TYPE == TASK_TYPE_25K)
#include "task21f_config.h"
#include "ui.h"
#include "log_config.h"
#include "log.h"
#include "common_include.h"


/* =============================================================================
 * 任务配置和回调函数
 * ============================================================================= */
bool task_running_flag = false;

void reset_cam(void) {
	maix_cam.num = 0;
	maix_cam.cmd = 0;
}

bool wait_cam_num(void) {
		if (maix_cam.num != 0) {
			return true;
		} else {
			return false;
		}
}

bool wait_cam_cmd(void) {
		if (maix_cam.cmd != 0) {
			return true;
		} else {
			return false;
		}
}
void send_start_cmd(void) {
		camera_send_data((uint8_t *)"START", 5);
}

void send_number_cmd(void) {
		camera_send_data((uint8_t *)"NUMBER", 6);
}

void send_track_cmd(void) {
		camera_send_data((uint8_t *)"TRACK", 5);
}

// 发送命令并等待响应的组合
void add_send_and_wait(void (*send_func)(void), bool (*wait_func)(void)) {
    car_add_function(send_func);
    car_add_wait_func_true(wait_func);
}

// 简化的setup函数
static void setup_base_part1(void) {
	car_path_init();
	car_add_straight(240);
	car_add_turn(90.0f);
	car_add_straight(50);
	car_add_turn(0.0f);
	car_add_straight(35);
	car_set_loop(1);
}

static void setup_base_part2(void) {
    car_path_init();
		car_add_straight(60);
		car_add_turn(45);
		car_add_straight(75);
		car_add_turn(-45);
		car_add_straight(70);
		car_add_turn(45);
		car_add_straight(70);
		car_add_float(&car.target_angle, 0);
		car_add_straight(70);
    car_set_loop(1);
}

static void setup_base_part3(void) {
    car_path_init();
		car_add_straight(115);
		car_add_circle(30, true, 270);  // 半径20cm，逆时针
		car_add_float(&car.target_angle, 90);
		car_add_straight(41);
		car_add_circle(30, true, 360); 
		car_add_float(&car.target_angle, 0);
		car_add_straight(195);
    car_set_loop(1);
}



static void run_task(const char *task_name, bool *task_flag, void (*setup_func)(void)) {
    if (*task_flag == true) {
        show_message("Running Failed");
        return;
    }
    *task_flag = true;
    show_message(task_name);    
    setup_func();     // 调用设置函数
    car_start();      // 启动状态机
    enable_periodic_task(EVENT_CAR_STATE_MACHINE);
}

static void run_base_part01_cb(void *arg) {
    run_task("Base Part 01", &task_running_flag, setup_base_part1);
}

static void run_base_part02_cb(void *arg) {
    run_task("Base Part 02", &task_running_flag, setup_base_part2);
}

static void run_base_part03_cb(void *arg) {
    run_task("Base Part 03", &task_running_flag, setup_base_part3);
}



static void play_music_1_cb(void *arg) {
	show_message("Play Music1");
	music_player_start(music_example_1, music_example_1_size);
}

static void play_music_2_cb(void *arg) {
	show_message("Play Music2");
	music_player_start(music_example_2, music_example_2_size);
}

static void stop_music_cb(void *arg) {
	show_message("Stop Music");
	music_player_stop();
}

#if CURRENT_IMU == MPU6050_GYRO
	extern float yaw, roll, pitch;
#elif (CURRENT_IMU == IMU660RA_GYRO)
extern Attitude_module attitude;
#endif

static menu_variable_t gyro_vars[] = {
#if CURRENT_IMU == WIT_GYRO
    MENU_VAR_READONLY("Yaw", &jy61p.yaw, VAR_TYPE_FLOAT),
		MENU_VAR_READONLY("Pitch", &jy61p.pitch, VAR_TYPE_FLOAT),
		MENU_VAR_READONLY("Roll", &jy61p.roll, VAR_TYPE_FLOAT),
#elif CURRENT_IMU == MPU6050_GYRO
	  MENU_VAR_READONLY("Yaw", &yaw, VAR_TYPE_FLOAT),
		MENU_VAR_READONLY("Pitch", &pitch, VAR_TYPE_FLOAT),
		MENU_VAR_READONLY("Roll", &roll, VAR_TYPE_FLOAT),
#elif (CURRENT_IMU == IMU660RA_GYRO)

		MENU_VAR_READONLY("Yaw", &attitude.pose_module.data.yaw, VAR_TYPE_FLOAT),
		MENU_VAR_READONLY("Pitch", &attitude.pose_module.data.pit, VAR_TYPE_FLOAT),
		MENU_VAR_READONLY("Roll", &attitude.pose_module.data.rol, VAR_TYPE_FLOAT),
#endif 
		MENU_VAR_END
};

static menu_variable_t car_vars[] = {
    MENU_VAR_BINARY_8BIT("Gray", &gray_byte),
    MENU_VAR_END
};

/* =============================================================================
 * 菜单创建
 * ============================================================================= */
void menu_init_and_create(void) {
    // 使用链式构建宏创建菜单结构
    MENU_BUILDER_START(main_menu, "Main Menu");
    
    // 任务执行菜单
    ADD_SUBMENU(main_menu, tasks1_menu, "Run 25K App", NULL);
    ADD_ACTION(tasks1_menu, _25_task1, "Run Base Part1", run_base_part01_cb);
		ADD_ACTION(tasks1_menu, _25_task2, "Run Base Part2", run_base_part02_cb);
		ADD_ACTION(tasks1_menu, _25_task3, "Run Base Part3", run_base_part03_cb);

	
		ADD_SUBMENU(main_menu, PlayMusic, "Play Music", NULL);
		ADD_ACTION(PlayMusic, music1, "ChunRiYing", play_music_1_cb);
    ADD_ACTION(PlayMusic, music2, "TianKongZhiCheng", play_music_2_cb);
		ADD_ACTION(PlayMusic, stop_music, "StopMusic", stop_music_cb);
		
		ADD_SUBMENU(main_menu, status_menu, "System Status", NULL);
		ADD_VAR_VIEW(status_menu, gyro_status_view, "Gyro Status", gyro_vars);
		ADD_VAR_VIEW(status_menu, car_status_view, "Car Status", car_vars);
    create_oled_menu(&main_menu);
}

#endif