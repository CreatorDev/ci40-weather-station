/*
 * wu.h
 *
 *  Created on: 24.03.2017
 *      Author: marek
 */

#ifndef CI40_WEATHER_STATION_WU_H_
#define CI40_WEATHER_STATION_WU_H_

#include "types.h"

void wu_init(char *id, char *pass, unsigned int sleep);
void wu_release(void);
void wu_send_measurements(struct measurement*);

#endif /* CI40_WEATHER_STATION_WU_H_ */
