#include "Cooling.h"

std::ostream& operator<<(std::ostream& OS, const phaseVolumeFractions& pvf)
{
	OS << "F = " << pvf.F << " , X = " << pvf.X;
	return OS;
}

steelCoefficients::steelCoefficients(double _c0, double _cAlpha, double _cGammaAlpha0, double _cGammaAlpha1,
	double _cGammaBeta0, double _cGammaBeta1) : c0(_c0), cAlpha(_cAlpha), cGammaAlpha0(_cGammaAlpha0),
	cGammaAlpha1(_cGammaAlpha1), cGammaBeta0(_cGammaBeta0), cGammaBeta1(_cGammaBeta1),
	Ae1((_cGammaAlpha0 - _cGammaBeta0) / (_cGammaBeta1 - _cGammaAlpha1)), Ae3((_c0 - _cGammaAlpha0) / _cGammaAlpha1),
	ceut(_cGammaAlpha0 + (_cGammaAlpha1 * Ae1)), Feut((ceut - _c0)/(ceut - _cAlpha)), Xeut(1.) {}

void Cooling::initRandomGenerators()
{
	int somePrimeNumber = 7723;
	randomGenerators.resize(omp_get_max_threads());
	for (int i = 0; i < omp_get_max_threads(); i++) {
		randomGenerators[i] = std::mt19937(((i + 1) * time(0)) + somePrimeNumber);
	}
}

Cooling::Cooling(TemperatureLoader* t, HistogramRhoAndD* h, std::array<double, 32>* coeff, std::array<double, 4>* ferriteGrainSizeCoeff, steelCoefficients* sCoeff)
{
	temperatures = t;
	histogram = h;
	coefficients = coeff;
	ferriteGrainSizeCoefficients = ferriteGrainSizeCoeff;
	steelCoeff = sCoeff;
	initRandomGenerators();
}

bool Cooling::ferriticTransformation(double D, double rho, double temperature, double dt, double cGammaAlpha, 
	bool didFerriticTransformationStart)
{
	if (!didFerriticTransformationStart)
	{
		double p = (*coefficients)[0] * pow(D, -(*coefficients)[1]) * pow(rho, (*coefficients)[2])
			* pow(((*steelCoeff).Ae3 - temperature), (*coefficients)[3]) * dt;
		double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[0]);
		//double s = 0.0001;	//for deterministic test
		if (p <= s) return false;
	}

	double Feq;
	double Xeq;
	if(temperature > (*steelCoeff).Ae1)
	{
		Feq = (cGammaAlpha - (*steelCoeff).c0) / (cGammaAlpha - (*steelCoeff).cAlpha);
		Xeq = Feq / (*steelCoeff).Feut;
	}
	else
	{
		Feq = (*steelCoeff).Feut;
		Xeq = (*steelCoeff).Xeut;
	}
	double C = ((*coefficients)[5] / pow(D, (*coefficients)[9])) *
		exp(-pow((abs(temperature - (*coefficients)[6]) / (*coefficients)[7]), (*coefficients)[8]));
	double xfInPreviousTimestep = volumeFractions[steelPhase::FERRITE].X;
	volumeFractions[steelPhase::FERRITE].X = volumeFractions[steelPhase::FERRITE].X + 
		(C * (Xeq - volumeFractions[steelPhase::FERRITE].X) * dt);
	volumeFractions[steelPhase::FERRITE].F = volumeFractions[steelPhase::FERRITE].X * (*steelCoeff).Feut;
	cGamma = ((*steelCoeff).c0 - (volumeFractions[steelPhase::FERRITE].F * (*steelCoeff).cAlpha)) / 
		(1. - volumeFractions[steelPhase::FERRITE].F);
	sumNeededForComputingFerriteGrainSize += (temperature * (abs(volumeFractions[steelPhase::FERRITE].X - xfInPreviousTimestep)));
	noTimestepsInFerriteTransformation++;
	return true;
}

bool Cooling::pearliticTransformation(double D, double rho, double temperature, double dt, bool didPearliticTransformationStart)
{
	if (!didPearliticTransformationStart)
	{
		double p = (*coefficients)[10] * pow(D, -(*coefficients)[11]) * pow(rho, (*coefficients)[12])
			* pow(((*steelCoeff).Ae1 - temperature), (*coefficients)[13]) * dt;
		double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[0]);
		//double s = 0.0001;	//for deterministic test
		if (p <= s) return false;
	}
	double C = ((*coefficients)[15] / pow(D, (*coefficients)[19])) *
		exp(-pow((abs(temperature - (*coefficients)[16]) / (*coefficients)[17]), (*coefficients)[18]));
	volumeFractions[steelPhase::PEARLITE].X = volumeFractions[steelPhase::PEARLITE].X + 
		(C * (1. - volumeFractions[steelPhase::PEARLITE].X) * dt);
	volumeFractions[steelPhase::PEARLITE].F = volumeFractions[steelPhase::PEARLITE].X * 
		(1. - volumeFractions[steelPhase::FERRITE].F);
	return true;
}

bool Cooling::bainiticTransformation(double D, double rho, double temperature, double dt, bool didBainiticTransformationStart)
{
	if (!didBainiticTransformationStart)
	{
		double p = (*coefficients)[20] * pow(D, -(*coefficients)[21]) * pow(rho, (*coefficients)[22])
			* pow(((*coefficients)[24] - temperature), (*coefficients)[23]) * dt;
		double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[0]);
		//double s = 0.0001;	//for deterministic test
		if (p <= s) return false;
	}
	double C = ((*coefficients)[25] / pow(D, (*coefficients)[29])) *
		exp(-pow((abs(temperature - (*coefficients)[26]) / (*coefficients)[27]), (*coefficients)[28]));
	volumeFractions[steelPhase::BAINITE].X = volumeFractions[steelPhase::BAINITE].X + 
		(C * (1. - volumeFractions[steelPhase::BAINITE].X) * dt);
	volumeFractions[steelPhase::BAINITE].F = volumeFractions[steelPhase::BAINITE].X * 
		(1. - volumeFractions[steelPhase::FERRITE].F - volumeFractions[steelPhase::PEARLITE].F);
	return true;
}

void Cooling::martensiticTransformation()
{
	volumeFractions[steelPhase::MARTENSITE].F = volumeFractions[steelPhase::MARTENSITE].X * 
		(1. - volumeFractions[steelPhase::FERRITE].F - volumeFractions[steelPhase::PEARLITE].F - 
			volumeFractions[steelPhase::BAINITE].F);
}

bool Cooling::didPhaseTransformationsEnd()
{
	double sumOfVolumeFractions = volumeFractions[steelPhase::FERRITE].F + volumeFractions[steelPhase::PEARLITE].F +
		volumeFractions[steelPhase::BAINITE].F + volumeFractions[steelPhase::MARTENSITE].F;
	if (abs(1. - sumOfVolumeFractions) < WHOLE_VOLUME_FULFILLMENT_PRECISION) return true;
	else return false;
}

inline bool Cooling::didPhaseTransformationsEnd(double ferrite_F, double pearlite_F, double bainite_F, double martensite_F)
{
	double sumOfVolumeFractions = ferrite_F + pearlite_F + bainite_F + martensite_F;
	if (abs(1. - sumOfVolumeFractions) < WHOLE_VOLUME_FULFILLMENT_PRECISION) return true;
	else return false;
}

const double Cooling::WHOLE_VOLUME_FULFILLMENT_PRECISION = 0.005;

HeatTreatmentResult* Cooling::stochasticCoolSingleThreaded()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());

	double ferriteGrainSize = 0.;
	//values depending on temperature (which changes over time)
	double cGammaAlpha = 0.;
	double cGammaBeta = 0.;

	for (int j = 0; j < histogram->getData().size(); j++)
	{
		//there will be connection with Hot Deformation Model

		//current average carbon content in the austenite
		cGamma = (*steelCoeff).c0;
		//reset the result variables
		volumeFractions[steelPhase::FERRITE].F = 0.;
		volumeFractions[steelPhase::FERRITE].X = 0.;
		volumeFractions[steelPhase::PEARLITE].F = 0.;
		volumeFractions[steelPhase::PEARLITE].X = 0.;
		volumeFractions[steelPhase::BAINITE].F = 0.;
		volumeFractions[steelPhase::BAINITE].X = 0.;
		volumeFractions[steelPhase::MARTENSITE].F = 0.;
		volumeFractions[steelPhase::MARTENSITE].X = 1.;

		//reset the ferrite grain size variables
		sumNeededForComputingFerriteGrainSize = 0.;
		noTimestepsInFerriteTransformation = 0;

		//auxiliary variables for checking logical conditions
		bool didFerriticTransformationStart = false;
		bool didPearliticTransformationStart = false;
		bool didBainiticTransformationStart = false;
		bool wasTransformationAlreadyStarted = false;

		for (int i = 0; i < temperatures->getData().size(); i++)
		{
			cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * (*temperatures)[i].temperature);
			cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * (*temperatures)[i].temperature);

			//if temperature < Mstart
			if ((*temperatures)[i].temperature < ((*coefficients)[30] - ((*coefficients)[31] * cGamma)))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				martensiticTransformation();
				break;
			}
			//if temperature < Bstart
			else if ((*temperatures)[i].temperature < (*coefficients)[24])
			{
				wasTransformationAlreadyStarted = didBainiticTransformationStart;
				didBainiticTransformationStart = bainiticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), didBainiticTransformationStart);
				if (wasTransformationAlreadyStarted != didBainiticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				if (didBainiticTransformationStart)
				{
					if (didPhaseTransformationsEnd())
					{
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, (*temperatures)[i].temperature);
						break;
					}
					else continue;
				}
			}
			//if cGamma > cGammaBeta
			else if (cGamma > cGammaBeta)
			{
				wasTransformationAlreadyStarted = didPearliticTransformationStart;
				didPearliticTransformationStart = pearliticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), didPearliticTransformationStart);
				if (wasTransformationAlreadyStarted != didPearliticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				if (didPearliticTransformationStart)
				{
					if (didPhaseTransformationsEnd())
					{
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, (*temperatures)[i].temperature);
						break;
					}
					else continue;
				}
			}
			//if temperature < Ae3
			else if ((*temperatures)[i].temperature < (*steelCoeff).Ae3)
			{
				wasTransformationAlreadyStarted = didFerriticTransformationStart;
				didFerriticTransformationStart = ferriticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), cGammaAlpha, didFerriticTransformationStart);
				if (wasTransformationAlreadyStarted != didFerriticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				//if (didFerriticTransformationStart && didPhaseTransformationsEnd()) break;		//for now we don't take into account ending of a ferritic transformation
			}
		}
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, volumeFractions[steelPhase::FERRITE].F);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, volumeFractions[steelPhase::PEARLITE].F);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, volumeFractions[steelPhase::BAINITE].F);
		result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, volumeFractions[steelPhase::MARTENSITE].F);
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] *
			(pow((*histogram)[j].D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - (sumNeededForComputingFerriteGrainSize / ((double)noTimestepsInFerriteTransformation))),
					(*ferriteGrainSizeCoefficients)[2])) * pow((*histogram)[j].rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}
	return result;
}

HeatTreatmentResult* Cooling::stochasticCool()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());
	
#pragma omp parallel for schedule(dynamic)
	for (int j = 0; j < histogram->getData().size(); j++)
	{
		//there will be connection with Hot Deformation Model

		double D = (*histogram)[j].D;
		double rho = (*histogram)[j].rho;

		//current average carbon content in the austenite
		double cGamma = (*steelCoeff).c0;
		//values depending on temperature (which changes over time)
		double cGammaAlpha = 0.;
		double cGammaBeta = 0.;

		//reset the ,,result variables"
		double ferrite_F = 0.;
		double ferrite_X = 0.;
		double pearlite_F = 0.;
		double pearlite_X = 0.;
		double bainite_F = 0.;
		double bainite_X = 0.;
		double martensite_F = 0.;
		double martensite_X = 1.;

		//reset the ferrite grain size variables
		double sumNeededForComputingFerriteGrainSize = 0.;
		double noTimestepsInFerriteTransformation = 0;
		double ferriteGrainSize = 0.;

		//auxiliary variables for checking logical conditions
		bool didFerriticTransformationStart = false;
		bool didPearliticTransformationStart = false;
		bool didBainiticTransformationStart = false;

		for (int i = 0; i < temperatures->getData().size(); i++)
		{
			double temperature = (*temperatures)[i].temperature;
			double dt = (*temperatures).getDeltaTime(i);

			cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * temperature);
			cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * temperature);

			//if temperature < Mstart
			if (temperature < ((*coefficients)[30] - ((*coefficients)[31] * cGamma)))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, temperature);

				//martensitic transformation vvv
				martensite_F = martensite_X * (1. - ferrite_F - pearlite_F - bainite_F);
				//martensitic transformation ^^^

				break;
			}
			//if temperature < Bstart
			else if (temperature < (*coefficients)[24])
			{
				//bainitic transformation vvv
				if (!didBainiticTransformationStart) {
					double p = (*coefficients)[20] * pow(D, -(*coefficients)[21]) * pow(rho, (*coefficients)[22])
						* pow(((*coefficients)[24] - temperature), (*coefficients)[23]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didBainiticTransformationStart = true;
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, temperature);
					}
				}
				if (didBainiticTransformationStart) {
					double C = ((*coefficients)[25] / pow(D, (*coefficients)[29])) *
						exp(-pow((abs(temperature - (*coefficients)[26]) / (*coefficients)[27]), (*coefficients)[28]));
					bainite_X = bainite_X + (C * (1. - bainite_X) * dt);
					bainite_F = bainite_X * (1. - ferrite_F - pearlite_F);

					if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) {
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, temperature);
						break;
					} else continue;
				}
				//bainitic transformation ^^^
			}
			//if cGamma > cGammaBeta
			else if (cGamma > cGammaBeta)
			{
				//pearlitic transformation vvv
				if (!didPearliticTransformationStart) {
					double p = (*coefficients)[10] * pow(D, -(*coefficients)[11]) * pow(rho, (*coefficients)[12])
						* pow(((*steelCoeff).Ae1 - temperature), (*coefficients)[13]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didPearliticTransformationStart = true;
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, temperature);
					}
				}
				if (didPearliticTransformationStart) {
					double C = ((*coefficients)[15] / pow(D, (*coefficients)[19])) *
						exp(-pow((abs(temperature - (*coefficients)[16]) / (*coefficients)[17]), (*coefficients)[18]));
					pearlite_X = pearlite_X + (C * (1. - pearlite_X) * dt);
					pearlite_F = pearlite_X * (1. - ferrite_F);

					if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) {
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, temperature);
						break;
					} else continue;
				}
				//pearlitic transformation ^^^
			}
			//if temperature < Ae3
			else if (temperature < (*steelCoeff).Ae3)
			{
				//ferritic transformation vvv
				if (!didFerriticTransformationStart) {
					double p = (*coefficients)[0] * pow(D, -(*coefficients)[1]) * pow(rho, (*coefficients)[2])
						* pow(((*steelCoeff).Ae3 - temperature), (*coefficients)[3]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didFerriticTransformationStart = true;
						result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, temperature);
					}
				}
				if (didFerriticTransformationStart) {
					double Feq;
					double Xeq;
					if (temperature > (*steelCoeff).Ae1) {
						Feq = (cGammaAlpha - (*steelCoeff).c0) / (cGammaAlpha - (*steelCoeff).cAlpha);
						Xeq = Feq / (*steelCoeff).Feut;
					}
					else {
						Feq = (*steelCoeff).Feut;
						Xeq = (*steelCoeff).Xeut;
					}
					double C = ((*coefficients)[5] / pow(D, (*coefficients)[9])) *
						exp(-pow((abs(temperature - (*coefficients)[6]) / (*coefficients)[7]), (*coefficients)[8]));
					double ferrite_X_inPreviousTimestep = ferrite_X;
					ferrite_X = ferrite_X + (C * (Xeq - ferrite_X) * dt);
					ferrite_F = ferrite_X * (*steelCoeff).Feut;
					cGamma = ((*steelCoeff).c0 - (ferrite_F * (*steelCoeff).cAlpha)) / (1. - ferrite_F);
					sumNeededForComputingFerriteGrainSize += (temperature * (abs(ferrite_X - ferrite_X_inPreviousTimestep)));
					noTimestepsInFerriteTransformation++;

					//if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) break;		//for now we don't take into account ending of a ferritic transformation
				}
				//ferritic transformation ^^^
			}
		}
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, ferrite_F);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, pearlite_F);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, bainite_F);
		result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, martensite_F);
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] * (pow(D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - (sumNeededForComputingFerriteGrainSize / ((double)noTimestepsInFerriteTransformation))),
					(*ferriteGrainSizeCoefficients)[2])) * pow(rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}
	return result;
}

HeatTreatmentResult* Cooling::stochasticCool_ALT()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());

	//variables for counting the points in which given transformation has started
	int noPointsWithFerriticTransformationStarted = 0;
	int noPointsWithFerriticTransformationStartedInCurrentTimestep = 0;
	int noPointsWithPearliticTransformationStarted = 0;
	int noPointsWithPearliticTransformationStartedInCurrentTimestep = 0;
	int noPointsWithBainiticTransformationStarted = 0;
	int noPointsWithBainiticTransformationStartedInCurrentTimestep = 0;

	//currently it is const, if that will change, then a vector martensite_X should be used instead
	const double martensite_X = 1.;

	//*TODO - if we will want to have lesser memory usage, then ferrite_F, pearlite_F and bainite_F can be stored in the result variable (but this probably will slow down the program)
	//current average carbon content in the austenite
	std::vector<double>* cGamma = new std::vector<double>;
	cGamma->resize(histogram->getData().size());

	//,,result variables"
	std::vector<double>* ferrite_F = new std::vector<double>;
	ferrite_F->resize(histogram->getData().size());
	std::vector<double>* ferrite_X = new std::vector<double>;
	ferrite_X->resize(histogram->getData().size());
	std::vector<double>* pearlite_F = new std::vector<double>;
	pearlite_F->resize(histogram->getData().size());
	std::vector<double>* pearlite_X = new std::vector<double>;
	pearlite_X->resize(histogram->getData().size());
	std::vector<double>* bainite_F = new std::vector<double>;
	bainite_F->resize(histogram->getData().size());
	std::vector<double>* bainite_X = new std::vector<double>;
	bainite_X->resize(histogram->getData().size());

	//ferrite grain size variables
	std::vector<double>* sumNeededForComputingFerriteGrainSize = new std::vector<double>;
	sumNeededForComputingFerriteGrainSize->resize(histogram->getData().size());
	std::vector<double>* noTimestepsInFerriteTransformation = new std::vector<double>;
	noTimestepsInFerriteTransformation->resize(histogram->getData().size());

	//auxiliary variables for checking logical conditions
	std::vector<bool>* didFerriticTransformationStart = new std::vector<bool>;
	didFerriticTransformationStart->resize(histogram->getData().size());
	std::vector<bool>* didPearliticTransformationStart = new std::vector<bool>;
	didPearliticTransformationStart->resize(histogram->getData().size());
	std::vector<bool>* didBainiticTransformationStart = new std::vector<bool>;
	didBainiticTransformationStart->resize(histogram->getData().size());
	std::vector<bool>* simulationEnded = new std::vector<bool>;
	simulationEnded->resize(histogram->getData().size());

#pragma omp parallel for schedule(dynamic)
	for (int j = 0; j < histogram->getData().size(); j++) {
		(*cGamma)[j] = (*steelCoeff).c0;
		(*ferrite_F)[j] = 0.;
		(*ferrite_X)[j] = 0.;
		(*pearlite_F)[j] = 0.;
		(*pearlite_X)[j] = 0.;
		(*bainite_F)[j] = 0.;
		(*bainite_X)[j] = 0.;
		(*sumNeededForComputingFerriteGrainSize)[j] = 0.;
		(*noTimestepsInFerriteTransformation)[j] = 0.;
		(*didFerriticTransformationStart)[j] = false;
		(*didPearliticTransformationStart)[j] = false;
		(*didBainiticTransformationStart)[j] = false;
		(*simulationEnded)[j] = false;
	}

	for (int i = 0; i < temperatures->getData().size(); i++) {
		double temperature = (*temperatures)[i].temperature;
		double dt = (*temperatures).getDeltaTime(i);

		//values depending on temperature (which changes over time)
		double cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * temperature);
		double cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * temperature);

#pragma omp parallel for schedule(dynamic)
		for (int j = 0; j < histogram->getData().size(); j++) {

			if ((*simulationEnded)[j]) continue;

			//there will be connection with Hot Deformation Model

			double D = (*histogram)[j].D;
			double rho = (*histogram)[j].rho;

			//if temperature < Mstart
			if (temperature < ((*coefficients)[30] - ((*coefficients)[31] * (*cGamma)[j])))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, temperature);

				//martensitic transformation vvv
				double martensite_F = martensite_X * (1. - (*ferrite_F)[j] - (*pearlite_F)[j] - (*bainite_F)[j]);
				result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, martensite_F);
				//martensitic transformation ^^^

				(*simulationEnded)[j] = true;
				continue;
			}
			//if temperature < Bstart
			else if (temperature < (*coefficients)[24])
			{
				//bainitic transformation vvv
				if (!(*didBainiticTransformationStart)[j]) {
					double p = (*coefficients)[20] * pow(D, -(*coefficients)[21]) * pow(rho, (*coefficients)[22])
						* pow(((*coefficients)[24] - temperature), (*coefficients)[23]) * dt;
					double s = ((std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]))
						* (1. - ((double)noPointsWithBainiticTransformationStarted/(double)histogram->getData().size()));
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						(*didBainiticTransformationStart)[j] = true;
						#pragma omp atomic
						noPointsWithBainiticTransformationStartedInCurrentTimestep++;
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, temperature);
					}
				}
				if ((*didBainiticTransformationStart)[j]) {
					double C = ((*coefficients)[25] / pow(D, (*coefficients)[29])) *
						exp(-pow((abs(temperature - (*coefficients)[26]) / (*coefficients)[27]), (*coefficients)[28]));
					(*bainite_X)[j] = (*bainite_X)[j] + (C * (1. - (*bainite_X)[j]) * dt);
					(*bainite_F)[j] = (*bainite_X)[j] * (1. - (*ferrite_F)[j] - (*pearlite_F)[j]);

					if (didPhaseTransformationsEnd((*ferrite_F)[j], (*pearlite_F)[j], (*bainite_F)[j], 0.)) {
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, temperature);
						(*simulationEnded)[j] = true;
					}
					
					continue;
				}
				//bainitic transformation ^^^
			}
			//if cGamma > cGammaBeta
			else if ((*cGamma)[j] > cGammaBeta)
			{
				//pearlitic transformation vvv
				if (!(*didPearliticTransformationStart)[j]) {
					double p = (*coefficients)[10] * pow(D, -(*coefficients)[11]) * pow(rho, (*coefficients)[12])
						* pow(((*steelCoeff).Ae1 - temperature), (*coefficients)[13]) * dt;
					double s = ((std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]))
						* (1. - ((double)noPointsWithPearliticTransformationStarted / (double)histogram->getData().size()));
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						(*didPearliticTransformationStart)[j] = true;
						#pragma omp atomic
						noPointsWithPearliticTransformationStartedInCurrentTimestep++;
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, temperature);
					}
				}
				if ((*didPearliticTransformationStart)[j]) {
					double C = ((*coefficients)[15] / pow(D, (*coefficients)[19])) *
						exp(-pow((abs(temperature - (*coefficients)[16]) / (*coefficients)[17]), (*coefficients)[18]));
					(*pearlite_X)[j] = (*pearlite_X)[j] + (C * (1. - (*pearlite_X)[j]) * dt);
					(*pearlite_F)[j] = (*pearlite_X)[j] * (1. - (*ferrite_F)[j]);

					if (didPhaseTransformationsEnd((*ferrite_F)[j], (*pearlite_F)[j], (*bainite_F)[j], 0.)) {
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, temperature);
						(*simulationEnded)[j] = true;
					}
					
					continue;
				}
				//pearlitic transformation ^^^
			}
			//if temperature < Ae3
			else if (temperature < (*steelCoeff).Ae3)
			{
				//ferritic transformation vvv
				if (!(*didFerriticTransformationStart)[j]) {
					double p = (*coefficients)[0] * pow(D, -(*coefficients)[1]) * pow(rho, (*coefficients)[2])
						* pow(((*steelCoeff).Ae3 - temperature), (*coefficients)[3]) * dt;
					double s = ((std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]))
						* (1. - ((double)noPointsWithFerriticTransformationStarted / (double)histogram->getData().size()));
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						(*didFerriticTransformationStart)[j] = true;
						#pragma omp atomic
						noPointsWithFerriticTransformationStartedInCurrentTimestep++;
						result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, temperature);
					}
				}
				if ((*didFerriticTransformationStart)[j]) {
					double Feq;
					double Xeq;
					if (temperature > (*steelCoeff).Ae1) {
						Feq = (cGammaAlpha - (*steelCoeff).c0) / (cGammaAlpha - (*steelCoeff).cAlpha);
						Xeq = Feq / (*steelCoeff).Feut;
					}
					else {
						Feq = (*steelCoeff).Feut;
						Xeq = (*steelCoeff).Xeut;
					}
					double C = ((*coefficients)[5] / pow(D, (*coefficients)[9])) *
						exp(-pow((abs(temperature - (*coefficients)[6]) / (*coefficients)[7]), (*coefficients)[8]));
					double ferrite_X_inPreviousTimestep = (*ferrite_X)[j];
					(*ferrite_X)[j] = (*ferrite_X)[j] + (C * (Xeq - (*ferrite_X)[j]) * dt);
					(*ferrite_F)[j] = (*ferrite_X)[j] * (*steelCoeff).Feut;
					(*cGamma)[j] = ((*steelCoeff).c0 - ((*ferrite_F)[j] * (*steelCoeff).cAlpha)) / (1. - (*ferrite_F)[j]);
					(*sumNeededForComputingFerriteGrainSize)[j] += (temperature * (abs((*ferrite_X)[j] - ferrite_X_inPreviousTimestep)));
					(*noTimestepsInFerriteTransformation)[j]++;

					//if (didPhaseTransformationsEnd((*ferrite_F)[j], (*pearlite_F)[j], (*bainite_F)[j], 0.) (*simulationEnded)[j] = true;		//for now we don't take into account ending of a ferritic transformation
				}
				//ferritic transformation ^^^
			}
		}
		noPointsWithFerriticTransformationStarted += noPointsWithFerriticTransformationStartedInCurrentTimestep;
		noPointsWithFerriticTransformationStartedInCurrentTimestep = 0;
		noPointsWithPearliticTransformationStarted += noPointsWithPearliticTransformationStartedInCurrentTimestep;
		noPointsWithPearliticTransformationStartedInCurrentTimestep = 0;
		noPointsWithBainiticTransformationStarted += noPointsWithBainiticTransformationStartedInCurrentTimestep;
		noPointsWithBainiticTransformationStartedInCurrentTimestep = 0;
	}

#pragma omp parallel for schedule(dynamic)
	for (int j = 0; j < histogram->getData().size(); j++) {
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, (*ferrite_F)[j]);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, (*pearlite_F)[j]);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, (*bainite_F)[j]);
		double ferriteGrainSize;
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] * (pow((*histogram)[j].D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - ((*sumNeededForComputingFerriteGrainSize)[j] / ((double)(*noTimestepsInFerriteTransformation)[j]))),
					(*ferriteGrainSizeCoefficients)[2])) * pow((*histogram)[j].rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}

	//memory deallocation
	delete cGamma;
	delete ferrite_F;
	delete ferrite_X;
	delete pearlite_F;
	delete pearlite_X;
	delete bainite_F;
	delete bainite_X;
	delete sumNeededForComputingFerriteGrainSize;
	delete noTimestepsInFerriteTransformation;
	delete didFerriticTransformationStart;
	delete didPearliticTransformationStart;
	delete didBainiticTransformationStart;
	delete simulationEnded;

	return result;
}

HeatTreatmentResult* Cooling::stochasticCoolOptimization_FERRITE_START_singleThreaded()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());

	double ferriteGrainSize = 0.;
	//values depending on temperature (which changes over time)
	double cGammaAlpha = 0.;
	double cGammaBeta = 0.;

	for (int j = 0; j < histogram->getData().size(); j++)
	{
		//there will be connection with Hot Deformation Model

		//current average carbon content in the austenite
		cGamma = (*steelCoeff).c0;
		//reset the result variables
		volumeFractions[steelPhase::FERRITE].F = 0.;
		volumeFractions[steelPhase::FERRITE].X = 0.;
		volumeFractions[steelPhase::PEARLITE].F = 0.;
		volumeFractions[steelPhase::PEARLITE].X = 0.;
		volumeFractions[steelPhase::BAINITE].F = 0.;
		volumeFractions[steelPhase::BAINITE].X = 0.;
		volumeFractions[steelPhase::MARTENSITE].F = 0.;
		volumeFractions[steelPhase::MARTENSITE].X = 1.;

		//reset the ferrite grain size variables
		sumNeededForComputingFerriteGrainSize = 0.;
		noTimestepsInFerriteTransformation = 0;

		//auxiliary variables for checking logical conditions
		bool didFerriticTransformationStart = false;
		bool didPearliticTransformationStart = false;
		bool didBainiticTransformationStart = false;
		bool wasTransformationAlreadyStarted = false;

		for (int i = 0; i < temperatures->getData().size(); i++)
		{
			if (didFerriticTransformationStart || didPearliticTransformationStart || didBainiticTransformationStart) break;

			cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * (*temperatures)[i].temperature);
			cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * (*temperatures)[i].temperature);

			//if temperature < Mstart
			if ((*temperatures)[i].temperature < ((*coefficients)[30] - ((*coefficients)[31] * cGamma)))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				martensiticTransformation();
				break;
			}
			//if temperature < Bstart
			else if ((*temperatures)[i].temperature < (*coefficients)[24])
			{
				wasTransformationAlreadyStarted = didBainiticTransformationStart;
				didBainiticTransformationStart = bainiticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), didBainiticTransformationStart);
				if (wasTransformationAlreadyStarted != didBainiticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				if (didBainiticTransformationStart)
				{
					if (didPhaseTransformationsEnd())
					{
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, (*temperatures)[i].temperature);
						break;
					}
					else continue;
				}
			}
			//if cGamma > cGammaBeta
			else if (cGamma > cGammaBeta)
			{
				wasTransformationAlreadyStarted = didPearliticTransformationStart;
				didPearliticTransformationStart = pearliticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), didPearliticTransformationStart);
				if (wasTransformationAlreadyStarted != didPearliticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				if (didPearliticTransformationStart)
				{
					if (didPhaseTransformationsEnd())
					{
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, (*temperatures)[i].temperature);
						break;
					}
					else continue;
				}
			}
			//if temperature < Ae3
			else if ((*temperatures)[i].temperature < (*steelCoeff).Ae3)
			{
				wasTransformationAlreadyStarted = didFerriticTransformationStart;
				didFerriticTransformationStart = ferriticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), cGammaAlpha, didFerriticTransformationStart);
				if (wasTransformationAlreadyStarted != didFerriticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				//if (didFerriticTransformationStart && didPhaseTransformationsEnd()) break;		//for now we don't take into account ending of a ferritic transformation
			}
		}
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, volumeFractions[steelPhase::FERRITE].F);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, volumeFractions[steelPhase::PEARLITE].F);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, volumeFractions[steelPhase::BAINITE].F);
		result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, volumeFractions[steelPhase::MARTENSITE].F);
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] *
			(pow((*histogram)[j].D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - (sumNeededForComputingFerriteGrainSize / ((double)noTimestepsInFerriteTransformation))),
					(*ferriteGrainSizeCoefficients)[2])) * pow((*histogram)[j].rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}
	return result;
}

HeatTreatmentResult* Cooling::stochasticCoolOptimization_FERRITE_FRACTION_and_PEARLITE_START_singleThreaded()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());

	double ferriteGrainSize = 0.;
	//values depending on temperature (which changes over time)
	double cGammaAlpha = 0.;
	double cGammaBeta = 0.;

	for (int j = 0; j < histogram->getData().size(); j++)
	{
		//there will be connection with Hot Deformation Model

		//current average carbon content in the austenite
		cGamma = (*steelCoeff).c0;
		//reset the result variables
		volumeFractions[steelPhase::FERRITE].F = 0.;
		volumeFractions[steelPhase::FERRITE].X = 0.;
		volumeFractions[steelPhase::PEARLITE].F = 0.;
		volumeFractions[steelPhase::PEARLITE].X = 0.;
		volumeFractions[steelPhase::BAINITE].F = 0.;
		volumeFractions[steelPhase::BAINITE].X = 0.;
		volumeFractions[steelPhase::MARTENSITE].F = 0.;
		volumeFractions[steelPhase::MARTENSITE].X = 1.;

		//reset the ferrite grain size variables
		sumNeededForComputingFerriteGrainSize = 0.;
		noTimestepsInFerriteTransformation = 0;

		//auxiliary variables for checking logical conditions
		bool didFerriticTransformationStart = false;
		bool didPearliticTransformationStart = false;
		bool didBainiticTransformationStart = false;
		bool wasTransformationAlreadyStarted = false;

		for (int i = 0; i < temperatures->getData().size(); i++)
		{
			if (didPearliticTransformationStart || didBainiticTransformationStart) break;

			cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * (*temperatures)[i].temperature);
			cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * (*temperatures)[i].temperature);

			//if temperature < Mstart
			if ((*temperatures)[i].temperature < ((*coefficients)[30] - ((*coefficients)[31] * cGamma)))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				martensiticTransformation();
				break;
			}
			//if temperature < Bstart
			else if ((*temperatures)[i].temperature < (*coefficients)[24])
			{
				wasTransformationAlreadyStarted = didBainiticTransformationStart;
				didBainiticTransformationStart = bainiticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), didBainiticTransformationStart);
				if (wasTransformationAlreadyStarted != didBainiticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				if (didBainiticTransformationStart)
				{
					if (didPhaseTransformationsEnd())
					{
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, (*temperatures)[i].temperature);
						break;
					}
					else continue;
				}
			}
			//if cGamma > cGammaBeta
			else if (cGamma > cGammaBeta)
			{
				wasTransformationAlreadyStarted = didPearliticTransformationStart;
				didPearliticTransformationStart = pearliticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), didPearliticTransformationStart);
				if (wasTransformationAlreadyStarted != didPearliticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				if (didPearliticTransformationStart)
				{
					if (didPhaseTransformationsEnd())
					{
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, (*temperatures)[i].temperature);
						break;
					}
					else continue;
				}
			}
			//if temperature < Ae3
			else if ((*temperatures)[i].temperature < (*steelCoeff).Ae3)
			{
				wasTransformationAlreadyStarted = didFerriticTransformationStart;
				didFerriticTransformationStart = ferriticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), cGammaAlpha, didFerriticTransformationStart);
				if (wasTransformationAlreadyStarted != didFerriticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				//if (didFerriticTransformationStart && didPhaseTransformationsEnd()) break;		//for now we don't take into account ending of a ferritic transformation
			}
		}
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, volumeFractions[steelPhase::FERRITE].F);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, volumeFractions[steelPhase::PEARLITE].F);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, volumeFractions[steelPhase::BAINITE].F);
		result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, volumeFractions[steelPhase::MARTENSITE].F);
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] *
			(pow((*histogram)[j].D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - (sumNeededForComputingFerriteGrainSize / ((double)noTimestepsInFerriteTransformation))),
					(*ferriteGrainSizeCoefficients)[2])) * pow((*histogram)[j].rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}
	return result;
}

HeatTreatmentResult* Cooling::stochasticCoolOptimization_PEARLITE_FRACTION_and_BAINITE_START_and_PEARLITE_END_singleThreaded()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());

	double ferriteGrainSize = 0.;
	//values depending on temperature (which changes over time)
	double cGammaAlpha = 0.;
	double cGammaBeta = 0.;

	for (int j = 0; j < histogram->getData().size(); j++)
	{
		//there will be connection with Hot Deformation Model

		//current average carbon content in the austenite
		cGamma = (*steelCoeff).c0;
		//reset the result variables
		volumeFractions[steelPhase::FERRITE].F = 0.;
		volumeFractions[steelPhase::FERRITE].X = 0.;
		volumeFractions[steelPhase::PEARLITE].F = 0.;
		volumeFractions[steelPhase::PEARLITE].X = 0.;
		volumeFractions[steelPhase::BAINITE].F = 0.;
		volumeFractions[steelPhase::BAINITE].X = 0.;
		volumeFractions[steelPhase::MARTENSITE].F = 0.;
		volumeFractions[steelPhase::MARTENSITE].X = 1.;

		//reset the ferrite grain size variables
		sumNeededForComputingFerriteGrainSize = 0.;
		noTimestepsInFerriteTransformation = 0;

		//auxiliary variables for checking logical conditions
		bool didFerriticTransformationStart = false;
		bool didPearliticTransformationStart = false;
		bool didBainiticTransformationStart = false;
		bool wasTransformationAlreadyStarted = false;

		for (int i = 0; i < temperatures->getData().size(); i++)
		{
			if (didBainiticTransformationStart) break;

			cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * (*temperatures)[i].temperature);
			cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * (*temperatures)[i].temperature);

			//if temperature < Mstart
			if ((*temperatures)[i].temperature < ((*coefficients)[30] - ((*coefficients)[31] * cGamma)))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				martensiticTransformation();
				break;
			}
			//if temperature < Bstart
			else if ((*temperatures)[i].temperature < (*coefficients)[24])
			{
				wasTransformationAlreadyStarted = didBainiticTransformationStart;
				didBainiticTransformationStart = bainiticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), didBainiticTransformationStart);
				if (wasTransformationAlreadyStarted != didBainiticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				if (didBainiticTransformationStart)
				{
					if (didPhaseTransformationsEnd())
					{
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, (*temperatures)[i].temperature);
						break;
					}
					else continue;
				}
			}
			//if cGamma > cGammaBeta
			else if (cGamma > cGammaBeta)
			{
				wasTransformationAlreadyStarted = didPearliticTransformationStart;
				didPearliticTransformationStart = pearliticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), didPearliticTransformationStart);
				if (wasTransformationAlreadyStarted != didPearliticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				if (didPearliticTransformationStart)
				{
					if (didPhaseTransformationsEnd())
					{
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, (*temperatures)[i].temperature);
						break;
					}
					else continue;
				}
			}
			//if temperature < Ae3
			else if ((*temperatures)[i].temperature < (*steelCoeff).Ae3)
			{
				wasTransformationAlreadyStarted = didFerriticTransformationStart;
				didFerriticTransformationStart = ferriticTransformation((*histogram)[j].D, (*histogram)[j].rho,
					(*temperatures)[i].temperature, (*temperatures).getDeltaTime(i), cGammaAlpha, didFerriticTransformationStart);
				if (wasTransformationAlreadyStarted != didFerriticTransformationStart)	//== transformation started in this step
					result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, (*temperatures)[i].temperature);
				//if (didFerriticTransformationStart && didPhaseTransformationsEnd()) break;		//for now we don't take into account ending of a ferritic transformation
			}
		}
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, volumeFractions[steelPhase::FERRITE].F);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, volumeFractions[steelPhase::PEARLITE].F);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, volumeFractions[steelPhase::BAINITE].F);
		result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, volumeFractions[steelPhase::MARTENSITE].F);
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] *
			(pow((*histogram)[j].D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - (sumNeededForComputingFerriteGrainSize / ((double)noTimestepsInFerriteTransformation))),
					(*ferriteGrainSizeCoefficients)[2])) * pow((*histogram)[j].rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}
	return result;
}

HeatTreatmentResult* Cooling::stochasticCoolOptimization_FERRITE_START()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());

#pragma omp parallel for schedule(dynamic)
	for (int j = 0; j < histogram->getData().size(); j++)
	{
		//there will be connection with Hot Deformation Model
		double D = (*histogram)[j].D;
		double rho = (*histogram)[j].rho;

		//current average carbon content in the austenite
		double cGamma = (*steelCoeff).c0;
		//values depending on temperature (which changes over time)
		double cGammaAlpha = 0.;
		double cGammaBeta = 0.;

		//reset the ,,result variables"
		double ferrite_F = 0.;
		double ferrite_X = 0.;
		double pearlite_F = 0.;
		double pearlite_X = 0.;
		double bainite_F = 0.;
		double bainite_X = 0.;
		double martensite_F = 0.;
		double martensite_X = 1.;

		//reset the ferrite grain size variables
		double sumNeededForComputingFerriteGrainSize = 0.;
		double noTimestepsInFerriteTransformation = 0;
		double ferriteGrainSize = 0.;

		//auxiliary variables for checking logical conditions
		bool didFerriticTransformationStart = false;
		bool didPearliticTransformationStart = false;
		bool didBainiticTransformationStart = false;

		for (int i = 0; i < temperatures->getData().size(); i++)
		{
			if (didFerriticTransformationStart || didPearliticTransformationStart || didBainiticTransformationStart) break;

			double temperature = (*temperatures)[i].temperature;
			double dt = (*temperatures).getDeltaTime(i);

			cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * temperature);
			cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * temperature);

			//if temperature < Mstart
			if (temperature < ((*coefficients)[30] - ((*coefficients)[31] * cGamma)))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, temperature);

				//martensitic transformation vvv
				martensite_F = martensite_X * (1. - ferrite_F - pearlite_F - bainite_F);
				//martensitic transformation ^^^

				break;
			}
			//if temperature < Bstart
			else if (temperature < (*coefficients)[24])
			{
				//bainitic transformation vvv
				if (!didBainiticTransformationStart) {
					double p = (*coefficients)[20] * pow(D, -(*coefficients)[21]) * pow(rho, (*coefficients)[22])
						* pow(((*coefficients)[24] - temperature), (*coefficients)[23]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didBainiticTransformationStart = true;
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, temperature);
					}
				}
				if (didBainiticTransformationStart) {
					double C = ((*coefficients)[25] / pow(D, (*coefficients)[29])) *
						exp(-pow((abs(temperature - (*coefficients)[26]) / (*coefficients)[27]), (*coefficients)[28]));
					bainite_X = bainite_X + (C * (1. - bainite_X) * dt);
					bainite_F = bainite_X * (1. - ferrite_F - pearlite_F);

					if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) {
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, temperature);
						break;
					}
					else continue;
				}
				//bainitic transformation ^^^
			}
			//if cGamma > cGammaBeta
			else if (cGamma > cGammaBeta)
			{
				//pearlitic transformation vvv
				if (!didPearliticTransformationStart) {
					double p = (*coefficients)[10] * pow(D, -(*coefficients)[11]) * pow(rho, (*coefficients)[12])
						* pow(((*steelCoeff).Ae1 - temperature), (*coefficients)[13]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didPearliticTransformationStart = true;
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, temperature);
					}
				}
				if (didPearliticTransformationStart) {
					double C = ((*coefficients)[15] / pow(D, (*coefficients)[19])) *
						exp(-pow((abs(temperature - (*coefficients)[16]) / (*coefficients)[17]), (*coefficients)[18]));
					pearlite_X = pearlite_X + (C * (1. - pearlite_X) * dt);
					pearlite_F = pearlite_X * (1. - ferrite_F);

					if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) {
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, temperature);
						break;
					}
					else continue;
				}
				//pearlitic transformation ^^^
			}
			//if temperature < Ae3
			else if (temperature < (*steelCoeff).Ae3)
			{
				//ferritic transformation vvv
				if (!didFerriticTransformationStart) {
					double p = (*coefficients)[0] * pow(D, -(*coefficients)[1]) * pow(rho, (*coefficients)[2])
						* pow(((*steelCoeff).Ae3 - temperature), (*coefficients)[3]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didFerriticTransformationStart = true;
						result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, temperature);
					}
				}
				if (didFerriticTransformationStart) {
					double Feq;
					double Xeq;
					if (temperature > (*steelCoeff).Ae1) {
						Feq = (cGammaAlpha - (*steelCoeff).c0) / (cGammaAlpha - (*steelCoeff).cAlpha);
						Xeq = Feq / (*steelCoeff).Feut;
					}
					else {
						Feq = (*steelCoeff).Feut;
						Xeq = (*steelCoeff).Xeut;
					}
					double C = ((*coefficients)[5] / pow(D, (*coefficients)[9])) *
						exp(-pow((abs(temperature - (*coefficients)[6]) / (*coefficients)[7]), (*coefficients)[8]));
					double ferrite_X_inPreviousTimestep = ferrite_X;
					ferrite_X = ferrite_X + (C * (Xeq - ferrite_X) * dt);
					ferrite_F = ferrite_X * (*steelCoeff).Feut;
					cGamma = ((*steelCoeff).c0 - (ferrite_F * (*steelCoeff).cAlpha)) / (1. - ferrite_F);
					sumNeededForComputingFerriteGrainSize += (temperature * (abs(ferrite_X - ferrite_X_inPreviousTimestep)));
					noTimestepsInFerriteTransformation++;

					//if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) break;		//for now we don't take into account ending of a ferritic transformation
				}
				//ferritic transformation ^^^
			}
		}
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, ferrite_F);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, pearlite_F);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, bainite_F);
		result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, martensite_F);
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] *
			(pow((*histogram)[j].D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - (sumNeededForComputingFerriteGrainSize / ((double)noTimestepsInFerriteTransformation))),
					(*ferriteGrainSizeCoefficients)[2])) * pow((*histogram)[j].rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}
	return result;
}

HeatTreatmentResult* Cooling::stochasticCoolOptimization_FERRITE_FRACTION_and_PEARLITE_START()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());

#pragma omp parallel for schedule(dynamic)
	for (int j = 0; j < histogram->getData().size(); j++)
	{
		//there will be connection with Hot Deformation Model
		double D = (*histogram)[j].D;
		double rho = (*histogram)[j].rho;

		//current average carbon content in the austenite
		double cGamma = (*steelCoeff).c0;
		//values depending on temperature (which changes over time)
		double cGammaAlpha = 0.;
		double cGammaBeta = 0.;

		//reset the ,,result variables"
		double ferrite_F = 0.;
		double ferrite_X = 0.;
		double pearlite_F = 0.;
		double pearlite_X = 0.;
		double bainite_F = 0.;
		double bainite_X = 0.;
		double martensite_F = 0.;
		double martensite_X = 1.;

		//reset the ferrite grain size variables
		double sumNeededForComputingFerriteGrainSize = 0.;
		double noTimestepsInFerriteTransformation = 0;
		double ferriteGrainSize = 0.;

		//auxiliary variables for checking logical conditions
		bool didFerriticTransformationStart = false;
		bool didPearliticTransformationStart = false;
		bool didBainiticTransformationStart = false;

		for (int i = 0; i < temperatures->getData().size(); i++)
		{
			if (didPearliticTransformationStart || didBainiticTransformationStart) break;

			double temperature = (*temperatures)[i].temperature;
			double dt = (*temperatures).getDeltaTime(i);

			cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * temperature);
			cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * temperature);

			//if temperature < Mstart
			if (temperature < ((*coefficients)[30] - ((*coefficients)[31] * cGamma)))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, temperature);

				//martensitic transformation vvv
				martensite_F = martensite_X * (1. - ferrite_F - pearlite_F - bainite_F);
				//martensitic transformation ^^^

				break;
			}
			//if temperature < Bstart
			else if (temperature < (*coefficients)[24])
			{
				//bainitic transformation vvv
				if (!didBainiticTransformationStart) {
					double p = (*coefficients)[20] * pow(D, -(*coefficients)[21]) * pow(rho, (*coefficients)[22])
						* pow(((*coefficients)[24] - temperature), (*coefficients)[23]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didBainiticTransformationStart = true;
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, temperature);
					}
				}
				if (didBainiticTransformationStart) {
					double C = ((*coefficients)[25] / pow(D, (*coefficients)[29])) *
						exp(-pow((abs(temperature - (*coefficients)[26]) / (*coefficients)[27]), (*coefficients)[28]));
					bainite_X = bainite_X + (C * (1. - bainite_X) * dt);
					bainite_F = bainite_X * (1. - ferrite_F - pearlite_F);

					if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) {
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, temperature);
						break;
					}
					else continue;
				}
				//bainitic transformation ^^^
			}
			//if cGamma > cGammaBeta
			else if (cGamma > cGammaBeta)
			{
				//pearlitic transformation vvv
				if (!didPearliticTransformationStart) {
					double p = (*coefficients)[10] * pow(D, -(*coefficients)[11]) * pow(rho, (*coefficients)[12])
						* pow(((*steelCoeff).Ae1 - temperature), (*coefficients)[13]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didPearliticTransformationStart = true;
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, temperature);
					}
				}
				if (didPearliticTransformationStart) {
					double C = ((*coefficients)[15] / pow(D, (*coefficients)[19])) *
						exp(-pow((abs(temperature - (*coefficients)[16]) / (*coefficients)[17]), (*coefficients)[18]));
					pearlite_X = pearlite_X + (C * (1. - pearlite_X) * dt);
					pearlite_F = pearlite_X * (1. - ferrite_F);

					if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) {
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, temperature);
						break;
					}
					else continue;
				}
				//pearlitic transformation ^^^
			}
			//if temperature < Ae3
			else if (temperature < (*steelCoeff).Ae3)
			{
				//ferritic transformation vvv
				if (!didFerriticTransformationStart) {
					double p = (*coefficients)[0] * pow(D, -(*coefficients)[1]) * pow(rho, (*coefficients)[2])
						* pow(((*steelCoeff).Ae3 - temperature), (*coefficients)[3]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didFerriticTransformationStart = true;
						result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, temperature);
					}
				}
				if (didFerriticTransformationStart) {
					double Feq;
					double Xeq;
					if (temperature > (*steelCoeff).Ae1) {
						Feq = (cGammaAlpha - (*steelCoeff).c0) / (cGammaAlpha - (*steelCoeff).cAlpha);
						Xeq = Feq / (*steelCoeff).Feut;
					}
					else {
						Feq = (*steelCoeff).Feut;
						Xeq = (*steelCoeff).Xeut;
					}
					double C = ((*coefficients)[5] / pow(D, (*coefficients)[9])) *
						exp(-pow((abs(temperature - (*coefficients)[6]) / (*coefficients)[7]), (*coefficients)[8]));
					double ferrite_X_inPreviousTimestep = ferrite_X;
					ferrite_X = ferrite_X + (C * (Xeq - ferrite_X) * dt);
					ferrite_F = ferrite_X * (*steelCoeff).Feut;
					cGamma = ((*steelCoeff).c0 - (ferrite_F * (*steelCoeff).cAlpha)) / (1. - ferrite_F);
					sumNeededForComputingFerriteGrainSize += (temperature * (abs(ferrite_X - ferrite_X_inPreviousTimestep)));
					noTimestepsInFerriteTransformation++;

					//if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) break;		//for now we don't take into account ending of a ferritic transformation
				}
				//ferritic transformation ^^^
			}
		}
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, ferrite_F);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, pearlite_F);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, bainite_F);
		result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, martensite_F);
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] *
			(pow((*histogram)[j].D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - (sumNeededForComputingFerriteGrainSize / ((double)noTimestepsInFerriteTransformation))),
					(*ferriteGrainSizeCoefficients)[2])) * pow((*histogram)[j].rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}
	return result;
}

HeatTreatmentResult* Cooling::stochasticCoolOptimization_PEARLITE_FRACTION_and_BAINITE_START_and_PEARLITE_END()
{
	//result object
	HeatTreatmentResult* result = new HeatTreatmentResult(histogram->getData().size());

#pragma omp parallel for schedule(dynamic)
	for (int j = 0; j < histogram->getData().size(); j++)
	{
		//there will be connection with Hot Deformation Model
		double D = (*histogram)[j].D;
		double rho = (*histogram)[j].rho;

		//current average carbon content in the austenite
		double cGamma = (*steelCoeff).c0;
		//values depending on temperature (which changes over time)
		double cGammaAlpha = 0.;
		double cGammaBeta = 0.;

		//reset the ,,result variables"
		double ferrite_F = 0.;
		double ferrite_X = 0.;
		double pearlite_F = 0.;
		double pearlite_X = 0.;
		double bainite_F = 0.;
		double bainite_X = 0.;
		double martensite_F = 0.;
		double martensite_X = 1.;

		//reset the ferrite grain size variables
		double sumNeededForComputingFerriteGrainSize = 0.;
		double noTimestepsInFerriteTransformation = 0;
		double ferriteGrainSize = 0.;

		//auxiliary variables for checking logical conditions
		bool didFerriticTransformationStart = false;
		bool didPearliticTransformationStart = false;
		bool didBainiticTransformationStart = false;

		for (int i = 0; i < temperatures->getData().size(); i++)
		{
			if (didBainiticTransformationStart) break;

			double temperature = (*temperatures)[i].temperature;
			double dt = (*temperatures).getDeltaTime(i);

			cGammaAlpha = (*steelCoeff).cGammaAlpha0 + ((*steelCoeff).cGammaAlpha1 * temperature);
			cGammaBeta = (*steelCoeff).cGammaBeta0 + ((*steelCoeff).cGammaBeta1 * temperature);

			//if temperature < Mstart
			if (temperature < ((*coefficients)[30] - ((*coefficients)[31] * cGamma)))
			{
				result->setData(j, enumTransformationResult::MARTENSITE_TEMPERATURE_START, temperature);

				//martensitic transformation vvv
				martensite_F = martensite_X * (1. - ferrite_F - pearlite_F - bainite_F);
				//martensitic transformation ^^^

				break;
			}
			//if temperature < Bstart
			else if (temperature < (*coefficients)[24])
			{
				//bainitic transformation vvv
				if (!didBainiticTransformationStart) {
					double p = (*coefficients)[20] * pow(D, -(*coefficients)[21]) * pow(rho, (*coefficients)[22])
						* pow(((*coefficients)[24] - temperature), (*coefficients)[23]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didBainiticTransformationStart = true;
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_START, temperature);
					}
				}
				if (didBainiticTransformationStart) {
					double C = ((*coefficients)[25] / pow(D, (*coefficients)[29])) *
						exp(-pow((abs(temperature - (*coefficients)[26]) / (*coefficients)[27]), (*coefficients)[28]));
					bainite_X = bainite_X + (C * (1. - bainite_X) * dt);
					bainite_F = bainite_X * (1. - ferrite_F - pearlite_F);

					if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) {
						result->setData(j, enumTransformationResult::BAINITE_TEMPERATURE_END, temperature);
						break;
					}
					else continue;
				}
				//bainitic transformation ^^^
			}
			//if cGamma > cGammaBeta
			else if (cGamma > cGammaBeta)
			{
				//pearlitic transformation vvv
				if (!didPearliticTransformationStart) {
					double p = (*coefficients)[10] * pow(D, -(*coefficients)[11]) * pow(rho, (*coefficients)[12])
						* pow(((*steelCoeff).Ae1 - temperature), (*coefficients)[13]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didPearliticTransformationStart = true;
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_START, temperature);
					}
				}
				if (didPearliticTransformationStart) {
					double C = ((*coefficients)[15] / pow(D, (*coefficients)[19])) *
						exp(-pow((abs(temperature - (*coefficients)[16]) / (*coefficients)[17]), (*coefficients)[18]));
					pearlite_X = pearlite_X + (C * (1. - pearlite_X) * dt);
					pearlite_F = pearlite_X * (1. - ferrite_F);

					if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) {
						result->setData(j, enumTransformationResult::PEARLITE_TEMPERATURE_END, temperature);
						break;
					}
					else continue;
				}
				//pearlitic transformation ^^^
			}
			//if temperature < Ae3
			else if (temperature < (*steelCoeff).Ae3)
			{
				//ferritic transformation vvv
				if (!didFerriticTransformationStart) {
					double p = (*coefficients)[0] * pow(D, -(*coefficients)[1]) * pow(rho, (*coefficients)[2])
						* pow(((*steelCoeff).Ae3 - temperature), (*coefficients)[3]) * dt;
					double s = (std::uniform_real_distribution<double>(0., 1.))(randomGenerators[omp_get_thread_num()]);
					//double s = 0.0001;	//for deterministic test
					if (p > s) {
						didFerriticTransformationStart = true;
						result->setData(j, enumTransformationResult::FERRITE_TEMPERATURE_START, temperature);
					}
				}
				if (didFerriticTransformationStart) {
					double Feq;
					double Xeq;
					if (temperature > (*steelCoeff).Ae1) {
						Feq = (cGammaAlpha - (*steelCoeff).c0) / (cGammaAlpha - (*steelCoeff).cAlpha);
						Xeq = Feq / (*steelCoeff).Feut;
					}
					else {
						Feq = (*steelCoeff).Feut;
						Xeq = (*steelCoeff).Xeut;
					}
					double C = ((*coefficients)[5] / pow(D, (*coefficients)[9])) *
						exp(-pow((abs(temperature - (*coefficients)[6]) / (*coefficients)[7]), (*coefficients)[8]));
					double ferrite_X_inPreviousTimestep = ferrite_X;
					ferrite_X = ferrite_X + (C * (Xeq - ferrite_X) * dt);
					ferrite_F = ferrite_X * (*steelCoeff).Feut;
					cGamma = ((*steelCoeff).c0 - (ferrite_F * (*steelCoeff).cAlpha)) / (1. - ferrite_F);
					sumNeededForComputingFerriteGrainSize += (temperature * (abs(ferrite_X - ferrite_X_inPreviousTimestep)));
					noTimestepsInFerriteTransformation++;

					//if (didPhaseTransformationsEnd(ferrite_F, pearlite_F, bainite_F, martensite_F)) break;		//for now we don't take into account ending of a ferritic transformation
				}
				//ferritic transformation ^^^
			}
		}
		//save the result variables
		result->setData(j, enumTransformationResult::FERRITE_VOLUME_FRACTION, ferrite_F);
		result->setData(j, enumTransformationResult::PEARLITE_VOLUME_FRACTION, pearlite_F);
		result->setData(j, enumTransformationResult::BAINITE_VOLUME_FRACTION, bainite_F);
		result->setData(j, enumTransformationResult::MARTENSITE_VOLUME_FRACTION, martensite_F);
		if (noTimestepsInFerriteTransformation == 0) ferriteGrainSize = 0.;
		else ferriteGrainSize = (*ferriteGrainSizeCoefficients)[0] *
			(pow((*histogram)[j].D, (*ferriteGrainSizeCoefficients)[1]) /
				pow(((*steelCoeff).Ae3 - (sumNeededForComputingFerriteGrainSize / ((double)noTimestepsInFerriteTransformation))),
					(*ferriteGrainSizeCoefficients)[2])) * pow((*histogram)[j].rho, (*ferriteGrainSizeCoefficients)[3]);
		result->setData(j, enumTransformationResult::FERRITE_GRAIN_SIZE, ferriteGrainSize);
	}
	return result;
}