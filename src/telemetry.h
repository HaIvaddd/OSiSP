#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <time.h>
#include <stdbool.h>

typedef enum {
    DATA_TYPE_TEMPERATURE,
    DATA_TYPE_PRESSURE,
    DATA_TYPE_HUMIDITY,
    DATA_TYPE_GPS,
    DATA_TYPE_STATUS
} telemetry_data_type;

typedef struct {
    double latitude;
    double longitude;
} gps_data;

typedef union {
    float temperature;
    float pressure;
    float humidity;
    gps_data gps;
    char status[20];
} telemetry_value;


typedef struct {
    int id;
    telemetry_data_type type;
    long long timestamp_ms;
    telemetry_value value;
} telemetry_data;

typedef struct virtual_source {
    int id;
    telemetry_data_type type;
    bool is_active;
    float current_value;
    float min_value;
    float max_value;
    float max_change;
    int update_interval_ms;
    gps_data gps;
    double max_gps_change;
    char *statuses[5];
    int num_statuses;
    telemetry_data data;
} virtual_source;


long long get_current_time_ms();
float generate_temperature(virtual_source *source);
float generate_pressure(virtual_source *source);
float generate_humidity(virtual_source *source);
gps_data generate_gps(virtual_source *source);
const char *generate_status(virtual_source *source);
void update_source_reading(virtual_source *source);

#endif // TELEMETRY_H