#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "wave.h"
#include "driver/i2s.h"
#include "driver/periph_ctrl.h"
#define I2S_NUM         (0)
#define RECORD_START_BTN 12
#define RECORD_STOP_BTN 15
#define GPIO_INPUT_PIN_SEL  ((1<<RECORD_START_BTN) | (1<<RECORD_STOP_BTN))
#define ESP_INTR_FLAG_DEFAULT 0
static xQueueHandle gpio_evt_queue = NULL;
static void IRAM_ATTR gpio_isr_handler(void* arg) {
	uint32_t gpio_num = (uint32_t) arg;
	//remove isr handler for gpio number.
	if(gpio_get_level(gpio_num)==0)
	  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);

}

void blink_task(void *pvParameter) {
	char start_record_flag=0;//检测录音任务是否重复构建
	 uint32_t io_num;
	//create a queue to handle gpio event from isr
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	gpio_config_t io_conf;
	//interrupt of rising edge
	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	//bit mask of the pins, use GPIO4/5 here
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	//change gpio intrrupt type for one pin
	gpio_set_intr_type(RECORD_START_BTN, GPIO_INTR_POSEDGE);
	gpio_set_intr_type(RECORD_STOP_BTN, GPIO_INTR_POSEDGE);
	//install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(RECORD_START_BTN, gpio_isr_handler,
			(void*) RECORD_START_BTN);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(RECORD_STOP_BTN, gpio_isr_handler, (void*) RECORD_STOP_BTN);
	while (1) {
		vTaskDelay(200 / portTICK_RATE_MS);
		if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			printf("GPIO[%d] intr\r\n", io_num);
			switch (io_num) {
				case RECORD_START_BTN:
					//开始录音

					if(start_record_flag==0)
					{
					printf("begin to record\r\n");
					IsStopRecord=false;
					i2s_start((i2s_port_t) I2S_NUM);
					 xTaskCreate(&start_record, "start_record", 10000, NULL, 3, NULL);
					 start_record_flag=1;
					 i2s_zero_dma_buffer((i2s_port_t) I2S_NUM);
					}
					break;
				case RECORD_STOP_BTN:
					//停止录音
					if(start_record_flag==1)
					{
						IsStopRecord=true;
						start_record_flag=0;
						i2s_stop((i2s_port_t) I2S_NUM);
						printf("stop to record\r\n");
					}
					break;
				default:
					break;
			}
		}
	}
}
