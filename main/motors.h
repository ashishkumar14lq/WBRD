/*
 * motors.h
 *
 *  Created on: 27 ago. 2020
 *      Author: Julian
 */

#ifndef MAIN_MOTORS_H_
#define MAIN_MOTORS_H_

#define us_between_steps	1000

#define m_direction		14		//1 - Clockwise / 0-Anticlockwise
#define motor1_step		25
#define motor2_step		2
#define motor_nEnable	4
#define motor_nReset	12
#define motor_nFault	27
#define motor_nSleep	26

//switches (clockwise and anticlockwise)
#define clock_pin				39
#define aclock_pin				32
#define SW_mode			33	// 1-NO / 0-NC

// If sample motor is motor 1, SAMPLE_MOTOR1 must be defined. If not, motor1 is detector motor
#define SAMPLE_MOTOR1

#ifdef SAMPLE_MOTOR1
	#define sample_step_pin 		motor1_step
	#define detector_step_pin		motor2_step
#else
	#define sample_step_pin 		motor2_step
	#define detector_step_pin		motor1_step
#endif



void sample_step(int nsteps)
{
	for(int i=0;i<nsteps;i++){
		ets_delay_us(us_between_steps);
		gpio_set_level(sample_step_pin, 1);
		ets_delay_us(us_between_steps);
		gpio_set_level(sample_step_pin, 0);
	}
}

void detector_step(int nsteps)
{
	for (int i=0;i<nsteps;i++){
		ets_delay_us(us_between_steps);
		gpio_set_level(detector_step_pin, 1);
		ets_delay_us(us_between_steps);
		gpio_set_level(detector_step_pin, 0);
	}
}

esp_err_t m_initialize(void)
{
	gpio_pad_select_gpio(m_direction);
	gpio_pad_select_gpio(sample_step_pin);
	gpio_pad_select_gpio(detector_step_pin);
	gpio_pad_select_gpio(motor_nEnable);
	gpio_pad_select_gpio(motor_nReset);
	gpio_pad_select_gpio(motor_nFault);
	gpio_pad_select_gpio(motor_nSleep);

	gpio_pad_select_gpio(clock_pin);
	gpio_pad_select_gpio(aclock_pin);
	gpio_pad_select_gpio(SW_mode);

	gpio_set_direction(m_direction, GPIO_MODE_OUTPUT);
	gpio_set_direction(sample_step_pin, GPIO_MODE_OUTPUT);
	gpio_set_direction(detector_step_pin, GPIO_MODE_OUTPUT);
	gpio_set_direction(motor_nEnable, GPIO_MODE_OUTPUT);
	gpio_set_direction(motor_nReset, GPIO_MODE_OUTPUT);
	gpio_set_direction(motor_nSleep, GPIO_MODE_OUTPUT);
	gpio_set_direction(motor_nFault, GPIO_MODE_INPUT);

	gpio_set_direction(clock_pin, GPIO_MODE_INPUT);
	gpio_set_direction(aclock_pin, GPIO_MODE_INPUT);
	gpio_set_direction(SW_mode, GPIO_MODE_INPUT);

	int swmode = gpio_get_level(SW_mode);
	if(swmode==0){
		ESP_LOGI(TAG, "NC switch");
	} else {
		ESP_LOGI(TAG, "NO switch");
	}

	gpio_set_level(m_direction, 0);
	gpio_set_level(sample_step_pin, 0);
	gpio_set_level(detector_step_pin, 0);
	gpio_set_level(motor_nEnable, 0);
	gpio_set_level(motor_nSleep, 1);
	gpio_set_level(motor_nReset, 1);

	int8_t clock = gpio_get_level(clock_pin);
	printf("clock: %d \n", clock);
	int8_t aclock = gpio_get_level(aclock_pin);;
	printf("aclock: %d \n", aclock);

	if((swmode==0)&&((clock==1)||(aclock==1))){	//NC (normal 0)
		ESP_LOGI(TAG, "Wrong initial status");
		return ESP_FAIL;
	}

	if((swmode==1)&&((clock==0)||(aclock==0))){	//NO (normal 1)
		ESP_LOGI(TAG, "Wrong initial status");
		return ESP_FAIL;
	}

	gpio_set_level(m_direction, 1);	// Clockwise
	ESP_LOGI(TAG,"Move clockwise until limit");
	while(((swmode==0)&&(clock==0))||((swmode==1)&&(clock==1))){
		detector_step(1);
		clock = gpio_get_level(clock_pin);
	}

	uint steps = 0;
	ESP_LOGI(TAG,"Move anti-clockwise until limit");
	gpio_set_level(m_direction, 0);	//Anticlockwise
	while(((swmode==0)&&(aclock==0))||((swmode==1)&&(aclock==1))){
		detector_step(1);
		steps++;
		aclock = gpio_get_level(aclock_pin);
	}

	ESP_LOGI(TAG,"allocate in center");
	gpio_set_level(m_direction, 1);	//clockwise
	uint32_t neg_steps = steps/2;
	detector_step(neg_steps);

	status.nstep_neg = neg_steps;
	status.nstep_pos = steps - neg_steps;

	float max_angle = status.nstep_pos * 17.777777778;
	printf("nsteps: %d, pos_steps: %d, neg_steps: %d, max_angle: %f \n", steps, status.nstep_pos, status.nstep_neg, max_angle);

	return ESP_OK;
}

#endif /* MAIN_MOTORS_H_ */
