#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <cctype>
#include "steelPhase.cpp"
#include "histogram.h"

//TODO: possible improvement: move all enum information to this place (e.g. rScriptOutputParametersNames) ; same with other enums
enum class enumTransformationResult
{
	FERRITE_TEMPERATURE_START,
	PEARLITE_TEMPERATURE_START,
	PEARLITE_TEMPERATURE_END,
	BAINITE_TEMPERATURE_START,
	BAINITE_TEMPERATURE_END,
	MARTENSITE_TEMPERATURE_START,
	FERRITE_VOLUME_FRACTION,
	PEARLITE_VOLUME_FRACTION,
	BAINITE_VOLUME_FRACTION,
	MARTENSITE_VOLUME_FRACTION,
	FERRITE_GRAIN_SIZE
};

class HeatTreatmentResult
{
	std::vector<std::map<enumTransformationResult, double>> data;
	std::map<enumTransformationResult, histogram> histograms;

	void fillWithInitialData(int noPoints);
	void createHistogram(enumTransformationResult whichField, int numberOfBins, histogramType hType = histogramType::CARE_ABOUT_OUTLIERS);
	void createHistogram(enumTransformationResult whichField, histogram* referenceHistogram);
	
public:
	static const double DOES_NOT_EXIST_SPECIAL_VALUE;
	static const double ZERO_SPECIAL_VALUE;

	HeatTreatmentResult(int noPoints);
	//constructor loading data from a (.csv) file
	HeatTreatmentResult(std::string filename);
	void setData(int noPoint, enumTransformationResult whichField, double value);
	double countAverageValue(enumTransformationResult whichField);
	void createHistogramsOutOfData(int numberOfBins, histogramType hType = histogramType::CARE_ABOUT_OUTLIERS);
	void createHistogramsOutOfData(HeatTreatmentResult* referenceResult);
	//method saving data from object to .csv file
	void saveToCSV(std::string filename);
	//method saving average result values to .csv file
	void saveAverageResultValuesToCSV(std::string filename);

	std::vector<std::map<enumTransformationResult, double>> getData();
	std::map<enumTransformationResult, histogram> getHistograms();
	std::map<enumTransformationResult, double> operator[](int a) const;
};