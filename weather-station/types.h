/*
 * types.h
 *
 *  Created on: 24.03.2017
 *      Author: marek
 */

#ifndef CI40_WEATHER_STATION_TYPES_H_
#define CI40_WEATHER_STATION_TYPES_H_

typedef enum {
    ClickType_None,
    ClickType_Thermo3,
    ClickType_Weather,
    ClickType_Thunder,
    ClickType_AirQuality,
    ClickType_CODetector
} ClickType;

typedef enum {
    MeasurementType_None,
    MeasurementType_Temperature,
    MeasurementType_Humidity,
    MeasurementType_Pressure,
    MeasurementType_AirQuality,
    MeasurementType_COConcentration,
    MeasurementType_ThunderDistance,
    MeasurementType_ThunderPower,
} MeasurementType;

struct measurement {
    struct measurement *next;
    int objID;
    int instance;
    float value;
    MeasurementType type;
};

#endif /* CI40_WEATHER_STATION_TYPES_H_ */
