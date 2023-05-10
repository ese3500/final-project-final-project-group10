/*
 * Buhlman.h
 *
 * Created: 4/6/2023
 *  Author: erica
 */ 

#include <avr/io.h>
#include <stdbool.h>

void initializeCalculations();
int calculateDepthFromPressure(int pressure);
float calculatePPLung(float currentPressure);
float calculateCompartmentPPOtherGasses(int timeSec, float compthalfTimeSec, float currentComptPPOtherGasses, float currentLungPPOtherGasses);
float ascendToPPForCompt(int compartmentIdx, float comptPP);
int getNoDecoStopMinutes(float currentPressure);
int getNoDecoStopMinSingleCompartment(int comptIdx, float currentPressure);
int minutesToTargetPressure(int targetPressure);
float calculateAscentRate(int timeAtCurrentDepth, int previousDepth, int currentDepth);
void advanceCalculations(int oldDepth, int newPressure, int totalDiveTime);
