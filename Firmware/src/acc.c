#include "hardware.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "acc.h"

void AccInit(void)
{
	
}

void ACC_I2C_IRQHandler(void)
{
/*
	char data;
	if (I2C_GetITStatus(ACC_I2C, I2C_IT_RXNE) == SET)
	{
		data = I2C_ReceiveData(ACC_I2C);
	}
*/
}
