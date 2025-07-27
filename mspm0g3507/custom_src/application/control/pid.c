#include "pid.h"
#include <math.h>

// PID控制器初始化 - 设置默认值
void PID_Init(PID_Controller_t *pid, PID_Type_e type) {
    // 清零所有参数
    pid->Kp = 0.0f;
    pid->Ki = 0.0f;
    pid->Kd = 0.0f;
    
    pid->target = 0.0f;
    pid->feedback = 0.0f;
    pid->error = 0.0f;
    pid->last_error = 0.0f;
    pid->last_last_error = 0.0f;
    
    pid->integral = 0.0f;
    pid->integral_max = 3000.0f;        // 默认积分限幅
    pid->integral_min = -3000.0f;
    pid->integral_separation_threshold = 0.0f; // 默认积分分离阈值
    
    pid->output = 0.0f;
    pid->output_offset = 0.0f;          // 默认无输出偏移
    pid->output_max = 3000.0f;          // 默认输出限幅
    pid->output_min = -3000.0f;
    pid->last_output = 0.0f;
    
    pid->deadzone = 0.0f;               // 默认无死区
    
    // 新增：微分项滤波相关
    pid->derivative_filter_alpha = 0.7f; // 默认滤波系数
    pid->last_derivative = 0.0f;
    pid->filtered_derivative = 0.0f;
    
    pid->type = type;
    pid->enable_integral_separation = 0;     // 默认关闭积分分离
    pid->enable_integral_limit = 0;          // 默认关闭积分限幅
    pid->enable_output_limit = 0;            // 默认关闭输出限幅
    pid->enable_deadzone = 0;                // 默认关闭死区
    pid->enable_anti_windup = 1;            // 默认开启抗积分饱和
    pid->enable_derivative_filter = 1;       // 默认开启微分滤波
}

// 死区处理函数
static float PID_DeadzoneProcess(float error, PID_Controller_t *pid) {
    if (!pid->enable_deadzone) {
        return error;
    }
    
    float abs_error = fabsf(error);
    if (abs_error < pid->deadzone) {
        return 0.0f;
    }
    
    // 去除死区后的误差
    if (error > 0.0f) {
        return error - pid->deadzone;
    } else {
        return error + pid->deadzone;
    }
}

// 积分分离检查
static uint16_t PID_IntegralSeparationCheck(float error, PID_Controller_t *pid) {
    if (!pid->enable_integral_separation) {
        return 1; // 不使用积分分离，允许积分
    }
    
    float abs_error = fabsf(error);
    return (abs_error < pid->integral_separation_threshold) ? 1 : 0;
}

// 抗积分饱和处理
static void PID_AntiWindupProcess(PID_Controller_t *pid) {
    if (!pid->enable_anti_windup || !pid->enable_output_limit) {
        return;
    }
    
    // 如果输出已经饱和，且误差方向会使饱和加剧，则停止积分
    if ((pid->output >= pid->output_max && pid->error > 0) ||
        (pid->output <= pid->output_min && pid->error < 0)) {
        // 可选：缓慢减小积分项，而不是完全停止
        pid->integral *= 0.95f;
    }
}

// 积分限幅处理
static void PID_IntegralLimitProcess(PID_Controller_t *pid) {
    if (!pid->enable_integral_limit) {
        return;
    }
    
    if (pid->integral > pid->integral_max) {
        pid->integral = pid->integral_max;
    } else if (pid->integral < pid->integral_min) {
        pid->integral = pid->integral_min;
    }
}

// 输出限幅处理
static void PID_OutputLimitProcess(PID_Controller_t *pid) {
    if (!pid->enable_output_limit) {
        return;
    }
    
    if (pid->output > pid->output_max) {
        pid->output = pid->output_max;
    } else if (pid->output < pid->output_min) {
        pid->output = pid->output_min;
    }
}

// 微分项滤波处理
static float PID_DerivativeFilter(float derivative, PID_Controller_t *pid) {
    if (!pid->enable_derivative_filter) {
        return derivative;
    }
    
    // 一阶低通滤波器
    pid->filtered_derivative = pid->derivative_filter_alpha * derivative + 
                              (1.0f - pid->derivative_filter_alpha) * pid->last_derivative;
    pid->last_derivative = pid->filtered_derivative;
    
    return pid->filtered_derivative;
}

// 位置式PID计算
float PID_PositionCalculate(PID_Controller_t *pid) {
    float proportional, integral, differential;
    
    // 计算误差并进行死区处理
    pid->error = pid->target - pid->feedback;
    pid->error = PID_DeadzoneProcess(pid->error, pid);
    
    // 比例项
    proportional = pid->Kp * pid->error;
    
    // 积分项 - 检查积分分离和抗饱和
    if (PID_IntegralSeparationCheck(pid->error, pid)) {
        // 先计算输出，用于抗积分饱和判断
        float temp_output = proportional + pid->Ki * pid->integral + pid->output_offset;
        
        // 抗积分饱和检查
        if (!pid->enable_anti_windup || 
            !((temp_output >= pid->output_max && pid->error > 0) ||
              (temp_output <= pid->output_min && pid->error < 0))) {
            pid->integral += pid->error;
        }
        
        PID_IntegralLimitProcess(pid); // 积分限幅
    }
    integral = pid->Ki * pid->integral;
    
    // 微分项 - 带滤波
    float error_diff = pid->error - pid->last_error;
    float raw_derivative = pid->Kd * error_diff;
    differential = PID_DerivativeFilter(raw_derivative, pid);
    
    // 计算输出
    pid->output = proportional + integral + differential + pid->output_offset;
    
    // 输出限幅
    PID_OutputLimitProcess(pid);
    
    // 抗积分饱和后处理
    PID_AntiWindupProcess(pid);
    
    // 更新历史误差
    pid->last_error = pid->error;
    
    return pid->output;
}

// 增量式PID计算
float PID_IncrementCalculate(PID_Controller_t *pid) {
    float delta_output;
    float proportional_delta, integral_delta, differential_delta;
    static uint8_t first_run = 1;  // 标记是否第一次运行
    
    // 计算误差并进行死区处理
    pid->error = pid->target - pid->feedback;
    pid->error = PID_DeadzoneProcess(pid->error, pid);
    
    // 增量式PID算法：Δu(k) = Kp[e(k)-e(k-1)] + Ki*e(k) + Kd[e(k)-2e(k-1)+e(k-2)]
    
    // 比例增量
    proportional_delta = pid->Kp * (pid->error - pid->last_error);
    
    // 积分增量 - 检查积分分离
    if (PID_IntegralSeparationCheck(pid->error, pid)) {
        integral_delta = pid->Ki * pid->error;
    } else {
        integral_delta = 0.0f;
    }
    
    // 微分增量 - 带滤波
    float error_diff2 = pid->error - 2.0f * pid->last_error + pid->last_last_error;
    float raw_derivative_delta = pid->Kd * error_diff2;
    differential_delta = PID_DerivativeFilter(raw_derivative_delta, pid);
    
    // 计算输出增量
    delta_output = proportional_delta + integral_delta + differential_delta;
    
    // 计算新的输出
    if (first_run) {
        // 第一次运行时加入偏移
        pid->output = pid->last_output + delta_output + pid->output_offset;
        first_run = 0;
    } else {
        // 后续运行不再累加偏移
        pid->output = pid->last_output + delta_output;
    }
    
    // 输出限幅前的抗积分饱和检查
    if (pid->enable_anti_windup && pid->enable_output_limit) {
        if ((pid->output >= pid->output_max && delta_output > 0) ||
            (pid->output <= pid->output_min && delta_output < 0)) {
            // 如果输出饱和且增量会使饱和加剧，则不更新输出
            pid->output = pid->last_output;
        }
    }
    
    // 输出限幅
    PID_OutputLimitProcess(pid);
    
    // 更新历史值
    pid->last_last_error = pid->last_error;
    pid->last_error = pid->error;
    pid->last_output = pid->output;
    
    return pid->output;
}

// PID主计算函数
float PID_Calculate(float target, float feedback, PID_Controller_t *pid) {
    pid->target = target;
    pid->feedback = feedback;
    
    if (pid->type == PID_TYPE_POSITION) {
        return PID_PositionCalculate(pid);
    } else {
        return PID_IncrementCalculate(pid);
    }
}

// 重置PID控制器
void PID_Reset(PID_Controller_t *pid) {
    pid->error = 0.0f;
    pid->last_error = 0.0f;
    pid->last_last_error = 0.0f;
    pid->integral = 0.0f;
    pid->output = 0.0f;
    pid->last_output = 0.0f;
    pid->last_derivative = 0.0f;
    pid->filtered_derivative = 0.0f;
}

// 辅助函数实现
void PID_SetTarget(PID_Controller_t *pid, float target) {
    pid->target = target;
}

void PID_SetFeedback(PID_Controller_t *pid, float feedback) {
    pid->feedback = feedback;
}

float PID_GetOutput(PID_Controller_t *pid) {
    return pid->output;
}

float PID_GetError(PID_Controller_t *pid) {
    return pid->error;
}

// 参数设置函数
void PID_SetParams(PID_Controller_t *pid, float kp, float ki, float kd) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
}

void PID_SetIntegralLimit(PID_Controller_t *pid, float max_val, float min_val) {
    pid->integral_max = max_val;
    pid->integral_min = min_val;
    pid->enable_integral_limit = 1;
}

void PID_SetOutputLimit(PID_Controller_t *pid, float max_val, float min_val) {
    pid->output_max = max_val;
    pid->output_min = min_val;
    pid->enable_output_limit = 1;
}

void PID_SetDeadzone(PID_Controller_t *pid, float deadzone) {
    pid->deadzone = deadzone;
    pid->enable_deadzone = (deadzone > 0.0f) ? 1 : 0;
}

void PID_SetIntegralSeparation(PID_Controller_t *pid, float threshold) {
    pid->integral_separation_threshold = threshold;
    pid->enable_integral_separation = (threshold > 0.0f) ? 1 : 0;
}


void PID_SetAntiWindup(PID_Controller_t *pid, uint8_t enable) {
    pid->enable_anti_windup = enable;
}

void PID_SetDerivativeFilter(PID_Controller_t *pid, uint8_t enable, float alpha) {
    pid->enable_derivative_filter = enable;
    if (alpha > 0.0f && alpha < 1.0f) {
        pid->derivative_filter_alpha = alpha;
    }
}