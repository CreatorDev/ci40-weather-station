/************************************************************************************************************************
 Copyright (c) 2016, Imagination Technologies Limited and/or its affiliated group companies.
 All rights reserved.
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 following conditions are met:
     1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
        following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
        following disclaimer in the documentation and/or other materials provided with the distribution.
     3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
        products derived from this software without specific prior written permission.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************************************************************************************/

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <letmecreate/letmecreate.h>
#include <getopt.h>
#include <string.h>
#include <awa/common.h>
#include <awa/client.h>
#include <awa/types.h>
#include "log.h"

#define OPERATION_PERFORM_TIMEOUT       (1000)
#define DEFAULT_SLEEP_TIME              (60)
#define DEFAULT_CLIENT_DAEMON_ADDRESS   "127.0.0.1"
#define DEFAULT_CLIENT_DAEMON_PORT      (12345)

#define TEMPERATURE_IPSO_OBJECT_ID      (3303)
#define HUMIDITY_IPSO_OBJECT_ID         (3304)
#define BAROMETER_IPSO_OBJECT_ID        (3315)
#define CONCENTRATION_IPSO_OBJECT_ID    (3325)
#define DISTANCE_IPSO_OBJECT_ID         (3330)
#define POWER_IPSO_OBJECT_ID            (3328)


typedef float (*SensorReadFunc)(uint8_t);

typedef enum {
    ClickType_None,
    ClickType_Thermo3,
    ClickType_Weather,
    ClickType_Thunder,
    ClickType_AirQuality,
    ClickType_CODetector
} ClickType;

static const struct element {
    char* name;
    ClickType type;
} types[] = {
    {"air", ClickType_AirQuality},
    {"co", ClickType_CODetector},
    {"thermo3", ClickType_Thermo3},
    {"thunder", ClickType_Thunder},
    {"weather", ClickType_Weather},
    {NULL, -1}
};

typedef struct {
    ClickType click1;
    ClickType click2;
    unsigned int sleepTime;     // in seconds
    char address[16];
    int port;
} options;

static const struct option long_options[] = {
    { "click1", required_argument, 0, '1' },
    { "click2", required_argument, 0, '2' },
    { "bus", required_argument, 0, 'b'},
    { "logLevel", required_argument, 0, 'v'},
    { "help", no_argument, 0, 'h'},
    { "sleep", required_argument, 0, 's'},
    { "port", required_argument, 0, 'p'},
    { 0, 0, 0, 0 }
};

struct measurement {
    struct measurement *next;
    int objID;
    int instance;
    float value;
};

int g_LogLevel = LOG_INFO;
FILE* g_DebugStream;
static volatile bool _Running = true;

static void exitApp(int __attribute__((unused))(signo)) {
    _Running = false;
}

static ClickType configDecodeClickType(char* type) {
    const struct element* iter = types;
    while (iter->name != NULL) {
        if (strcasecmp(iter->name, type) == 0)
            return iter->type;

        iter++;
    }

    return ClickType_None;
}

static void printUsage(const char *program)
{
    printf("Usage: %s [options]\n\n"
        " -1, --click1   : Type of click installed in microBus slot 1 (default:none)\n"
        "                  air, co, none, thermo3, thunder, weather\n"
        " -2, --click2   : Type of click installed in microBus slot 2 (default:none)\n"
        "                  air, co, none, thermo3, thunder, weather\n"
        " -s, --sleep    : delay between measurements in seconds. (default: %ds)\n"
        " -a, --address  : Address to connect to AWA client daemon (default: %s)\n"
        " -p, --port     : Port to connect to AWA client daemon. (default: %d)\n"
        " -v, --logLevel : Debug level from 1 to 5\n"
        "                   fatal(1), error(2), warning(3), info(4), debug(5) and max(>5)\n"
        "                   default is info.\n"
        " -h, --help     : prints this help\n",
        program, DEFAULT_SLEEP_TIME, DEFAULT_CLIENT_DAEMON_ADDRESS, DEFAULT_CLIENT_DAEMON_PORT);
}

static bool loadConfiguration(int argc, char **argv, options *opts) {
    bool success = true;
    int c;

    while ((c = getopt_long(argc, argv, "s:1:2:c:b:hv:p:a:", long_options, NULL)) != -1) {
        switch (c) {
            case '1':
                opts->click1 = configDecodeClickType(optarg);
                if (opts->click1 == ClickType_None) {
                    success = false;
                    LOG(LOG_ERROR, "Failed to parse click1 option.\n");
                }
                break;

            case '2':
                opts->click2 = configDecodeClickType(optarg);
                if (opts->click2 == ClickType_None) {
                    success = false;
                    LOG(LOG_ERROR, "Failed to parse click2 option.\n");
                }
                break;

            case 's':
                errno = 0;
                opts->sleepTime = strtoul(optarg, NULL, 10);
                if ((opts->sleepTime == ULONG_MAX && errno == ERANGE)
                ||  (opts->sleepTime == 0 && errno != 0)) {
                    success = false;
                    LOG(LOG_ERROR, "Failed to parse sleep option.\n");
                }
                break;

            case 'a':
                if (strlen(optarg) >= sizeof(opts->address)) {
                    success = false;
                    LOG(LOG_ERROR, "Failed to parse address option.\n");
                }
                strcpy(opts->address, optarg);
                break;

            case 'p':
                errno = 0;
                opts->port = strtol(optarg, NULL, 10);
                if ((errno == ERANGE && (opts->port == LONG_MAX || opts->port == LONG_MIN))
                ||  (errno != 0 && opts->port == 0)) {
                    success = false;
                    LOG(LOG_ERROR, "Failed to parse port option.\n");
                }
                break;

            case 'v':
                errno = 0;
                g_LogLevel = strtol(optarg, NULL, 10);
                if ((errno == ERANGE && (g_LogLevel == LONG_MAX || g_LogLevel == LONG_MIN))
                ||  (errno != 0 && g_LogLevel == 0)) {
                    success = false;
                    g_LogLevel = LOG_INFO; /* Revert back to default log level */
                    LOG(LOG_ERROR, "Failed to parse log level option.\n");
                }
                break;

            case 'h':
                printUsage(argv[0]);
                exit(0);

            case '?':
                /* getopt_long already printed an error message. */
                success = false;
                break;

            default:
                abort();
        }
    }

    return success;
}

static float readThermo3(uint8_t busIndex) {
    LOG(LOG_DEBUG, "Reading thermo3 on bus#%d", busIndex);
    float temperature = 0.f;

    i2c_select_bus(busIndex);
    if (thermo3_click_get_temperature(&temperature) < 0)
        LOG(LOG_ERROR, "Reading temperature measurement failed!");

    return temperature;
}

static float readCO(uint8_t busIndex) {
    LOG(LOG_DEBUG, "Reading CO on bus#%d", busIndex);
    uint16_t value = 0;

    if (co_click_get_measure(busIndex, &value) < 0)
        LOG(LOG_ERROR, "Reading CO measurement failed!");

    return value;
}

static float readAirQuality(uint8_t busIndex) {
    LOG(LOG_DEBUG, "Reading air quality on bus#%d", busIndex);
    uint16_t value = 0;

    if (air_quality_click_get_measure(busIndex, &value) < 0)
        LOG(LOG_ERROR, "Reading air quality measurement failed!");

    return value;
}

static uint8_t readWeather(uint8_t busIndex, double* data) {
    LOG(LOG_DEBUG, "Reading weather on bus#%d", busIndex);

    i2c_select_bus(busIndex);
    if (weather_click_read_measurements(&data[0], &data[1], &data[2]) < 0) {
        LOG(LOG_ERROR, "Reading weather measurements failed!");
        return -1;
    }

    return 0;
}

static AwaClientSession* connectToAwa(char *address, int port) {
    AwaClientSession *session = AwaClientSession_New();
    if (!session) {
        LOG(LOG_ERROR, "AwaClientSession_New() failed\n");
        return NULL;
    }

    if (AwaClientSession_SetIPCAsUDP(session, address, port) != AwaError_Success) {
        LOG(LOG_ERROR, "AwaClientSession_SetIPCAsUDP() failed\n");
        AwaClientSession_Free(&session);
        return NULL;
    }

    if (AwaClientSession_Connect(session) != AwaError_Success) {
        LOG(LOG_ERROR, "AwaClientSession_Connect() failed\n");
        AwaClientSession_Free(&session);
        return NULL;
    }

    LOG(LOG_INFO, "Client Session Established: %s:%d\n", address, port);

    return session;
}

static void disconnectAwa(AwaClientSession *session) {
    if (!session)
        return;

    AwaClientSession_Disconnect(session);
    AwaClientSession_Free(&session);
}

static void readSerialNumberFromEthernetMAC(char *address, int port)
{
    char mac_address[18];
    FILE *file = fopen("/sys/class/net/eth0/address", "r");
    if (!file) {
        LOG(LOG_ERROR, "Could not read ethernet mac address.\n");
        return;
    }

    if (fread(mac_address, 1, 17, file) != 17) {
        fclose(file);
        return;
    }

    fclose(file);

    mac_address[17] = '\0';

    AwaClientSession* session = connectToAwa(address, port);
    if (!session)
        return;

    AwaClientSetOperation* operation = AwaClientSetOperation_New(session);
    AwaClientSetOperation_AddValueAsCString(operation, "/3/0/2", mac_address);
    AwaError result = AwaClientSetOperation_Perform(operation, OPERATION_PERFORM_TIMEOUT);
    if (result != AwaError_Success)
        LOG(LOG_ERROR, "Failed to set of object /3/0/2");
    AwaClientSetOperation_Free(&operation);
    disconnectAwa(session);
}

static void createIPSO(AwaClientSession *session, int objectId, int instance, int resourceId) {
    AwaClientSetOperation * operation = AwaClientSetOperation_New(session);

    char buf[40];
    if (resourceId == -1) {
        sprintf(&buf[0], "/%d/%d", objectId, instance);
        AwaClientSetOperation_CreateObjectInstance(operation, &buf[0]);

    } else {
        sprintf(&buf[0], "/%d/%d/%d", objectId, instance, resourceId);
        LOG(LOG_INFO, "Creating instance of resource %s", &buf[0]);
        AwaClientSetOperation_CreateOptionalResource(operation, &buf[0]);
    }

    AwaError result = AwaClientSetOperation_Perform(operation, OPERATION_PERFORM_TIMEOUT);
    LOG(LOG_DEBUG, "Awa create response: %d", result);
    AwaClientSetOperation_Free(&operation);
}

static AwaError setIPSO(AwaClientSession *session, int objectId, int instance, int resourceId, float value) {
    char buf[40];
    sprintf(&buf[0], "/%d/%d/%d", objectId, instance, resourceId);
    LOG(LOG_INFO, "Storing value %0.3f into %s", value, &buf[0]);
    AwaClientSetOperation* operation = AwaClientSetOperation_New(session);
    AwaClientSetOperation_AddValueAsFloat(operation, &buf[0], value);
    AwaError result = AwaClientSetOperation_Perform(operation, OPERATION_PERFORM_TIMEOUT);
    LOG(LOG_DEBUG, "Awa set response: %d", result);
    AwaClientSetOperation_Free(&operation);
    return result;
}

static void setIPSOwithRetry(AwaClientSession *session, int objectId, int instance, int resourceId, float value) {
    if (setIPSO(session, objectId, instance, resourceId, value) != AwaError_Success) {
        LOG(LOG_ERROR, "Failed to set IPSO object /%d/%d/%d. Retrying...\n", objectId, instance, resourceId);
        createIPSO(session, objectId, instance, -1);
        createIPSO(session, objectId, instance, resourceId);
        setIPSO(session, objectId, instance, resourceId, value);
    }
}

static float getIPSO(AwaClientSession *session, int objectId, int instance, int resourceId, float defaultValue) {
    char buf[40];
    sprintf(&buf[0], "/%d/%d/%d", objectId, instance, resourceId);
    LOG(LOG_DEBUG, "Getting value of %s", &buf[0]);
    AwaClientGetOperation * operation = AwaClientGetOperation_New(session);

    AwaClientGetOperation_AddPath(operation, &buf[0]);
    AwaError result = AwaClientGetOperation_Perform(operation, OPERATION_PERFORM_TIMEOUT);
    if (result != AwaError_Success) {
        AwaClientGetOperation_Free(&operation);
        return defaultValue;
    }

    const AwaFloat* value;
    float resultValue;

    const AwaClientGetResponse* response = AwaClientGetOperation_GetResponse(operation);
    AwaClientGetResponse_GetValueAsFloatPointer(response, (const char*)&buf[0], &value);
    resultValue = (float)*value;
    LOG(LOG_DEBUG, "Got value %f\n", (float)resultValue);

    AwaClientGetOperation_Free(&operation);
    return resultValue;
}

static void sendMeasurement(AwaClientSession *session, struct measurement m) {
    float minValue = getIPSO(session, m.objID, m.instance, 5601, 1000);
    float maxValue = getIPSO(session, m.objID, m.instance, 5602, -1000);

    setIPSOwithRetry(session, m.objID, m.instance, 5700, m.value);
    if (minValue > m.value)
        setIPSOwithRetry(session, m.objID, m.instance, 5601, m.value);
    if (maxValue < m.value)
        setIPSOwithRetry(session, m.objID, m.instance, 5602, m.value);
}

static void addMeasurement(struct measurement **measurements, int objId, int instance, float value)
{
    struct measurement *m = malloc(sizeof(struct measurement));
    if (!m) {
        LOG(LOG_ERROR, "Failed to allocate memory for a measurement.\n");
        return;
    }

    m->next = NULL;
    m->objID = objId;
    m->instance = instance;
    m->value = value;

    if (*measurements == NULL)
        *measurements = m;
    else {
        struct measurement *last = *measurements;
        while (last->next)
            last = last->next;
        last->next = m;
    }
}

static void handleMeasurements(uint8_t bus, int objId, int instance, SensorReadFunc sensorFunc, struct measurement **measurements) {
    float value = sensorFunc(bus);
    addMeasurement(measurements, objId, instance, value);
}

static void handleWeatherMeasurements(uint8_t busIndex,
        int temperatureInstance, int pressureInstance, int humidityInstance, struct measurement **measurements) {
    double data[] = {0,0,0};
    if (readWeather(busIndex, data) < 0) {
        LOG(LOG_ERROR, "Reading weather on bus#%d failed!", busIndex);
        return;
    }
    LOG(LOG_INFO, "Reading weather measurements: temp = %f, pressure = %f, humidity = %f",
                data[0], data[1], data[2]);
    addMeasurement(measurements, TEMPERATURE_IPSO_OBJECT_ID, temperatureInstance, data[0]);
    addMeasurement(measurements, BAROMETER_IPSO_OBJECT_ID, pressureInstance, data[1]);
    addMeasurement(measurements, HUMIDITY_IPSO_OBJECT_ID, humidityInstance, data[2]);
}

static void performMeasurements(ClickType clickType, uint8_t busIndex, struct measurement **measurements, int *instances) {
    switch (clickType) {
        case ClickType_Thermo3:
            handleMeasurements(busIndex, TEMPERATURE_IPSO_OBJECT_ID, instances[0]++, &readThermo3, measurements);
            break;
        case ClickType_Weather:
            handleWeatherMeasurements(busIndex,
                    instances[0]++,
                    instances[1]++,
                    instances[2]++,
                    measurements);
            break;
        case ClickType_Thunder:
            break;
        case ClickType_AirQuality:
            handleMeasurements(busIndex, CONCENTRATION_IPSO_OBJECT_ID, instances[3]++, &readAirQuality, measurements);
            break;
        case ClickType_CODetector:
            handleMeasurements(busIndex, CONCENTRATION_IPSO_OBJECT_ID, instances[3]++, &readCO, measurements);
            break;
        default:
            break;
    }
}

static void sendMeasurements(AwaClientSession *session, struct measurement *measurements)
{
    struct measurement *ptr = measurements;
    while (ptr) {
        sendMeasurement(session, *ptr);
        ptr = ptr->next;
    }
}

static void releaseMeasurements(struct measurement *measurements)
{
    struct measurement *ptr = measurements;
    while (ptr) {
        struct measurement *next = ptr->next;
        free(ptr);
        ptr = next;
    }
}

static int initialize_click(ClickType clickType, uint8_t busIndex) {
    i2c_select_bus(busIndex);
    switch (clickType) {
        case ClickType_Thermo3:
            if (thermo3_click_enable(0) < 0) {
                LOG(LOG_ERROR, "Failed to enable thermo3 click on bus#%d\n", busIndex);
                return -1;
            }
            break;
        case ClickType_Weather:
            if (weather_click_enable() < 0) {
                LOG(LOG_ERROR, "Failed to enable weather click on bus#%d\n", busIndex);
                return -1;
            }
            break;

            //TODO: add rest if needed
        default:
            break;
    }

    return 0;
}

static int release_click(ClickType clickType, uint8_t busIndex) {
    i2c_select_bus(busIndex);
    switch (clickType) {
        case ClickType_Thermo3:
            if (thermo3_click_disable() < 0) {
                LOG(LOG_ERROR, "Failed to disable thermo3 click on bus#%d\n", busIndex);
                return -1;
            }
            break;
        case ClickType_Weather:
            if (weather_click_disable() < 0) {
                LOG(LOG_ERROR, "Failed to disable weather click on bus#%d\n", busIndex);
                return -1;
            }
            break;
        default:
            break;
    }

}

int main(int argc, char **argv) {
    options opts = {
        .click1 = ClickType_None,
        .click2 = ClickType_None,
        .sleepTime = DEFAULT_SLEEP_TIME,
        .address = DEFAULT_CLIENT_DAEMON_ADDRESS,
        .port = DEFAULT_CLIENT_DAEMON_PORT,
    };
    struct sigaction action = {
        .sa_handler = exitApp,
        .sa_flags = 0
    };

    if (loadConfiguration(argc, argv, &opts) == false)
        return -1;

    if (sigemptyset(&action.sa_mask) < 0
    ||  sigaction(SIGINT, &action, NULL) < 0) {
        LOG(LOG_ERROR, "Failed to set Control+C handler\n");
        return -1;
    }

    if (i2c_init() < 0) {
        LOG(LOG_ERROR, "Failed to initialize I2C.\n");
        return -1;
    }

    if (initialize_click(opts.click1, MIKROBUS_1) < 0
    ||  initialize_click(opts.click2, MIKROBUS_2) < 0) {
        i2c_release();
        return -1;
    }

    readSerialNumberFromEthernetMAC(opts.address, opts.port);

    while(_Running) {
        AwaClientSession* session = connectToAwa(opts.address, opts.port);
        if (session) {
            //contains last used instance ids for all registered sensors
            int instances[6] = {
                0,  //3303 (temperature)
                0,  //3304 (humidity)
                0,  //3315 (barometer)
                0,  //3325 (concentration)
                0,  //3330 (distance)
                0   //3328 (power)
            };
            struct measurement *measurements = NULL;
            performMeasurements(opts.click1, MIKROBUS_1, &measurements, instances);
            performMeasurements(opts.click2, MIKROBUS_2, &measurements, instances);
            sendMeasurements(session, measurements);
            releaseMeasurements(measurements);
            disconnectAwa(session);
        }
        sleep(opts.sleepTime);
    }

    release_click(opts.click1, MIKROBUS_1);
    release_click(opts.click2, MIKROBUS_2);
    i2c_release();

    return 0;
}
