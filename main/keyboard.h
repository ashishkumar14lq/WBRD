/*
 * teclado.h
 *
 *  Created on: 12 mar. 2020
 *      Author: Julian
 */

#ifndef MAIN_TECLADO_H_
#define MAIN_TECLADO_H_

// Keypad:   - + SET
#define but_minus	36	// E1: -
#define but_plus	37	// E2: +
#define but_set		38	// E3: SET

#define bmin_flag 	0x01
#define bplus_flag 	0x02
#define set_flag	0x04

void keyboard_gpio_initilize(void)
{
	gpio_pad_select_gpio(but_minus);
	gpio_pad_select_gpio(but_plus);
	gpio_pad_select_gpio(but_set);

	gpio_set_direction(but_minus, GPIO_MODE_INPUT);
	gpio_set_direction(but_plus, GPIO_MODE_INPUT);
	gpio_set_direction(but_set, GPIO_MODE_INPUT);
}

int get_key_pressed(void)
{
	int ret = 0;
	if (gpio_get_level(but_minus))
	{
		ret |= bmin_flag;
	} else if (gpio_get_level(but_plus)){
		ret |= bplus_flag;
	} else if (gpio_get_level(but_set)){
		ret |= set_flag;
	}
	return ret;
}

#endif /* MAIN_TECLADO_H_ */
