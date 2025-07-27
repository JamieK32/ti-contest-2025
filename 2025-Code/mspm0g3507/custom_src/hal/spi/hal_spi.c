#include "hal_spi.h"


// SPI д�ֽں���
uint8_t spi_read_write_byte(SPI_Regs *spi_inst, uint8_t byte)
{
    // �ȴ� SPI ������� (�����ʹ�õ�������ģʽ�� transmitData8������ܲ���Ҫ)
    while (DL_SPI_isBusy(spi_inst));
    DL_SPI_transmitData8(spi_inst, byte);
    // �ȴ����� FIFO �ǿգ���ʾ�����ѽ��� (���ڴ����Ϳ��ܲ���Ҫ��������)
    while(DL_SPI_isRXFIFOEmpty(spi_inst));
    // ���ؽ��յ������� (�������Ҫ���գ����Է��� dummy ֵ)
    return DL_SPI_receiveData8(spi_inst); // �������Ҫ���գ�����ע�͵�
    return 0; // ���� dummy ֵ
}

