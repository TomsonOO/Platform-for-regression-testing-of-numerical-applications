#pragma once
#include <vector>
#include <map>
#include <fstream>
#include <algorithm>
#include "histogram.h"
#include "Cooling.h"
#include "HeatTreatmentResult.h"

struct sensitivityAnalysisResult
{
	int coefficientNumber;
	std::map<enumTransformationResult, double> sensitivityFactorsParameters;
	std::map<enumTransformationResult, double> sensitivityFactorsHistograms;

	sensitivityAnalysisResult(int _coefficientNumber,
		std::map<enumTransformationResult, double> _sensitivityFactorsParameters,
		std::map<enumTransformationResult, double> _sensitivityFactorsHistograms);
};

//operators needed for comparing
bool operator== (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2);
bool operator!= (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2);
bool operator< (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2);
bool operator> (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2);
bool operator<= (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2);
bool operator>= (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2);

double computeSensitivityFactor(double coefficient, double deltaCoefficient, double baseY, double analysedY);

//NOTE: this function will sort input vector
void writeResultOfSensitivityAnalysisToFile(std::vector<sensitivityAnalysisResult> vectorOfResults, std::string filename);

sensitivityAnalysisResult performSensitivityAnalysis(Cooling* model, HeatTreatmentResult* baseResult, std::array<double, 32>* coefficients,
	int coefficientNumber, double sensitivityAnalysisRatio, distanceBetweenHistograms distanceBetweenHistogramsType);