#pragma once 
#include "poker.h"

void start_jpeg_filter_task(void); 

void send_jpeg_process_job(
    const char *in,
    const char *out,
    poker_filter_t filter);