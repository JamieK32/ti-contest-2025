#include "mspm0g3507_it.h"
#include "ti_msp_dl_config.h"
#include "encoder_user.h"
#include "wit_jyxx.h"
#include "bluetooth.h"
#include "maix_cam.h"

/**
 * @brief UART 中断处理函数
 */
void UART_0_INST_IRQHandler(void) {
    uint8_t uart_data;
    DL_UART_IIDX idx = DL_UART_getPendingInterrupt(UART_0_INST);
		wit_imu_uart_irq_handler(idx);
    DL_UART_clearInterruptStatus(UART_0_INST, idx);
}

// UART中断处理函数 - 简化版本，只负责接收数据到环形缓冲区
void UART_1_INST_IRQHandler(void) {
    DL_UART_IIDX idx = DL_UART_getPendingInterrupt(UART_1_INST);

		camera_irq_handler(DL_UART_IIDX_RX);
    DL_UART_clearInterruptStatus(UART_1_INST, idx);
}

void GROUP1_IRQHandler(void) {
   if (DL_Interrupt_getStatusGroup(DL_INTERRUPT_GROUP_1, PORTB_INT_IIDX)) {
			encoder_group1_irq_handler();
    }
}