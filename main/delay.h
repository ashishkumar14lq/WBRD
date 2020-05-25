/*
 * delay.h
 *
 *  Created on: 16 mar. 2020
 *      Author: Julian
 */




#ifndef MAIN_DELAY_H_
#define MAIN_DELAY_H_


void delay_ms(int x)
{
	vTaskDelay(x  * configTICK_RATE_HZ / 1000);
}

void delay_us(int x)
{
	vTaskDelay(x  * configTICK_RATE_HZ / 1000000);
}

#endif /* MAIN_DELAY_H_ */
