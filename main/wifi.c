#include "wifi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "wave.h"
static const char *TAG = "wifi";
#define FREAD_LEN 4096
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
static ip4_addr_t s_ip_addr;

int state = 0;
const static char http_hdr[] = "HTTP/1.1 200 OK\r\n";
const static char http_stream_hdr[] =
		"Content-type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n\r\n";
const static char http_pcm_hdr[] =
		"Content-type: application/octet-stream\r\n\r\n";
//const static char http_pcm_hdr[] =
//		"Content-type: audio/wav\r\n\r\n";
const static char http_pgm_hdr[] =
		"Content-type: image/x-portable-graymap\r\n\r\n";
const static char http_stream_boundary[] =
		"--123456789000000000000987654321\r\n";

static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		s_ip_addr = event->event_info.got_ip.ip_info.ip;
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently
		 auto-reassociate. */
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}

void initialise_wifi(void) {
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	wifi_config_t wifi_config = { .sta = { .ssid = CONFIG_WIFI_SSID, .password =
			CONFIG_WIFI_PASSWORD, }, };
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MODEM));
	ESP_LOGI(TAG, "Connecting to \"%s\"", wifi_config.sta.ssid);
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true,
	portMAX_DELAY);
	ESP_LOGI(TAG, "Connected");

}
char *data = NULL;
void http_server_netconn_serve(struct netconn *conn) {
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	/* Read the data from the port, blocking if nothing yet there.
	 We assume the request (the part we care about) is in one netbuf */
	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		netbuf_data(inbuf, (void**) &buf, &buflen);

		FILE *fp = fopen("/spiffs/images/test.wav", "rb+");
		if (fp == NULL) {
			ESP_LOGE(TAG, "     Error fopen\r\n");
			return;
		}
		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		ESP_LOGI(TAG, " file size=%d\r\n",size);
		char *data = (char *) malloc(FREAD_LEN);
		size_t res = 0;
//		ESP_LOGE(TAG, "    success malloc %d\r\n",size);
////		memset(data, 0, size);
//		size_t res = fread(data, 1, size, fp);
//		if (res <= 0) {
//			ESP_LOGE(TAG, "fwrite err\r\n");
//			return;
//		}
//		ESP_LOGI(TAG, "fread success\r\n");
//		fclose(fp);
		/* Is this an HTTP GET command? (only check the first 5 chars, since
		 there are other formats for GET, and we're keeping it very simple )*/
		if (buflen >= 5 && buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T'
				&& buf[3] == ' ' && buf[4] == '/') {
			/* Send the HTTP header
			 * subtract 1 from the size, since we dont send the \0 in the string
			 * NETCONN_NOCOPY: our data is const static, so no need to copy it
			 */
			netconn_write(conn, http_hdr, sizeof(http_hdr) - 1, NETCONN_NOCOPY);
			//check if a stream is requested.
			netconn_write(conn, http_pcm_hdr, sizeof(http_pcm_hdr) - 1,
					NETCONN_NOCOPY);
			while (!feof(fp)) {
				memset(data, 0, FREAD_LEN);
				res = fread(data, 1, FREAD_LEN, fp);
				if (res <= 0) {
					ESP_LOGE(TAG, "fread err\r\n");
					return;
				}
				err = netconn_write(conn, data, res, NETCONN_NOCOPY);
			}
		}
		fclose(fp);
		ESP_LOGE(TAG, "fread finsih\r\n");
	}
	/* Close the connection (server closes in HTTP) */
	netconn_close(conn);

	/* Delete the buffer (netconn_recv gives us ownership,
	 so we have to make sure to deallocate the buffer) */
	netbuf_delete(inbuf);
	free(data);
}

void http_server(void *pvParameters) {
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
	} while (err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);

}
