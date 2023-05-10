
#include <avr/io.h>
#include <stdbool.h>
#include <uart.h>
#include <stdio.h>

float waterVaporPressureValue = 6000;//pa?
int diveTime = 0;
float ascentRate = 0;
//Default values
float nitrogenRateInGas = 0.78;
float seaLevelAtmosphericPressure = 1013.25;

//Coefficients of the ZH-L16A Buhlmann decomression algorithm - pulled from wikipedia
//https://en.wikipedia.org/wiki/B%C3%BChlmann_decompression_algorithm
float halfTimes[16] = {4.0, 8.0, 12.5, 18.5, 27.0, 38.3, 54.3, 77.0, 109.0, 146.0, 187.0, 239.0, 305.0, 390.0, 498.0, 635.0};
float aValues[16] = {1.2599, 1, 0.8618, 0.7562, 0.6667, 0.5933, 0.5282, 0.4701, 0.4187, 0.3798, 0.3497, 0.3223, 0.2971, 0.2737, 0.2523, 0.2327};
float bValues[16] = {0.505, 0.6514, 0.7222, 0.7825, 0.8125, 0.8434, 0.8693, 0.891, 0.9092, 0.9222, 0.9319, 0.9403, 0.9477, 0.9544, 0.9602, 0.9653};

float compartmentPPsN[16];

float calculatePPLung(float currentPressure) { //calculates the partial pressure of nitrogen in the lung
	return (currentPressure - waterVaporPressureValue) * nitrogenRateInGas;
}

void initializeCalculations() {
	diveTime = 0;
	
	//Initialize the compts with initial pp value
	for (int i = 0; i < 16; i++) {
		compartmentPPsN[i] = calculatePPLung(seaLevelAtmosphericPressure);
	}

}
int calculateDepthFromPressure(int pressure) {
	float depth = 0;//in ft

	if (pressure > seaLevelAtmosphericPressure) {
		depth = (pressure - seaLevelAtmosphericPressure) / (9.80665 * 10.25);
		depth *= 3.281;//meters to ft
	}
	return depth;
}
int calculateHydrostaticPressureFromDepth(int depth) {
	return seaLevelAtmosphericPressure + (9.80665 * 10.25 * depth / 3.281 * 100);
}
void setPPOfCompartment(int compartmentIdx, float pressure) {
	compartmentPPsN[compartmentIdx] = pressure;
}
float calculateCompartmentPPOtherGasses(int timeSec, float compthalfTimeSec, float currentComptPPOtherGasses, float currentLungPPOtherGasses) {
	return currentComptPPOtherGasses + (currentLungPPOtherGasses - currentComptPPOtherGasses) * (1 - pow(2, (-timeSec/compthalfTimeSec)));
}
float ascendToPPForCompt(int compartmentIdx, float comptPP) {
	return (comptPP * bValues[compartmentIdx]) - (1000 * aValues[compartmentIdx] * bValues[compartmentIdx]);
}
bool getDecoStopNeeded(float ascendToPP) {
	return (ascendToPP > seaLevelAtmosphericPressure);
}
int getNoDecoStopMinutesIdx(int comptIdx, float currentPressure) {
	int min = 0;
	bool decoStopNeeded = false;
	float comptPP;
	while (!decoStopNeeded && min < 99) {
		min++;
		comptPP = calculateCompartmentPPOtherGasses(min * 60, halfTimes[comptIdx] * 60, compartmentPPsN[comptIdx], calculatePPLung(currentPressure));
		decoStopNeeded = getDecoStopNeeded(ascendToPPForCompt(comptIdx, comptPP));
	}
	return min;
}
int getNoDecoStopMinutes(float currentPressure) {
	int maxDecoTime = 0;
	for (int i = 0; i < 16; i++) {
		int i_time = getNoDecoStopMinutesIdx(i, currentPressure);
		if (maxDecoTime < i_time) {
			maxDecoTime = i_time;
		}
	}
	return maxDecoTime;
}
int minutesToTargetPressure(int targetPressure) {
	float ascentCeilings[16];
	int min = 0;
	bool sufficientTimeFound = 0;

	while (!sufficientTimeFound) {
		min++;

		//this loop finds ands save the PP for each compartment, then finds the ascendTo PP for each compartment
		for (int comptIdx = 0; comptIdx < 16; comptIdx++) {
			ascentCeilings[comptIdx] = ascendToPPForCompt(comptIdx, 
				calculateCompartmentPPOtherGasses(min * 60, halfTimes[comptIdx] * 60, compartmentPPsN[comptIdx], calculatePPLung(targetPressure)));
		}

		//All max ascents have to be lower than the target pressure
		float maximum = 0;
		bool foundGreater = 0;
		for (int i = 0; i < 16; i++) {
			if (ascentCeilings[i] >= targetPressure) {
				foundGreater = 1;
			}
		}
		sufficientTimeFound = foundGreater;
	}
	return min;
}
float calculateAscentRate(int timeAtCurrentDepth, int previousDepth, int currentDepth) {
	if (previousDepth > currentDepth) {
		return (previousDepth - currentDepth) / timeAtCurrentDepth;
	}
	return 0;
}
void advanceCalculations(int oldDepth, int newPressure, int totalDiveTime) { //call every time there is a change in pressure

	int timeSpentPrevPressure = totalDiveTime - diveTime; //seconds
	if (timeSpentPrevPressure > 0) { //at least a second spent at that pressure
		ascentRate = calculateAscentRate(timeSpentPrevPressure, oldDepth, calculateDepthFromPressure(newPressure));
		diveTime = totalDiveTime;

		bool decoNeeded[16];
		int minToDecoNeeded[16];
		float ascentCeilings[16];

		for (int i = 0; i < 16; i++) { //finds PP for each compartment
			compartmentPPsN[i] = calculateCompartmentPPOtherGasses(timeSpentPrevPressure, halfTimes[i] * 60, compartmentPPsN[i], calculatePPLung(newPressure));

			//Calculate the ascendTo partial pressure for the compartment and store it in the ascent ceiling array
			ascentCeilings[i] = ascendToPPForCompt(i, compartmentPPsN[i]);

			decoNeeded[i] = getDecoStopNeeded(ascentCeilings[i]);
			if (!decoNeeded[i]) {
				//Calculate minutes can be spent in the current depth
				minToDecoNeeded[i] = getNoDecoStopMinutesIdx(i, newPressure);
			} else {
				minToDecoNeeded[i] = 0;
				//Calculate first deco stop
				float firstDecoStop = calculateDepthFromPressure(ascentCeilings[i]);
			}
		}
		//Calculate if DECO is needed for any of the compartments
		bool decoNeededOverall = 0;
		for (int i = 0; i < 16; i++) {
			if (decoNeeded[i]) {
				decoNeededOverall = 1;
				break;
			}
		}
		if (!decoNeededOverall) {
			//finds smallest compartment time to deco
			int minToDeco = minToDecoNeeded[0];
			for (int i = 1; i < 16; i++) {
				if (minToDecoNeeded[i] < minToDeco) {
					minToDeco = minToDecoNeeded[i];
				}
			}
		} else {
			//TODO: throw some error or alert because we're assuming a different dive profile
			OCR0A = 142;
		}
	}
}
