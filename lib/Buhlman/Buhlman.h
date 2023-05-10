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
int calculateHydrostaticPressureFromDepth(int depth);
void setPPOfCompartment(int compartmentIdx, float pressure);
float calculatePPLung(float currentPressure);
float calculateCompartmentPPOtherGasses(int timeSec, float compthalfTimeSec, float currentComptPPOtherGasses, float currentLungPPOtherGasses);
float ascendToPPForCompt(int compartmentIdx, float comptPP);
int getNoDecoStopMinutes(float currentPressure);
int getNoDecoStopMinutesIdx(int comptIdx, float currentPressure);
bool getDecoStopNeeded(float ascendToPP);
int minutesToTargetPressure(int targetPressure);
float calculateAscentRate(int timeAtCurrentDepth, int previousDepth, int currentDepth);
void advanceCalculations(int oldDepth, int newPressure, int totalDiveTime);
