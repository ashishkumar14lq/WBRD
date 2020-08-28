/*
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "esp_netif.h"
#include "esp_eth.h"
*/
#include "esp_log.h"
#include <esp_http_server.h>


static const char *TAGWS = "HTTP_server";

static esp_err_t get_get_handler(httpd_req_t *req)
{
    char resp_str[20];
    int value = get_read();
    float power;

    if (status.gain == _50uw)
    	{
			power = (float)value / 4095 * 50;
			sprintf(resp_str, "  INT: %05.1f uW  ", power);
		} else {
			power = (float)value / 4095 * 5;
			sprintf(resp_str, "INT: %05.2f mW", power);
		}

    httpd_resp_send(req, resp_str, strlen(resp_str));

	return ESP_OK;
}

static esp_err_t set_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    int resp = 10;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAGWS, "Found URL query => %s", buf);
            char param[32];

            if (httpd_query_key_value(buf, "gain", param, sizeof(param)) == ESP_OK) {
            	resp = http_update(1, param);
            }

            if (httpd_query_key_value(buf, "angle", param, sizeof(param)) == ESP_OK) {
            	resp = http_update(2, param);
            }
        }
        printf("ya sali, angle: %f\n", status.angle);
        free(buf);
    }

    char resp_str[30];
    if(resp == 1)
    {
    	sprintf(resp_str, "uptd:gain ");
    }
    else if( resp == 2)
    {
    	sprintf(resp_str, "uptd:ang  ");
    }
    else if( resp == 3)
    {
    	sprintf(resp_str, "angle:OFR ");
    }
    else if (resp == 10)
    {
    	sprintf(resp_str,"IDK:comm  ");
    }

    printf("%s\n", resp_str);

    if (true)
    {
    	if (status.gain == _5mw)
    	{
    		sprintf(&resp_str[10],"OK,G=5mW,A=%5.1f", status.angle);
    	}
    	else
    	{
    		sprintf(&resp_str[10],"OK,G=50uW,A=%5.1f", status.angle);
    	}
    }
    else
    {
    	if (status.gain == _5mw)
    	{
    		sprintf(&resp_str[10],"ERROR,G=5mW,A=%5.1f", status.angle);
    	}
    	else
    	{
    		sprintf(&resp_str[10],"ERROR,G=50uW,A=%5.1f", status.angle);
    	}
    }
    printf("%s\n", resp_str);
    printf("%f\n", status.angle);

    printf("Segunda parte lista\n");

    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    printf("listo \n");
    return ESP_OK;
}


static const httpd_uri_t set = {
    .uri       = "/set",
    .method    = HTTP_GET,
    .handler   = set_get_handler,
};

static const httpd_uri_t get = {
    .uri       = "/get",
    .method    = HTTP_GET,
    .handler   = get_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAGWS, "Starting server on port: '%d'", config.server_port);


    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAGWS, "Registering URI handlers");
        httpd_register_uri_handler(server, &set);
        httpd_register_uri_handler(server, &get);
        return server;
    }

    ESP_LOGI(TAGWS, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
 }

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    printf("Disconnect handler\n");
    if (*server) {
        ESP_LOGI(TAGWS, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
	printf("Connect handler\n");
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAGWS, "Starting webserver");
        *server = start_webserver();
    }
}

//
void webserver_init(void)
{
    static httpd_handle_t server = NULL;

    ESP_LOGI(TAGWS, "Registering WiFi events");

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));
    //server = start_webserver();
}
