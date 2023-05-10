
#include <avr/io.h>
#include <stdbool.h>
#include <uart.h>
#include <stdio.h>

const float waterVaporPressureValue = 6000;//pa, because all calculations are done in Pa in this library
int diveTime = 0;
float ascentRate = 0;
//Default values
const float nitrogenRateInGas = 0.78;
const float seaLevelAtmosphericPressure = 1013.0;

//Coefficients of the ZH-L16A Buhlmann decomression algorithm - pulled from wikipedia
//https://en.wikipedia.org/wiki/B%C3%BChlmann_decompression_algorithm
float halfTimes[16] = {4.0, 8.0, 12.5, 18.5, 27.0, 38.3, 54.3, 77.0, 109.0, 146.0, 187.0, 239.0, 305.0, 390.0, 498.0, 635.0};
float aValues[16] = {1.2599, 1, 0.8618, 0.7562, 0.6667, 0.5933, 0.5282, 0.4701, 0.4187, 0.3798, 0.3497, 0.3223, 0.2971, 0.2737, 0.2523, 0.2327};
float bValues[16] = {0.505, 0.6514, 0.7222, 0.7825, 0.8125, 0.8434, 0.8693, 0.891, 0.9092, 0.9222, 0.9319, 0.9403, 0.9477, 0.9544, 0.9602, 0.9653};

//this stores the current partial pressure of N2 in each compartment
float compartmentPPsN[16];

//this calculates the partial pressure of nitrogen that is being breathed in
//this assumes that you are not diving with enriched air
float calculatePPLung(float currentPressure) {
	return (currentPressure - waterVaporPressureValue) * nitrogenRateInGas;
}

//sets up the calculations
void initializeCalculations() {
	diveTime = 0;
	//Initialize the compartments with initial nitrogen partial pressure value
	//this assumes that you haven't recently been diving
	//i.e. the compartments start at sea level atmospheric pressure
	for (int i = 0; i < 16; i++) {
		compartmentPPsN[i] = calculatePPLung(seaLevelAtmosphericPressure);
	}
}

//calculates the depth in feet from the pressure, in pascals
//used externally
int calculateDepthFromPressure(int pressure) {
	float depth = 0;//in ft
	//ignore if the depth will be negative; it is moot
	if (pressure > seaLevelAtmosphericPressure) {
		//uses constants: g and assumed density of salt water
		depth = (pressure - seaLevelAtmosphericPressure) / (9.80665 * 10.25);
		depth *= 3.281;//meters to ft
	}
	return depth;
}

//calculates the partial pressure of other gasses (not nitrogen) in a specific compartment
float calculateCompartmentPPOtherGasses(int timeSec, float compthalfTimeSec, float currentComptPPOtherGasses, float currentLungPPOtherGasses) {
	return currentComptPPOtherGasses + (currentLungPPOtherGasses - currentComptPPOtherGasses) * (1 - pow(2, (-timeSec/compthalfTimeSec)));
}

//this calculates the external pressure to which this specific compartment can ascend to without issue right now
//any lower pressure requires some decompression
float ascendToPPForCompt(int compartmentIdx, float comptPP) {
	return (comptPP * bValues[compartmentIdx])-(aValues[compartmentIdx] * bValues[compartmentIdx] * 1000);
}

//finds the maximum time that the diver can stay at this current pressure without having to take a decompression stop,
//but it only considers one compartment
int getNoDecoStopMinSingleCompartment(int comptIdx, float currentPressure) {
	int min = 0;
	bool decoStop = false;
	float comptPP;
	//this maxes it out at 100 minutes, because its really not significant after that and it increases speed of the calculation
	// also we're only displaying 2 digits
	while (!decoStop && min < 100) {
		min++;
		comptPP = calculateCompartmentPPOtherGasses(min * 60, halfTimes[comptIdx] * 60, compartmentPPsN[comptIdx], calculatePPLung(currentPressure));
		decoStop = ascendToPPForCompt(comptIdx, comptPP) > seaLevelAtmosphericPressure; 
	}
	return min;
}

//used externally
//finds the maximum time that the diver can stay at this current pressure without having to take a decompression stop
int getNoDecoStopMinutes(float currentPressure) {
	int maxDecoTime = 0;
	//finds the max time for any compartment
	for (int i = 0; i < 16; i++) {
		int t = getNoDecoStopMinSingleCompartment(i, currentPressure);
		if (maxDecoTime < t) {
			maxDecoTime = t;
		}
	}
	return maxDecoTime;
}

//calculates the time in minutes until a target pressure is reached
int minutesToTargetPressure(int targetPressure) {
	float ascentCeilings[16];
	int min = 0;
	bool sufficientTimeFound = 0;
	while (!sufficientTimeFound) {
		min++;//counts time until all of the compartments reach the target
		//this loop finds ands save the PP for each compartment, then finds the ascendTo PP for each compartment
		for (int comptIdx = 0; comptIdx < 16; comptIdx++) {
			ascentCeilings[comptIdx] = ascendToPPForCompt(comptIdx,
			calculateCompartmentPPOtherGasses(min * 60, halfTimes[comptIdx] * 60, compartmentPPsN[comptIdx], calculatePPLung(targetPressure)));
		}
		//All ascents have to be lower than the target pressure
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

//calculates the current ascent rate, only if ascending
float calculateAscentRate(int timeAtCurrentDepth, int previousDepth, int currentDepth) {
	if (previousDepth > currentDepth) {
		return (previousDepth - currentDepth) / timeAtCurrentDepth;
	}
	return 0;
}

//moves everything forward when a new pressure reading comes int
void advanceCalculations(int oldDepth, int newPressure, int totalDiveTime) { //call every time there is a change in pressure
	if (totalDiveTime - diveTime > 0) { //at least a second spent at that pressure, otherwise its a repeated measurement
		ascentRate = calculateAscentRate(totalDiveTime - diveTime, oldDepth, calculateDepthFromPressure(newPressure));
		diveTime = totalDiveTime;
		float ascentCeilings[16];
		bool decoRequired[16];
		int minToDecoRequired[16];
		for (int i = 0; i < 16; i++) { //updates PP for each compartment
			compartmentPPsN[i] = calculateCompartmentPPOtherGasses(totalDiveTime - diveTime, halfTimes[i] * 60, compartmentPPsN[i], calculatePPLung(newPressure));
			ascentCeilings[i] = ascendToPPForCompt(i, compartmentPPsN[i]);
			//sees if any of the compartments requires a decompression stop
			decoRequired[i] = ascentCeilings[i] > seaLevelAtmosphericPressure;
			if (decoRequired[i]) {
				minToDecoRequired[i] = 0;
			} else {
				minToDecoRequired[i] = getNoDecoStopMinSingleCompartment(i, newPressure);
			}
		}
		bool anyCompartmentNeedsDeco = 0;
		for (int i = 0; i < 16; i++) {
			if (decoRequired[i]) {
				anyCompartmentNeedsDeco = 1;
				break;
			}
		}
		if (!anyCompartmentNeedsDeco) {
			//finds smallest compartment time to deco
			int minToDeco = minToDecoRequired[0];
			for (int i = 1; i < 16; i++) {
				if (minToDecoRequired[i] < minToDeco) {
					minToDeco = minToDecoRequired[i];
				}
			}
		}
	}
}
