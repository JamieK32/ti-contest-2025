#include "task25k_config.h"
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
	car_add_straight(50);
	car_set_loop(1);
}

static void setup_base_part2(void) {
    car_path_init();
		car_add_straight(70);
		car_add_turn(46);
		car_add_straight(70);
		car_add_turn(-46);
		car_add_straight(72);
		car_add_turn(46);
		car_add_straight(65);
		car_add_float(&car.target_angle, 0);
		car_add_straight(85);
    car_set_loop(1);
}

static void setup_base_part3(void) {
    car_path_init();
		car_add_straight(115);
		car_add_circle(35, true, 270);  // 半径20cm，逆时针
		car_add_float(&car.target_angle, 90);
		car_add_straight(44);
		car_add_circle(38, true, 360); 
		car_add_float(&car.target_angle, 0);
		car_add_straight(205);
    car_set_loop(1);
}

static void setup_play_part1(void) {
		car_path_init();                                    // 初始化路径规划
		car_add_straight(90);
		car_add_turn(90.0f);
		car_add_straight(95);
		car_add_turn(0.0f);
		car_add_straight(100);
		car_add_turn(-90.0f);
		car_add_straight(42);
		car_add_turn(0.0f);
		car_add_straight(100);
    car_set_loop(1);
}

void play_part2_path1(void) {
  	car_add_straight(87);
		car_add_turn(90.0f);
		car_add_straight(45);
	  car_add_turn(0.0f);
		car_add_straight(48);
	  car_add_turn(90.0f);
		car_add_straight(50);
    car_add_turn(0.0f);
		car_add_straight(95);
    car_add_float(&car.target_angle,-90.0f);
		car_add_straight(60);
	  car_add_float(&car.target_angle,0.0f);
		car_add_straight(70);  
}
void play_part2_path2(void) {
		car_add_straight(97);
		car_add_turn(90.0f);
		car_add_straight(100);
		car_add_turn(0.0f);
		car_add_straight(95);
		car_add_turn(-90.0f);
		car_add_straight(50);
		car_add_turn(0.0f);
		car_add_straight(90); 
}

void play_part2_path3(void) {
		car_add_straight(142);
		car_add_turn(-90.0f);
		car_add_straight(50);
		car_add_turn(0.0f);
		car_add_straight(100);
		car_add_turn(90.0f);
		car_add_straight(100);
		car_add_turn(0.0f);
		car_add_straight(70);   
}

void play_part2_path4(void) {
		car_add_straight(85);
	 	car_add_turn(-90.0f);
		car_add_straight(50);
	  car_add_turn(0.0f);
		car_add_straight(103);
		car_add_turn(90.0f);
		car_add_straight(50);
		car_add_turn(0.0f);
		car_add_straight(50);
	  car_add_float(&car.target_angle,90.0f);
		car_add_straight(57);
	  car_add_float(&car.target_angle, 0);
		car_add_straight(80);
}
void play_1(void) {
	set_alert_count(1);
	start_alert();
}

void play_2(void) {
	set_alert_count(2);
	start_alert();
}

void play_3(void) {
	set_alert_count(3);
	start_alert();
}

void play_4(void) {
	set_alert_count(4);
	start_alert();
}
void task5_func(void) {
	if (maix_cam.cmd == 0x01) {
			car_add_function(play_1);
			play_part2_path1();

	} else if (maix_cam.cmd == 0x02) {
			car_add_function(play_2);
			play_part2_path2();

	} else if (maix_cam.cmd == 0x03) {  
			car_add_function(play_3);
			play_part2_path3();

	} else if (maix_cam.cmd == 0x04) {
			car_add_function(play_4);
			play_part2_path4();
	}
}


static void setup_play_part2(void) {
    car_path_init();
		add_send_and_wait(send_start_cmd, wait_cam_cmd);  // 使用封装
    car_add_function(task5_func);
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
		enable_periodic_task(EVENT_CAR);
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

static void run_play_part01_cb(void *arg) {
    run_task("Play Part 01", &task_running_flag, setup_play_part1);
}

static void run_play_part02_cb(void *arg) {
    run_task("Play Part 02", &task_running_flag, setup_play_part2);
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
		ADD_ACTION(tasks1_menu, _25_task4, "Run Play Part1", run_play_part01_cb);
		ADD_ACTION(tasks1_menu, _25_task5, "Run Play Part2", run_play_part02_cb);
	
		ADD_SUBMENU(main_menu, PlayMusic, "Play Music", NULL);
		ADD_ACTION(PlayMusic, music1, "ChunRiYing", play_music_1_cb);
    ADD_ACTION(PlayMusic, music2, "TianKongZhiCheng", play_music_2_cb);
		ADD_ACTION(PlayMusic, stop_music, "StopMusic", stop_music_cb);
		
		ADD_SUBMENU(main_menu, status_menu, "System Status", NULL);
		ADD_VAR_VIEW(status_menu, gyro_status_view, "Gyro Status", gyro_vars);
		ADD_VAR_VIEW(status_menu, car_status_view, "Car Status", car_vars);
    create_oled_menu(&main_menu);
}

