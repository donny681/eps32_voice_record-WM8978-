#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "driver/i2s.h"
#include "i2c.h"
#include "esp_log.h"
#include "wave.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "wm8978.h"
#include "esp_log.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
bool IsStopRecord = true;
static const char *TAG = "WAVE";
#define SAMPLE_RATE     (8000)
#define MCLK_FREQ (SAMPLE_RATE*256)
#define I2S_NUM         (0)
#define WAVE_FREQ_HZ    (100)
#define PI 3.14159265
#define SAMPLE_PER_CYCLE (SAMPLE_RATE/WAVE_FREQ_HZ)
QueueHandle_t *event_queue;
intr_handle_t i2s_isr_handler;
char *buf = NULL;
static void IRAM_ATTR i2s_isr(void* arg) {
	esp_intr_disable(i2s_isr_handler);
	I2S0.int_clr.val = I2S0.int_raw.val;
	printf("isr");
	esp_intr_enable(i2s_isr_handler);
}

void RecordVoice() {
	unsigned int sample_val;
	float sin_float, triangle_float, triangle_step = 65536.0 / SAMPLE_PER_CYCLE;

	//for 36Khz sample rates, we create 100Hz sine wave, every cycle need 36000/100 = 360 samples (4-bytes each sample)
	//using 6 buffers, we need 60-samples per buffer
	//2-channels, 16-bit each channel, total buffer is 360*4 = 1440 bytes
	int mode = I2S_MODE_MASTER | I2S_MODE_RX;

	i2s_config_t i2s_config = { .mode = (i2s_mode_t) mode,            // Only TX
			.sample_rate = SAMPLE_RATE, .bits_per_sample =
					I2S_BITS_PER_SAMPLE_16BIT,          //16-bit per channel
			.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,           //2-channels
			.communication_format = I2S_COMM_FORMAT_I2S, .intr_alloc_flags =
			ESP_INTR_FLAG_LEVEL1,               //Interrupt level 1
			.dma_buf_count = 32, .dma_buf_len = 64                           //
			};

	i2s_pin_config_t pin_config = { .bck_io_num = 26, .ws_io_num = 25,
			.data_out_num = -1, .data_in_num = 23                    //Not used
			};

//	i2s_driver_install((i2s_port_t) I2S_NUM, &i2s_config, 0, NULL);
	i2s_driver_install((i2s_port_t) I2S_NUM, &i2s_config,
			i2s_config.dma_buf_count, &event_queue);
	i2s_set_pin((i2s_port_t) I2S_NUM, &pin_config);
	i2s_stop((i2s_port_t) I2S_NUM);
//	int cnt = 0;
//	buf = calloc(DATA_SIZE, sizeof(char));
//	struct timeval tv = { 0 };
//	struct timezone *tz = { 0 };
//	gettimeofday(&tv, &tz);
//	uint64_t micros = tv.tv_usec + tv.tv_sec * 1000000;
//	uint64_t micros_prev = micros;
//	uint64_t delta = 0;
//	while (1) {
//		int bytes_read = 0;
//		while (bytes_read == 0) {
//			bytes_read = i2s_read_bytes((i2s_port_t) I2S_NUM, buf, DATA_SIZE,
//					0);
//		}
//		uint32_t samples_read = bytes_read / 2
//				/ (I2S_BITS_PER_SAMPLE_16BIT / 8);
////		for (int i = 0; i < samples_read; i++) {
////			int left=(buf[1]<<8)+buf[0];
////
////			printf("\t%d\t%d",,(buf[2]<<8+buf[1]));
////		}
//
//		cnt += samples_read;
//		if (cnt >= SAMPLE_RATE) {
//			gettimeofday(&tv, &tz);
//			micros = tv.tv_usec + tv.tv_sec * 1000000;
//			delta = micros - micros_prev;
//			micros_prev = micros;
//			printf("%d samples in %" PRIu64 " usecs\n", cnt, delta);
//
//			cnt = 0;
//		}
//	}
}

int RecordFileInit(char *dirname, FILE *fp, __WaveHeader WaveHead) {
	struct stat st = { 0 };
	int res;
	if (stat(dirname, &st) == -1) {
		ESP_LOGE(TAG, "file %s not here\r\n", dirname);

	} else {
		ESP_LOGI(TAG, "file %s is here\r\n", dirname);
		res = remove(dirname);
		if (res != 0) {
			ESP_LOGE(TAG, "     Error removing directory (%d)\r\n", res);
			return -1;
		}
		ESP_LOGE(TAG, "      removing directory (%d) successfully\r\n", res);
	}
	fp = fopen("/spiffs/images/test.pcm", "wb+");
	if (fp == NULL) {
		ESP_LOGE(TAG, "     Error fopen\r\n");
		return -1;
	}

	res = fwrite((const void*) (&WaveHead), sizeof(__WaveHeader ), 1, fp);
	if (res <= 0) {
		ESP_LOGE(TAG, "fwrite err\r\n");
		return -1;
	}
	ESP_LOGI(TAG, "fwrite success\r\n");
//	fclose(fp);
	return 0;
}

#define MCLK_IO 21
esp_err_t mclk_enable_out_clock(void) {
	printf("mclk_enable_out_clock\r\n");
	periph_module_enable(PERIPH_LEDC_MODULE);

	ledc_timer_config_t timer_conf;
	timer_conf.bit_num = 1;
	timer_conf.freq_hz = MCLK_FREQ;
	timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
	timer_conf.timer_num = LEDC_TIMER_0;
	esp_err_t err = ledc_timer_config(&timer_conf);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ledc_timer_config failed, rc=%x", err);
		return err;
	}

	ledc_channel_config_t ch_conf;
	ch_conf.channel = LEDC_CHANNEL_0;
	ch_conf.timer_sel = LEDC_TIMER_0;
	ch_conf.intr_type = LEDC_INTR_DISABLE;
	ch_conf.duty = 1;
	ch_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
	ch_conf.gpio_num = MCLK_IO;
	err = ledc_channel_config(&ch_conf);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ledc_channel_config failed, rc=%x", err);
		return err;
	}
	return ESP_OK;
}

void wm8978_record() {
	WM8978_ADDA_Cfg(0, 1);										//开启adc
	WM8978_Input_Cfg(1, 1, 0);							//开启输入通道(MIC&LINE_IN)
	WM8978_Output_Cfg(0, 1);						//开启BYPASS输出，可以在耳机中听到录音的声音
	WM8978_MIC_Gain(46);									//
	WM8978_I2S_Cfg(2, 0);										//飞利浦标准，16位数据长度
	WM8978_SPKvol_Set(15);										//喇叭音量设置为0，避免啸叫
	mclk_enable_out_clock();
	RecordVoice();
}
char *save_data = NULL;
#define SAVE_DATA_LEN (1024*100)
void start_record(void *pvParameters) {
	int res = 0;
	char *dirname = "/spiffs/images/test.wav";
	printf("start_record!\n");
	uint32_t RecDataSize = 0;
	FILE *fp = NULL;
	__WaveHeader WaveHead;
	WaveHead.riff.ChunkID = 0X46464952;				//"RIFF"
	WaveHead.riff.ChunkSize = 0;							//还未确定,最后需要计算
	WaveHead.riff.Format = 0X45564157; 				//"WAVE"
	WaveHead.fmt.ChunkID = 0X20746D66; 				//"fmt "
	WaveHead.fmt.ChunkSize = 16; 							//大小为16个字节
	WaveHead.fmt.AudioFormat = 0X01; 			//0X01,表示PCM;0X01,表示IMA ADPCM
	WaveHead.fmt.NumOfChannels = 2;						//双声道
	WaveHead.fmt.SampleRate = 44100;					//16Khz采样率 采样速率
	WaveHead.fmt.ByteRate = WaveHead.fmt.SampleRate * 4;//字节速率=采样率*通道数*(ADC位数/8)
	WaveHead.fmt.BlockAlign = 4;							//块大小=通道数*(ADC位数/8)
	WaveHead.fmt.BitsPerSample = 16;					//16位PCM
	WaveHead.data.ChunkID = 0X61746164;				//"data"
	WaveHead.data.ChunkSize = 0;							//数据大小,还需要计算
//	res = RecordFileInit("/spiffs/images/test.pcm", fp, WaveHead);
//	if (res < 0) {
//		ESP_LOGE(TAG, "RecordFileInit err");
//	} else {
//		ESP_LOGE(TAG, "RecordFileInit success");
//	}
	struct stat st = { 0 };
	if (stat(dirname, &st) == -1) {
		ESP_LOGE(TAG, "file %s not here\r\n", dirname);

	} else {
		ESP_LOGI(TAG, "file %s is here\r\n", dirname);
		res = remove(dirname);
		if (res != 0) {
			ESP_LOGE(TAG, "     Error removing directory (%d)\r\n", res);
		}
		ESP_LOGE(TAG, "      removing directory (%d) successfully\r\n", res);
	}
	fp = fopen(dirname, "wb+");
	if (fp == NULL) {
		ESP_LOGE(TAG, "     Error fopen\r\n");
	}

//	res = fwrite((const void*) (&WaveHead), sizeof(__WaveHeader ), 1, fp);
//	if (res <= 0) {
//		ESP_LOGE(TAG, "fwrite err\r\n");
//	}
	ESP_LOGI(TAG, "fwrite success\r\n");
	buf = (char *) malloc(DATA_SIZE);
//	save_data= (char *) malloc(SAVE_DATA_LEN);
	save_data = calloc(SAVE_DATA_LEN, sizeof(char));
	int save_data_counter = 0;
	i2s_event_t event;
	vTaskDelay(2000 / portTICK_RATE_MS);							//延时稳定
	/*time*/
	struct timeval tv = { 0 };
		struct timezone *tz = { 0 };
		gettimeofday(&tv, &tz);
		uint64_t micros = tv.tv_usec + tv.tv_sec * 1000000;
		uint64_t micros_prev = micros;
		uint64_t delta = 0;
	/*******************/
	while (1) {
		memset(buf, 0, DATA_SIZE);
		if (!IsStopRecord) {
			if (fp != NULL) {
				if (xQueueReceive(event_queue, &event,
						3 *portTICK_PERIOD_MS) == pdTRUE) {
					if (event.type == I2S_EVENT_RX_DONE) {
						res = i2s_read_bytes((i2s_port_t) I2S_NUM, buf,
						DATA_SIZE, 1 / portTICK_PERIOD_MS);
						i2s_zero_dma_buffer((i2s_port_t) I2S_NUM);
//						buf[res]='\0';
						RecDataSize = RecDataSize + res;
						ESP_LOGE(TAG, "xQueueReceive %d,%d\r\n", event.size,
								res);
						uint32_t samples_read = res / 2
								/ (I2S_BITS_PER_SAMPLE_16BIT / 8);
						for (int i = 0; i < samples_read; i++) {
							save_data[save_data_counter++] = buf[i * 4];
							save_data[save_data_counter++] = buf[i * 4 + 1];
						}
					}
				}
			} else {
				ESP_LOGE(TAG, "fp err\r\n");

			}
		} else {
			/*time*/
			gettimeofday(&tv, &tz);
			micros = tv.tv_usec + tv.tv_sec * 1000000;
			delta = micros - micros_prev;
			micros_prev = micros;
			printf("samples in %" PRIu64 " usecs\n", delta);
			/************/
			WaveHead.riff.ChunkSize = RecDataSize + 36;			//整个文件的大小-8;
			WaveHead.data.ChunkSize = RecDataSize;				//数据大小

			res = fseek(fp, 0, SEEK_SET);						//偏移到文件头.
			printf("res=%d\r\n", res);
//		res = fwrite((const void*) (&WaveHead), sizeof(__WaveHeader ), 1, fp);
			if(fp==NULL)
			{
				ESP_LOGE(TAG, "fp err\r\n");

			}
			res = fwrite((const void*) save_data, sizeof(char),
					save_data_counter , fp);
			if (res <= 0) {
				ESP_LOGE(TAG, "fwrite err\r\n");
			}else{
				printf("end reconrd\r\nRecDataSize=%d,save_data_counter=%d,fwrite=%d\r\n",
									RecDataSize, save_data_counter,res);

			}
			fclose(fp);
			free(buf);
			free(save_data);
			save_data_counter = 0;
			vTaskDelete(NULL);
		}
	}
}

