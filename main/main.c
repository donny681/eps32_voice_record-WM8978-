/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <math.h>
#include "wm8978.h"
#include "driver/i2c.h"
#include "driver/i2s.h"
#include "i2c.h"
#include "esp_log.h"
#include "_ansi.h"
#include <errno.h>
#include <sys/fcntl.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "spiffs_vfs.h"
#include "wave.h"
#include "wifi.h"
#include "blink.h"
static const char *TAG = "Main";
#define SAMPLE_RATE     (44100)
#define I2S_NUM         (0)
#define WAVE_FREQ_HZ    (100)
#define PI 3.14159265

#define SAMPLE_PER_CYCLE (SAMPLE_RATE/WAVE_FREQ_HZ)


static void audio_ouput_task(void *pvParameters) {
	unsigned int sample_val;
	float sin_float, triangle_float, triangle_step = 65536.0 / SAMPLE_PER_CYCLE;

	//for 36Khz sample rates, we create 100Hz sine wave, every cycle need 36000/100 = 360 samples (4-bytes each sample)
	//using 6 buffers, we need 60-samples per buffer
	//2-channels, 16-bit each channel, total buffer is 360*4 = 1440 bytes
	int mode = I2S_MODE_MASTER | I2S_MODE_TX;

	i2s_config_t i2s_config = {
				.mode = (i2s_mode_t) mode,            // Only TX
				.sample_rate = SAMPLE_RATE,
				.bits_per_sample = (i2s_bits_per_sample_t) 16,          //16-bit per channel
				.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,           //2-channels
				.communication_format = I2S_COMM_FORMAT_I2S,
				.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,               //Interrupt level 1
				.dma_buf_count = 6,
				.dma_buf_len = 60                            //
			};

	i2s_pin_config_t pin_config = {
			.bck_io_num = 26,
			.ws_io_num = 25,
			.data_out_num = 22,
			.data_in_num = -1                    //Not used
			};

	i2s_driver_install((i2s_port_t) I2S_NUM, &i2s_config, 0, NULL);
	i2s_set_pin((i2s_port_t) I2S_NUM, &pin_config);

//	i2s_set_sample_rates((i2s_port_t) I2S_NUM, 22050);

	triangle_float = -32767;

	while (1) {
		for (int i = 0; i < SAMPLE_PER_CYCLE; i++) {
			sin_float = sin(i * PI / 180.0);
			if (sin_float >= 0)
				triangle_float += triangle_step;
			else
				triangle_float -= triangle_step;
			sin_float *= 32767;

			sample_val = 0;
			sample_val += (short) triangle_float;
			sample_val = sample_val << 16;
			int ret = i2s_push_sample((i2s_port_t) I2S_NUM, (char *) &sample_val, portMAX_DELAY);

//			ESP_LOGI(TAG, "i2s_push_sample return:%d", ret);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}






void app_main()
{
    printf("psraw_task!\n");

    i2c_master_init();
    if(WM8978_Init())
    {
    	esp_restart();
    }

    vfs_spiffs_register();
    initialise_wifi();
//    xTaskCreate(&audio_ouput_task, "audio_ouput_task", 8192, NULL, 5, NULL);
	if (spiffs_is_mounted) {
		vTaskDelay(2000 / portTICK_RATE_MS);//延时稳定
//    xTaskCreate(&start_record, "start_record", 8192, NULL, 3, NULL);
    xTaskCreate(blink_task, "blink_task", 2048, NULL, 5, NULL);
    xTaskCreate(&http_server, "http_server", 15000, NULL, 4, NULL);
//    xTaskCreate(&Record_task, "Record_task", 15000, NULL, 3, NULL);
//    xTaskCreate(&printf_task, "printf_task", 12000, NULL, 5, NULL);
    wm8978_record();
	}else{
		ESP_LOGI(TAG,"spiffs_is not_mounted");

	}

}
