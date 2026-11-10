#pragma once
#include "ward.h"
#include "swa_packet.h"

void displayBegin();
// beds[] is indexed 1..SWA_MAX_BEDS
void displayDraw(const BedInfo *beds, int activeNodes);
