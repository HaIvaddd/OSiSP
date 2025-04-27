#include "conf.h"
#include "telemetry.h"

#include <stdlib.h> 
#include <stdio.h>  
#include <string.h> 


static void init_temp_sensor(virtual_source *s, int id, int interval) {
    s->id = id;
    s->type = DATA_TYPE_TEMPERATURE;
    s->is_active = true;
    s->current_value = 20.0f + (rand() % 10) - 5;
    s->min_value = -10.0f;
    s->max_value = 40.0f;
    s->max_change = 0.5f;
    s->update_interval_ms = interval;
    memset(&s->data, 0, sizeof(s->data));
}

static void init_gps_sensor(virtual_source *s, int id, int interval) {
    s->id = id;
    s->type = DATA_TYPE_GPS;
    s->is_active = true;
    s->gps.latitude = 55.75 + ((rand() / (double)RAND_MAX) * 0.1 - 0.05);
    s->gps.longitude = 37.61 + ((rand() / (double)RAND_MAX) * 0.1 - 0.05);
    s->max_gps_change = 0.001;
    s->update_interval_ms = interval;
     memset(&s->data, 0, sizeof(s->data));
}

 static void init_status_sensor(virtual_source *s, int id, int interval) {
    s->id = id;
    s->type = DATA_TYPE_STATUS;
    s->is_active = true;
    static const char *default_statuses[] = {"OK", "WARNING", "ERROR", "IDLE", "BUSY"};
    size_t num_default = sizeof(default_statuses) / sizeof(default_statuses[0]);
    size_t capacity_s_statuses = sizeof(s->statuses) / sizeof(s->statuses[0]);
    size_t count_to_copy = (num_default < capacity_s_statuses) ? num_default : capacity_s_statuses;
    for (size_t i = 0; i < count_to_copy; i++) {
        s->statuses[i] = default_statuses[i];
    }
    for (size_t i = count_to_copy; i < capacity_s_statuses; i++) {
        s->statuses[i] = NULL; // Заполняем оставшиеся элементы NULL
    }
    s->num_statuses = count_to_copy;
    s->update_interval_ms = interval;
     memset(&s->data, 0, sizeof(s->data));
}

static void init_pressure_sensor(virtual_source *s, int id, int interval) {
    s->id = id;
    s->type = DATA_TYPE_PRESSURE;
    s->is_active = true;
    s->current_value = 1013.25f + (rand() % 10) - 5;
    s->min_value = 950.0f;
    s->max_value = 1100.0f;
    s->max_change = 0.5f;
    s->update_interval_ms = interval;
     memset(&s->data, 0, sizeof(s->data));
}

static void init_humidity_sensor(virtual_source *s, int id, int interval) {
    s->id = id;
    s->type = DATA_TYPE_HUMIDITY;
    s->is_active = true;
    s->current_value = 50.0f + (rand() % 10) - 5;
    s->min_value = 0.0f;
    s->max_value = 100.0f;
    s->max_change = 1.0f;
    s->update_interval_ms = interval;
     memset(&s->data, 0, sizeof(s->data));
}

source_config load_sources_config() {
    size_t num_sources_to_create = 6; 
    printf("Load conf");

    virtual_source *sources_array = malloc(num_sources_to_create * sizeof(virtual_source));
    if (sources_array == NULL) {
        perror("malloc failed for sources array in conf.c");
        return (source_config){NULL, 0}; 
    }

    init_temp_sensor(&sources_array[0], 101, 2000);
    init_gps_sensor(&sources_array[1], 205, 1000);
    init_status_sensor(&sources_array[2], 333, 5000);
    init_temp_sensor(&sources_array[3], 102, 3000);
    init_pressure_sensor(&sources_array[4], 401, 1500);
    init_humidity_sensor(&sources_array[5], 501, 2500);

    printf("Sensore create from conf\n");
    return (source_config){sources_array, num_sources_to_create};
}

void free_sources_config(source_config config) {
    if (config.sources != NULL) {
        printf("Clean mem\n");
        for (size_t i = 0; i < config.count; i++) {
            free(config.sources[i].statuses);
        }
        free(config.sources);
    }
}