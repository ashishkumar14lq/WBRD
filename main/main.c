#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <driver/adc.h>
#include "stdio.h"
#include "string.h"
#include "esp32/rom/ets_sys.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_task_wdt.h"

#include "propLCD.h"

esp_err_t event_handler(void *ctx, system_event_t *event);

/* Configurations */

#define ESP_WIFI_SELF_SSID	"OPTIWSPRS"
#define ESP_WIFI_SELF_PASS	"Optiregion@1"
#define ESP_MAX_STA_CONN		1

#define default_step 1
#define step_step 0.1
#define min_step 0.1
#define max_step 2

#define delay_between_autosteps 100	//ms

/* PIN definitions */
#define laser_en 	9   // High enable laser output

#define AN50UW 	35	// ADC 7
#define AN5MW 	34	// ADC 6

/* Type definitions */

typedef enum {
	_5mw,
	_50uw,
} gain_type;

typedef enum {
	pre_init,
	initializing,
	set_step,
	set_gain,
	working,
} mode;

// This structure is used to save the complete status of the mechanical system
typedef struct {
	gain_type gain;
	float step_size;
	signed steps;
	float angle;
	mode mode;
	int nstep_pos;
	int nstep_neg;
	int limit_reached;
	int screen_flag;
	int32_t pretime;
} system_status_type;


/* Global variables */

// status has the complete status of the mechanical system
system_status_type status = {
		.gain = _5mw,
		.step_size = default_step,
		.steps = 0,
		.angle = 0,
		.mode = pre_init,
		.nstep_pos = 0,
		.nstep_neg = 0,
		.limit_reached = 0,
		.screen_flag = 0,
		.pretime = 0,
};



#include "motors.h"
#include "keyboard.h"

void neg_step(void);
void pos_step(void);



int http_update(int param, char* value){
	if (param == 1) // gain
	{
		int gain_mode = atoi(value);		// Mode 1: 5mW, Mode 2: 50uw
		printf("Mode: %s\n", value);
		printf("Mode: %d\n", gain_mode);
		if(gain_mode == 1)
		{
			printf("5mw\n");
			status.gain = _5mw;
		}
		else if (gain_mode == 2)
		{
			status.gain = _50uw;
		}
		return 1;
	}
	else if (param == 2)
	{
		printf("\n\n");
		float angle = strtof(value, NULL);				// Get angle from string
		printf("angle: \n");
		float new_step_dec = (angle * steps_per_degree);		// Calculate final steps position



		signed lower_limit = 0 - (signed)status.nstep_neg;		// Lower limit in number of steps with sign
		printf("Angle: %f, steps future: %d. Range: %d - %d\n", angle, (int)new_step_dec, lower_limit, status.nstep_pos);

		if (((signed int)new_step_dec >= lower_limit) && ((signed int)new_step_dec <= (signed int)status.nstep_pos))
		{
				status.angle = angle;
				printf("readed angle: %f, saved angle: %f\n", angle, status.angle);
				signed int nsteps = (signed int)new_step_dec - status.steps;
				printf("Need to make %d steps\n", nsteps);
				if (nsteps > 0)
				{
					printf("Move clockwise \n");
					gpio_set_level(m_direction, 1); // Clockwise
					ets_delay_us(1);
					printf("Mode detector %d steps \n", nsteps);
					detector_step(nsteps);
					printf("Mode sample %d steps \n", 2*nsteps);
					sample_step(2*nsteps);
					status.steps = (signed)new_step_dec;
					status.limit_reached = 0;
					printf("Done \n");
				}
				else
				{
					printf("Move anti-clockwise \n");
					nsteps = abs(nsteps);
					gpio_set_level(m_direction, 0);	//Anticlockwise

					ets_delay_us(1);
					printf("Mode detector %d steps \n", nsteps);
					detector_step(nsteps);
					printf("Mode sample %d steps \n", 2*nsteps);
					sample_step(2*nsteps);
					status.steps = (signed)new_step_dec;
					status.limit_reached = 0;
					printf("Done. Angle: %f\n", status.angle);
				}
				return 2;		// OK
		}
		printf("Impossible, out of range\n");
		return 3;		// Out of range
	}
	return 0;	// Wrong parameter

}

int get_read(void);
#include "HTTP_server.h"

/* WiFi resources */

/* WiFi type definitions*/

typedef enum {
	softap,
	sta,
	uninitialized,
} connection_type;

typedef enum {
	connected,
	disconnected,
} connection_stat;

typedef struct {
	connection_type type;
	connection_stat status;
} connection_data;

connection_data wifi_stat = {
	.type = uninitialized,
	.status = disconnected,
};

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}


/* WiFi - FreeRTOS event group to signal when we are connected*/
//static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void wifi_connect(void);

static void wifi_softap_event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        wifi_stat.status = connected;
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
        wifi_stat.status = disconnected;
    }
}

void wifi_init_softap(void)
{
	nvs_flash_init();
	tcpip_adapter_init();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_softap_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SELF_SSID,
            .ssid_len = strlen(ESP_WIFI_SELF_SSID),
            .password = ESP_WIFI_SELF_PASS,
            .max_connection = ESP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_stat.type = softap;
    wifi_stat.status = disconnected;
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_connect(void){
	ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
	wifi_init_softap();
}

/* APP functions */

void neg_step(void)
{

	float new_angle =  status.angle - status.step_size;
	float new_step_dec = new_angle * steps_per_degree;
	signed int nsteps = status.steps - (signed int)new_step_dec;


	if (abs(status.steps - nsteps) > status.nstep_neg)
	{
		// steps to the limit
		nsteps = status.nstep_neg - abs(status.steps) ;
	}

	if (nsteps > 0)
	{
		gpio_set_level(m_direction, 0); // Anti-clockwise
		ets_delay_us(1);
		detector_step(nsteps);
		sample_step(2*nsteps);
		status.steps = status.steps - nsteps;
		status.angle -= status.step_size;
		status.limit_reached = 0;

		printf("%d steps. Final angle: %4.1f. \n", nsteps, status.angle);
	}
	else
	{
		printf("No steps, limit reached\n");
		status.limit_reached = 1;
	}
}

void pos_step(void)
{
	const float steps_per_degree = 17.7777777778;

	float new_angle =  status.angle + status.step_size;
	float new_step_dec = new_angle * steps_per_degree;
	signed int nsteps = (int)new_step_dec - status.steps;

	if (((status.steps + nsteps) > status.nstep_pos))
	{
		// steps to the limit
		nsteps = status.nstep_pos - status.steps;
		printf("%d steps to the limit\n", nsteps);
	}

	if (nsteps > 0)
	{
		gpio_set_level(m_direction, 1); // Clockwise
		ets_delay_us(1);
		detector_step(nsteps);
		sample_step(2*nsteps);
		status.steps = status.steps + nsteps;
		status.angle += status.step_size;
		status.limit_reached = 0;

		printf("%d steps. Final angle: %5.1f. \n", nsteps, status.angle);
	}
	else
	{
		printf("No steps, limit reached\n");
		status.limit_reached = 1;
	}
}

void key_handle(int key)
{
	printf("key:  %d\n", key);
	// key: 1 minus; 2 plus; 3 set
	//if (status.mode == set_step)
	if (status.mode == set_step)
	{
		printf("set_step");
		switch (key)
		{
			case 1:		// Minus button
				if ((status.step_size - step_step) >= min_step)	{
					status.step_size = status.step_size - step_step;
				}
				else
				{
					status.step_size = min_step;
				}
				break;
			case 2:		// Plus button
				if ((status.step_size + step_step) <= max_step)	{
					status.step_size = status.step_size + step_step;
				}
				else
				{
					status.step_size = max_step;
				}
				break;
			case 3:		// SET button
				status.mode = set_gain;
				break;
		}
	}
	else if (status.mode == set_gain)
	{
		printf("set_gain");
		switch (key)
		{
			case 1:		// Minus button
				if (status.gain == _5mw) {status.gain = _50uw;}
				else {status.gain = _5mw;}
				break;
			case 2:		// Plus button
				if (status.gain == _5mw) {status.gain = _50uw;}
				else {status.gain = _5mw;}
				break;
			case 3:		// SET button
				status.mode = working;
				gpio_set_level(laser_en, 1);
				break;
		}
	}
	else if (status.mode == working)
	{
		printf("working");
		switch (key)
		{
			case 1:		// Minus button
				neg_step();
				break;
			case 2:		// Plus button
				pos_step();
				break;
			case 3:		// SET button
				status.mode = set_step;
				gpio_set_level(laser_en, 0);
				break;
		}

	}
}

int get_read(void)
{
	double cumsum = 0;
	if (status.gain == _50uw)
	{
		adc1_config_width(ADC_WIDTH_BIT_12);
		adc1_config_channel_atten(ADC1_CHANNEL_7,ADC_ATTEN_DB_11);
		for (int i = 1;i<=20;i++)
		{
			int val = adc1_get_raw(ADC1_CHANNEL_7);
			cumsum = cumsum + val;
		}
	}
	else
	{
		adc1_config_width(ADC_WIDTH_BIT_12);
		adc1_config_channel_atten(ADC1_CHANNEL_6,ADC_ATTEN_DB_11);
		for (int i = 1;i<=20;i++)
		{
			int val = adc1_get_raw(ADC1_CHANNEL_6);
			cumsum = cumsum + val;
		}
	}
	cumsum = cumsum/20;
	int answer = (int)cumsum;
	return answer;
}

// Write the first line of the display
void disp_line1(char* text){
	lcd_goto(1,1);
	lcd_write_string(text);
}

// Write the second line of the display
void disp_line2(char* text){
	lcd_goto(2,1);
	lcd_write_string(text);
}

int are_equal(system_status_type A, system_status_type B)
{
	int equal = 1;
	if (A.angle != B.angle) {equal = 0;}
	if (A.gain != B.gain) {equal = 0;}
	if (A.limit_reached != B.limit_reached) {equal = 0;}
	if (A.mode != B.mode) {equal = 0;}
	if (A.screen_flag != B.screen_flag) {equal = 0;}
	if (A.step_size != B.step_size) {equal = 0;}
	if (A.limit_reached != B.limit_reached) {equal = 0;}
	return equal;
}

// Update only the values of the position of the motors
void display_semi_update(void)
{
	char text[17];
	disp_line1("                ");
	if (status.limit_reached)
	{
		disp_line2("  LIMIT REACHED  ");
	}
	else
	{
		sprintf(text, " S=%04.1f D=%04.1f ", status.angle, 2*status.angle);
		disp_line2(text);
	}
}

// Handle the display
void display_handler(void)
{
	static system_status_type backup;
	char text[17];

	if (!are_equal(status, backup)){
		if (status.mode == set_step)
		{
			disp_line1(" SET STEP ANGLE ");
			sprintf(text, " S=%04.1f  D=%04.1f ", status.step_size, 2*status.step_size);
			disp_line2(text);
		}
		else if (status.mode == set_gain)
		{
			disp_line1("    SET GAIN    ");
			if (status.gain == _50uw) {disp_line2("     50 uW      "); }
			else {disp_line2("      5 mW      ");}
		}
		else if (status.mode == working)
		{
			int value = get_read();
			sprintf(text, " S=%04.1f D=%04.1f ", status.angle, 2*status.angle);
			disp_line2(text);

			float power;
			if (status.gain == _50uw)
			{
				power = (float)value / 4095 * 50;
				sprintf(text, "  INT: %04.1f uW  ", power);
				disp_line1(text);
			} else {
				power = (float)value / 4095 * 5;
				sprintf(text, "  INT: %04.2f mW  ", power);
				disp_line1(text);
			}
		}
		backup = status;
		backup.screen_flag = 0;
		status.pretime = esp_log_timestamp();
	}
}

/* MAIN APP */

void app_main(void)
{
	gpio_pad_select_gpio(laser_en);
	gpio_set_level(laser_en, 0);
	gpio_set_direction(laser_en, GPIO_MODE_OUTPUT);


    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    lcd_init();

    disp_line1("   OPTIREGION   ");
    disp_line2(" STARTING WSPRS ");

    // Initialize the WiFi connection
    wifi_connect();
    // Initialize web server
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    webserver_init();

    // Initilize motors
    ESP_LOGI(TAG, "Initialize motor position");
    status.mode = initializing;
    if (m_initialize() == ESP_OK)
    {
    	status.mode = set_step;
    } else {
    	ESP_LOGE(TAG, "Motor initialization ERROR, reseting in 10s");
    	vTaskDelay(10000 / portTICK_PERIOD_MS);
    	esp_restart();
    }

    // Initialize keyboard
    keyboard_gpio_initilize();

	int key;
	int key_hand;
	int count;
    while (true) {

    	count = 0;
    	key_hand = 0;
    	key = get_key_pressed();

    	if(key) {printf("%d \n", key);}

    	if (key & bmin_flag)
    	{
			printf("-\n");
			key_hand = 1;
    		while(get_key_pressed() & bmin_flag)
    		{
    			vTaskDelay(10 / portTICK_PERIOD_MS);
    			if (count >= 100)
    			{
    				printf("- long \n");
    				while(get_key_pressed() == 1)
    				{
    					neg_step();
    					display_semi_update();
    					vTaskDelay(delay_between_autosteps / portTICK_PERIOD_MS);
    				}
    				printf("Release\n");
    			}
    				else if (status.mode == working) {count++;}
    		}
    	}

    	if (key & bplus_flag)
    	{
			printf("+\n");
			key_hand = 2;
    		while(get_key_pressed() & bplus_flag)
    		{
    			vTaskDelay(10 / portTICK_PERIOD_MS);
    			if (count >= 100)
    			{
    				printf("+ long \n");
    				while(get_key_pressed() == 2)
    				{
    					pos_step();
    					display_semi_update();
    					vTaskDelay(delay_between_autosteps / portTICK_PERIOD_MS);
    				}
    				printf("Release\n");
    			}
    				else if (status.mode == working) {count++;}
    		}
    	}

    	if (key & set_flag)
    	{
			printf("SET\n");
			key_hand = 3;
    		while(get_key_pressed() & set_flag)
    		{
    			vTaskDelay(10 / portTICK_PERIOD_MS);
    		}
    	}


		if ((count < 100) && (key_hand))
		{
			key_handle(key_hand);
		}
    	vTaskDelay(10 / portTICK_PERIOD_MS);

    	if 	(esp_log_timestamp() >= (status.pretime+500)) {status.screen_flag = 1;};
    	display_handler();
    }
}
