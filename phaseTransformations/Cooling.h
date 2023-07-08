#pragma once
#include <iostream>
#include <array>
#include <map>
#include <cmath>
#include <random>
#include <ctime>
#include <omp.h>
#include "TemperatureLoader.h"
#include "HistogramRhoAndD.h"
#include "HeatTreatmentResult.h"
#include "steelPhase.cpp"

//auxiliary struct representing volume fractions of single phase
struct phaseVolumeFractions
{
	double F;		//volume fraction with respect to the whole volume
	double X;		//volume fraction with respect to the maximum volume fraction at the temperature T
};
//operator overloader for displaying data in phaseVolumeFractions
std::ostream& operator<<(std::ostream& OS, const phaseVolumeFractions& pvf);

//auxiliary struct with coefficients of steel
struct steelCoefficients
{
	const double c0;
	const double cAlpha;
	const double cGammaAlpha0;
	const double cGammaAlpha1;
	const double cGammaBeta0;
	const double cGammaBeta1;
	const double Ae1;
	const double Ae3;
	const double ceut;
	const double Feut;
	const double Xeut;

	//constructor ; takes only 6 values, because Ae1, Ae3, ceut, Feut and Xeut are computed from the previous ones
	steelCoefficients(double _c0, double _cAlpha, double _cGammaAlpha0, double _cGammaAlpha1,
		double _cGammaBeta0, double _cGammaBeta1);
};

class Cooling
{
	std::vector<std::mt19937> randomGenerators;		//currently uses Mersenne Twister Algorithm - can be replaced by any other C++ random generator
	void initRandomGenerators();

	//pointer at object with temperature values
	TemperatureLoader* temperatures;
	//pointer at histogram with points data
	HistogramRhoAndD* histogram;
	//array with coefficients a1, a2, ..., a31, a32
	std::array<double, 32>* coefficients;
	//array with coefficients b1, b2, b3, b4
	std::array<double, 4>* ferriteGrainSizeCoefficients;
	//struct with coefficients of steel (c0, cAlpha, cGammaAlpha0, cGammaAlpha1, 
	//cGammaBeta0, cGammaBeta1, Ae1, Ae3, ceut, Feut, Xeut)
	steelCoefficients* steelCoeff;
	//map with values of volume fractions
	std::map<steelPhase, phaseVolumeFractions> volumeFractions{ {steelPhase::FERRITE, {0.,0.}}, {steelPhase::PEARLITE, {0.,0.}}, 
		{steelPhase::BAINITE, {0.,0.}}, {steelPhase::MARTENSITE, {0.,0.}} };
	//current average carbon content in the austenite
	double cGamma = 0.;
	//v for computing ferrite grain size
	double sumNeededForComputingFerriteGrainSize = 0.;
	int noTimestepsInFerriteTransformation = 0;

	//private transformation methods used in stochasticCool method
	bool ferriticTransformation(double D, double rho, double temperature, double dt, double cGammaAlpha, bool didFerriticTransformationStart);
	bool pearliticTransformation(double D, double rho, double temperature, double dt, bool didPearliticTransformationStart);
	bool bainiticTransformation(double D, double rho, double temperature, double dt, bool didBainiticTransformationStart);
	void martensiticTransformation();

	bool didPhaseTransformationsEnd();
	inline bool didPhaseTransformationsEnd(double ferrite_F, double pearlite_F, double bainite_F, double martensite_F);

public:
	static const double WHOLE_VOLUME_FULFILLMENT_PRECISION;

	Cooling(TemperatureLoader* t, HistogramRhoAndD* h, std::array<double, 32>* coeff, std::array<double, 4>* ferriteGrainSizeCoeff, steelCoefficients* sCoeff);
	HeatTreatmentResult* stochasticCoolSingleThreaded();
	HeatTreatmentResult* stochasticCool();
	HeatTreatmentResult* stochasticCool_ALT();

	//methods used for optimization ; probably to be deleted later
	//can be refactored, but reasonable refactor causes great decrease in software performance, so it haven't been applied
	//used for optimization coefficients related to ferrite temperature start ONLY
	HeatTreatmentResult* stochasticCoolOptimization_FERRITE_START_singleThreaded();
	HeatTreatmentResult* stochasticCoolOptimization_FERRITE_START();
	//used for optimization coefficients related to ferrite volume fraction and pearlite temperature start ONLY
	HeatTreatmentResult* stochasticCoolOptimization_FERRITE_FRACTION_and_PEARLITE_START_singleThreaded();
	HeatTreatmentResult* stochasticCoolOptimization_FERRITE_FRACTION_and_PEARLITE_START();
	//used for optimization coefficients related to pearlite volume fraction and bainite temperature start and pearlite temperature end ONLY
	HeatTreatmentResult* stochasticCoolOptimization_PEARLITE_FRACTION_and_BAINITE_START_and_PEARLITE_END_singleThreaded();
	HeatTreatmentResult* stochasticCoolOptimization_PEARLITE_FRACTION_and_BAINITE_START_and_PEARLITE_END();
};