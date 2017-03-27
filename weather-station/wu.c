#include <glib.h>
#include <curl.h>
#include "wu.h"
#include "types.h"

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
    int size =
            g_snprintf(NULL,0,"https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?ID=%s&PASSWORD=%s&dateutc=now&action=updateraw&realtime=1&rtfreq=%d",
                    wu_id,
                    wu_password,
                    sleep);
                    wu_base_url = g_malloc0(size+1);
    g_snprintf(wu_base_url, size+1,
               "https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?ID=%s&PASSWORD=%s&dateutc=now&action=updateraw&realtime=1&rtfreq=%d",
                wu_id,
                wu_password,
                sleep
                );
}

void wu_send_measurements(struct measurement *measurements) {
    char *url;
    char *tmpUrl = NULL;
    tmpUrl = g_strdup(wu_base_url);
    struct measurement *ptr = measurements;
    char *param;
    int size;
    while (ptr) {
        switch (ptr->type) {
            case MeasurementType_Temperature:
                size = g_snprintf(NULL, 0, "&tempf=%.2f", ptr->value);
                param = g_malloc0(size+1);
                g_snprintf(param, size+1, "&tempf=%.2f", ptr->value*9/5+32); // convert from celsius to fahrenheit
                g_free(url);
                url = g_strconcat(tmpUrl, param, NULL);
                g_free(tmpUrl);
                tmpUrl = strdup(url);
                g_free(param);
                break;
            case MeasurementType_Humidity:
                size = g_snprintf(NULL, 0, "&humidity=%.2f", ptr->value);
                param = g_malloc0(size+1);
                g_snprintf(param, size+1, "&humidity=%.2f", ptr->value);
                g_free(url);
                url = g_strconcat(tmpUrl, param, NULL);
                g_free(tmpUrl);
                tmpUrl = strdup(url);
                g_free(param);
                break;
            case MeasurementType_Pressure:
                size = g_snprintf(NULL, 0, "&baromin=%.2f", ptr->value*0.02953); // convert from hPa to inches of mercury
                param = g_malloc0(size+1);
                g_snprintf(param, size+1, "&baromin=%.2f", ptr->value*0.02953);
                g_free(url);
                url = g_strconcat(tmpUrl, param, NULL);
                g_free(tmpUrl);
                tmpUrl = strdup(url);
                g_free(param);
                break;
            case MeasurementType_COConcentration:
                size = g_snprintf(NULL, 0, "&AqCO=%.2f", ptr->value); // convert from hPa to inches of mercury
                param = g_malloc0(size+1);
                g_snprintf(param, size+1, "&AqCO=%.2f", ptr->value);
                g_free(url);
                url = g_strconcat(tmpUrl, param, NULL);
                g_free(tmpUrl);
                tmpUrl = strdup(url);
                g_free(param);
                break;
            default:
                break;
        }
        ptr = ptr->next;
    }
    curl_send(url);
    g_free(url);
    g_free(tmpUrl);


}

void wu_release() {
    g_free(wu_id);
    g_free(wu_password);
    g_free(wu_base_url);
}


