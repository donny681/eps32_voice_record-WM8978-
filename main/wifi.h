#ifndef __WIFI_H
#define __WIFI_H
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/dirent.h>
 void initialise_wifi(void);
 void http_server(void *pvParameters);
#endif
