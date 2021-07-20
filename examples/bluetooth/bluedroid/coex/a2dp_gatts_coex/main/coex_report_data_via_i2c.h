#ifndef __COEX_REPORT_DATA_VIA_I2C_H
#define __COEX_REPORT_DATA_VIA_I2C_H


#include "freertos/semphr.h"



typedef struct {
    unsigned char is_ble;
    unsigned char  mac[6];
    unsigned short name_len;
    char name[0];
} btDev;

typedef struct {
    unsigned short total_len;
    //unsigned char *append_pointer;
    unsigned char *frame;
} btMsgBuff;

void i2c_comm_app_main();

extern btMsgBuff bt_msg;
extern bool bt_msg_ready;
extern SemaphoreHandle_t msg_mutex;


#endif

