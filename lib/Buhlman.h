/*
 * Buhlman.h
 *
 * Created: 4/6/2023
 *  Author: erica
 */ 

#include <avr/io.h>
#include <stdbool.h>

void initializeCalculations(float waterVaporPressureCorrection_, int currentDepth);
void updateParameters(int currentDepth, int newMaxDepth, int newDiveTime);
int calculateDepthFromPressure(int pressure);
int calcaulteHydrostaticPressureFromDepth(int depth_);
void setPPOfCompartment(int compartmentIdx, float pressure);
float calculatePPLung(float currentPressure);
float calculateCompartmentPPOtherGasses(int timeSec, float compthalfTimeSec, float currentComptPPOtherGasses, float currentLungPPOtherGasses);
float ascendToPPForCompt(int compartmentIdx, float comptPP);
int getNoDecoStopMinutes(int comptIdx, float currentPressure);
bool getDecoStopNeeded(float ascendToPP);
int minutesToTargetPressure(int targetPressure);
float calculateAscentRate(int timeAtCurrentDepth, int previousDepth, int currentDepth);
void advance(float newPressure, int totalDiveTime);

#endif 