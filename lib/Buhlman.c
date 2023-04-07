
#include <avr/io.h>
#include <stdbool.h>

float waterVaporPressureValue = 0;
int diveTime = 0;
int depth = 0;
int maxDepth = 0;
float ascentRate = 0;
//Default values
float nitrogenRateInGas = 0.78;
float seaLevelAtmosphericPressure = 1013.25;

//Coefficients of the ZH-L16C-GF algorithm
float halfTimes[16];
float aValues[16];
float bValues[16];
float compartmentPPsN[16];

void initializeCalculations(float waterVaporPressureCorrection_, int currentDepth) {
	waterVaporPressureValue = waterVaporPressureCorrection_;
	diveTime = 0;
	depth = currentDepth;
	maxDepth = depth;
	
	halfTimes = {4.0, 8.0, 12.5, 18.5, 27.0, 38.3, 54.3, 77.0, 109.0, 146.0, 187.0, 239.0, 305.0, 390.0, 498.0, 635.0};
	aValues = {1.2599, 1.0000, 0.8618, 0.7562, 0.6200, 0.5043, 0.4410, 0.4000, 0.3750, 0.3500, 0.3295, 0.3065, 0.2835, 0.2610, 0.2480, 0.2327};
	bValues = {0.5050, 0.6514, 0.7222, 0.7825, 0.8126, 0.8434, 0.8693, 0.8910, 0.9092, 0.9222, 0.9319, 0.9403, 0.9477, 0.9544, 0.9602, 0.9653};

	//Initialize the compts with initial pp value
	for (int i = 0; i < 16; i++) {
		compartmentPPsN[i] = calculatePPLung(seaLevelAtmosphericPressure);
	}

}
void updateParameters(int currentDepth, int newMaxDepth, int newDiveTime) {
	diveTime = newDiveTime;
	depth = currentDepth;
	maxDepth = newMaxDepth;
}
int calculateDepthFromPressure(int pressure) {
	int depthFt = 0;
	if (pressure > seaLevelAtmosphericPressure) {
		depthFt = (pressure - seaLevelAtmosphericPressure) / (9.80665 * 10.25) * 3.281;
	}
	return depthFt;
}
int calcaulteHydrostaticPressureFromDepth(int depth_) {
	return seaLevelAtmosphericPressure + (9.80665 * 10.25 * depth_ / 3.281 * 100);
}
void setPPOfCompartment(int compartmentIdx, float pressure) {
	compartmentPPsN[compartmentIdx] = pressure;
}
float calculatePPLung(float currentPressure) { //calculates the partial pressure of nitrogen in the lung
	return (currentPressure - waterVaporPressureValue) * nitrogenRateInGas;
}
float calculateCompartmentPPOtherGasses(int timeSec, float compthalfTimeSec, float currentComptPPOtherGasses, float currentLungPPOtherGasses) {
	return currentComptPPOtherGasses + (currentLungPPOtherGasses - currentComptPPOtherGasses) * (1 - pow(2, (-timeSec/compthalfTimeSec)));
}
float ascendToPPForCompt(int compartmentIdx, float comptPP) {
	return (comptPP * bValues[compartmentIdx]) - (1000 * aValues[compartmentIdx] * bValues[compartmentIdx]);
}
int getNoDecoStopMinutes(int comptIdx, float currentPressure) {
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
bool getDecoStopNeeded(float ascendToPP) {
	return ascendToPP > seaLevelAtmosphericPressure;
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
void advance(float newPressure, int totalDiveTime) { //call every time there is a change in pressure

	int timeSpentPrevPressure = totalDiveTime - diveTime; //seconds
	if (timeSpentPrevPressure > 0) { //at least a second spent at that pressure
		ascentRate = calculateAscentRate(timeSpentPrevPressure, depth, calculateDepthFromPressure(newPressure));
		diveTime = totalDiveTime;
		depth = calculateDepthFromPressure(newPressure);
		if (maxDepth < depth) {
			maxDepth = depth;
		}

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
				minToDecoNeeded[i] = minToDecoNeeded(i, newPressure);
			} else {
				minToDecoNeeded[i] = 0;
				//Calculate first deco stop
				float firstDecoStop = calculateDepthFromPressure(ascentCeilings[i]);
			}
		}
		//Calculate if DECO is needed for any of the compartments
		bool decoNeeded = false;
		for (int i = 0; i < 16; i++) {
			if (decoNeeded[i]) {
				decoNeeded = true;
				break;
			}
		}
		if (!decoNeeded) {
			//finds smallest compartment time to deco
			int minToDeco = minToDecoNeeded[0];
			for (int i = 1; i < 16; i++) {
				if (minToDecoNeeded[i] < minToDeco) {
					minToDeco = minToDecoNeeded[i];
				}
			}
		} else {
			//TODO: throw some error or alert because we're assuming a different dive profile
		}
	}
}
