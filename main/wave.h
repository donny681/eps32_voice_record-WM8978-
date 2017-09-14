#ifndef __WAVE_H
#define __WAVE_H
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include "freertos/queue.h"
#include "esp_err.h"
#include <stdbool.h>
#define DATA_SIZE 4096
//RIFF块
typedef  struct __attribute__((packed))
		{
	uint32_t ChunkID;			//chunk id固定为"RIFF",即0X46464952
	uint32_t ChunkSize;			//集合大小=文件总大小-8
	uint32_t Format;			//格式;WAVE,即0X45564157
}ChunkRIFF;
//fmt块
typedef struct __attribute__((packed))
{
	uint32_t ChunkID;		   		//chunk id;这里固定为"fmt ",即0X20746D66
	uint32_t ChunkSize ;		  //子集合大小(不包括ID和Size);这里为:20.
	uint16_t AudioFormat;	  	//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	uint16_t NumOfChannels;		//通道数量;1,表示单声道;2,表示双声道;
	uint32_t SampleRate;			//采样率;0X1F40,表示8Khz
	uint32_t ByteRate;				//字节速率;
	uint16_t BlockAlign;			//块对齐(字节);
	uint16_t BitsPerSample;		//单个采样数据大小;4位ADPCM,设置为4
	//uint16_t ByteExtraData;		//附加的数据字节;2个; 线性PCM,没有这个参数
}ChunkFMT;
//fact块
typedef  struct __attribute__((packed))
{
	uint32_t ChunkID;		   		//chunk id;这里固定为"fact",即0X74636166;线性PCM没有这个部分
	uint32_t ChunkSize ;		  //子集合大小(不包括ID和Size);这里为:4.
	uint32_t FactSize;	  		//转换成PCM的文件大小;
}ChunkFACT;
//LIST块
typedef  struct __attribute__((packed))
{
	uint32_t ChunkID;		   		//chunk id;这里固定为"LIST",即0X74636166;
	uint32_t ChunkSize ;		  //子集合大小(不包括ID和Size);这里为:4.
}ChunkLIST;

//data块
typedef struct __attribute__((packed))
{
	uint32_t ChunkID;		   		//chunk id;这里固定为"data",即0X5453494C
	uint32_t ChunkSize ;		  //子集合大小(不包括ID和Size)
}ChunkDATA;

//wav头
typedef struct __attribute__((packed))
{
	ChunkRIFF riff;						//riff块
	ChunkFMT fmt;  						//fmt块
	//ChunkFACT fact;						//fact块 线性PCM,没有这个结构体
	ChunkDATA data;						//data块
}__WaveHeader;

//wav 播放控制结构体
typedef struct __attribute__((packed))
{
	uint16_t audioformat;			//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	uint16_t nchannels;				//通道数量;1,表示单声道;2,表示双声道;
	uint16_t blockalign;			//块对齐(字节);
	uint32_t datasize;				//WAV数据大小
	uint32_t totsec ;					//整首歌时长,单位:秒
	uint32_t cursec ;					//当前播放时长
	uint32_t bitrate;	   			//比特率(位速)
	uint32_t samplerate;			//采样率
	uint16_t bps;							//位数,比如16bit,24bit,32bit
	uint32_t datastart;				//数据帧开始的位置(在文件里面的偏移)
}wavctrl;

#define WAVEFILEBUFSIZE		32768

bool IsStopRecord;

int RecordFileInit(char *dirname, FILE *fp,__WaveHeader WaveHead) ;
void printf_task(void *pvParameters);
esp_err_t mclk_enable_out_clock(void);
void wm8978_record();
void RecordVoice();
void start_record(void *pvParameters);
#endif
