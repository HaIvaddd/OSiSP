#include "telemetry.h"
#include "endian_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <arpa/inet.h>


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
    if (source->num_statuses <= 0) {
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

ssize_t serialize_telemetry_data(const telemetry_data *data, unsigned char *buffer, size_t buffer_size) {
    if (data == NULL || buffer == NULL) {
        fprintf(stderr, "serialize_telemetry: NULL pointer passed.\n");
        return -1;
    }

    size_t required_size = 0;
    size_t offset = 0; 

    required_size = 1 + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint64_t);

    switch (data->type) {
        case DATA_TYPE_TEMPERATURE:
        case DATA_TYPE_PRESSURE:
        case DATA_TYPE_HUMIDITY:
            required_size += sizeof(uint32_t);
            break;
        case DATA_TYPE_GPS:
            required_size += 2 * sizeof(uint64_t); 
            break;
        case DATA_TYPE_STATUS:
            required_size += sizeof(data->value.status); 
            break;
        default:
            fprintf(stderr, "serialize_telemetry: Unknown data type %d for source %d\n",
                    data->type, data->id);
            return -1;
    }

    if (buffer_size < required_size) {
         fprintf(stderr, "serialize_telemetry: Buffer too small. Required: %zu, Available: %zu\n",
                 required_size, buffer_size);
        return -1;
    }

    buffer[offset] = 'T';
    offset += sizeof(char);

    uint32_t net_id = htonl((uint32_t)data->id);
    memcpy(buffer + offset, &net_id, sizeof(net_id));
    offset += sizeof(net_id);

    uint8_t type_byte = (uint8_t)data->type;
    buffer[offset] = type_byte;
    offset += sizeof(type_byte);

    uint64_t net_timestamp = htonll((uint64_t)data->timestamp_ms);
    memcpy(buffer + offset, &net_timestamp, sizeof(net_timestamp));
    offset += sizeof(net_timestamp);

    switch (data->type) {
        case DATA_TYPE_TEMPERATURE: {
            uint32_t net_float = htonf(data->value.temperature);
            memcpy(buffer + offset, &net_float, sizeof(net_float));
            offset += sizeof(net_float);
            break;
        }
        case DATA_TYPE_PRESSURE: {
             uint32_t net_float = htonf(data->value.pressure);
             memcpy(buffer + offset, &net_float, sizeof(net_float));
             offset += sizeof(net_float);
             break;
        }
        case DATA_TYPE_HUMIDITY: {
             uint32_t net_float = htonf(data->value.humidity);
             memcpy(buffer + offset, &net_float, sizeof(net_float));
             offset += sizeof(net_float);
             break;
        }
        case DATA_TYPE_GPS: {
            uint64_t net_lat = htond(data->value.gps.latitude);
            uint64_t net_lon = htond(data->value.gps.longitude);
            memcpy(buffer + offset, &net_lat, sizeof(net_lat));
            offset += sizeof(net_lat);
            memcpy(buffer + offset, &net_lon, sizeof(net_lon));
            offset += sizeof(net_lon);
            break;
        }
        case DATA_TYPE_STATUS: {
            memcpy(buffer + offset, data->value.status, sizeof(data->value.status));
            offset += sizeof(data->value.status);
            break;
        }
    }

    if (offset != required_size) {
         fprintf(stderr, "serialize_telemetry: Internal error - offset (%zu) != required_size (%zu)\n",
                 offset, required_size);
         return -1;
    }

    return (ssize_t)offset;
}