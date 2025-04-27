#include "telemetry.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <linux/time.h>


long long get_current_time_ms() {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("clock_gettime");
        return -1;
    }
    return (long long)(ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);
}

float generate_temperature(virtual_source *source) {

    float change = ((float)rand() / RAND_MAX) * 2 * source->max_change - source->max_change;
    float new_value = source->current_value + change;
    if (new_value < source->min_value) {
        new_value = source->min_value;
    } else if (new_value > source->max_value) {
        new_value = source->max_value;
    }
    source->current_value = new_value;
    return new_value;
}

gps_data generate_gps(virtual_source *source) {
    double lat_change = ((rand() / (double)RAND_MAX) * 2.0 - 1.0) * source->max_gps_change;
    double lon_change = ((rand() / (double)RAND_MAX) * 2.0 - 1.0) * source->max_gps_change;
    gps_data new_gps = source->gps;
    new_gps.latitude += lat_change;
    new_gps.longitude += lon_change;
    if (new_gps.latitude > 90.0) new_gps.latitude = 90.0;
    if (new_gps.latitude < -90.0) new_gps.latitude = -90.0;
    if (new_gps.longitude > 180.0) new_gps.longitude -= 360.0;
    if (new_gps.longitude < -180.0) new_gps.longitude += 360.0;
    source->gps = new_gps;
    return new_gps;
}

const char *generate_status(virtual_source *source) {
    if (source->num_statuses <= 0 || source->statuses == NULL) {
        return "NO_STATUSES_CONFIGURED"; // Возвращаем строку-ошибку
    }
    int status_index = rand() % source->num_statuses;
    return source->statuses[status_index];
}

float generate_humidity(virtual_source *source) {
    float change = ((float)rand() / RAND_MAX) * 2 * source->max_change - source->max_change;
    float new_value = source->current_value + change;
    if (new_value < source->min_value) {
        new_value = source->min_value;
    } else if (new_value > source->max_value) {
        new_value = source->max_value;
    }
    source->current_value = new_value;
    return new_value;
}
float generate_pressure(virtual_source *source) {
    float change = ((float)rand() / RAND_MAX) * 2 * source->max_change - source->max_change;
    float new_value = source->current_value + change;
    if (new_value < source->min_value) {
        new_value = source->min_value;
    } else if (new_value > source->max_value) {
        new_value = source->max_value;
    }
    source->current_value = new_value;
    return new_value;
}

void update_source_reading(virtual_source *source) {
    if (!source || !source->is_active) {
       return;
   }
   source->data.id = source->id;
   source->data.type = source->type;
   source->data.timestamp_ms = get_current_time_ms();

   switch(source->type) {
       case DATA_TYPE_TEMPERATURE:
           source->data.value.temperature = generate_temperature(source);
           break;
       case DATA_TYPE_PRESSURE:
            source->data.value.pressure = generate_pressure(source);
           break;
       case DATA_TYPE_HUMIDITY:
            source->data.value.humidity = generate_humidity(source);
           break;
       case DATA_TYPE_GPS:
           source->data.value.gps = generate_gps(source);
           break;
       case DATA_TYPE_STATUS:
           strncpy(source->data.value.status,
                   generate_status(source),
                   sizeof(source->data.value.status) - 1);
           source->data.value.status[sizeof(source->data.value.status) - 1] = '\0';
           break;
       default:
            memset(&(source->data.value), 0, sizeof(telemetry_value));
           break;
   }
}