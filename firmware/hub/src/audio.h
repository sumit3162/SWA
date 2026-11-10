#pragma once
#include <stdint.h>

void audioBegin();
// queued, returns right away. plays "bed N" then the warning / critical phrase
void audioAnnounce(uint8_t bed, uint8_t level);
