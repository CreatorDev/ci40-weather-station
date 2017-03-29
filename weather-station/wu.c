/************************************************************************************************************************
 Copyright (c) 2017, Imagination Technologies Limited and/or its affiliated group companies.
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

#include <glib.h>
#include <curl.h>
#include "wu.h"
#include "types.h"

#define WU_BASE_URL_FORMAT "https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?ID=%s&PASSWORD=%s&dateutc=now&action=updateraw&realtime=1&rtfreq=%d"

static char *wu_id;
static char *wu_password;
static char *wu_base_url;
static unsigned int timeout;

static void curl_send(char *url) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        g_message("Successfully sent data to weather undeground");
    } else {
        g_warning("Failed to send data to weather undeground");
    }
    curl_easy_cleanup(curl);
}

void wu_init(char *id, char *password, unsigned int sleep) {
    wu_id = g_strdup(id);
    wu_password = g_strdup(password);
    timeout = sleep;
    int size = g_snprintf(NULL,0, WU_BASE_URL_FORMAT,wu_id,wu_password,sleep);
    wu_base_url = g_malloc0(size+1);
    g_snprintf(wu_base_url, size+1, WU_BASE_URL_FORMAT, wu_id, wu_password, sleep);
}

void wu_send_measurements(struct measurement *measurements) {

    char *url = g_strdup(wu_base_url);
    char *tmp = NULL;
    struct measurement *ptr = measurements;
    char *param = NULL;
    int size;
    while (ptr) {
        switch (ptr->type) {
            case MeasurementType_Temperature:
                size = g_snprintf(NULL, 0, "&tempf=%.2f", ptr->value);
                param = g_malloc0(size+1);
                tmp = url;
                g_snprintf(param, size+1, "&tempf=%.2f", ptr->value*9/5+32); // convert from celsius to fahrenheit
                url = g_strconcat(url, param, NULL);
                g_free(tmp);
                g_free(param);
                break;
            case MeasurementType_Humidity:
                size = g_snprintf(NULL, 0, "&humidity=%.2f", ptr->value);
                param = g_malloc0(size+1);
                tmp = url;
                g_snprintf(param, size+1, "&humidity=%.2f", ptr->value);
                url = g_strconcat(url, param, NULL);
                g_free(tmp);
                g_free(param);
                break;
            case MeasurementType_Pressure:
                size = g_snprintf(NULL, 0, "&baromin=%.2f", ptr->value*0.02953f); // convert from hPa to inches of mercury
                param = g_malloc0(size+1);
                tmp = url;
                g_snprintf(param, size+1, "&baromin=%.2f", ptr->value*0.02953f);
                url = g_strconcat(url, param, NULL);
                g_free(tmp);
                g_free(param);
                break;
            case MeasurementType_COConcentration:
                size = g_snprintf(NULL, 0, "&AqCO=%.2f", ptr->value);
                param = g_malloc0(size+1);
                tmp = url;
                g_snprintf(param, size+1, "&AqCO=%.2f", ptr->value);
                url = g_strconcat(url, param, NULL);
                g_free(tmp);
                g_free(param);
                break;
            default:
                break;
        }
        ptr = ptr->next;
    }
    curl_send(url);
    g_free(url);
}

void wu_release() {
    g_free(wu_id);
    g_free(wu_password);
    g_free(wu_base_url);
}


