#ifndef CONF_H
#define CONF_H

#include "telemetry.h" 
#include <stddef.h>    

typedef struct {
    virtual_source *sources;
    size_t count;          
} source_config;

source_config load_sources_config();
void free_sources_config(source_config config);

#endif // CONF_H