#ifndef _SENSOR_H__
#define _SENSOR_H__
#include "app.h"

extern uint8_t bSensorCheck;
boolean app_sensor_init(void);
void app_sensor_check(void);
uint8_t driver_sensor_check_up(void);
void app_sensor_get_id(void);
void app_sensor_powerdown(void);
void app_sensor_wakeup(void);

#endif
