#pragma once
#include <fstream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include "Cooling.h"
#include "HeatTreatmentResult.h"
#include "histogram.h"

//TEST GUIDE
//to check if stability of result of given model satisfies you, run method checkStability, then move to result directory and
//check file stabilityCheckAverageResults.csv - if there is at least one 0 (not counting Ferrite Grain Size), the result is most
//probably not stable - most probably you don't have to check other things ; if results in this file satisfies you, you can check
//file histogramsDistances.csv - there are distances between each histogram being part of the result - you can check if results in
//this file satisfies you, if it is so, you can check file stabilityCheckHistograms.r - you can run R script (ctrl+a -> ctrl+Enter 
//in Rstudio) and take a look at the result histograms ; detailed and average results are also available, but you don't have to read
//them to tell if result is stable
class StabilityManualTest
{
	static bool checkStabilityOfAverageResult(std::vector<std::map<enumTransformationResult, double>>* averageResults, double maxValueDifference,
		enumTransformationResult resultParameter);

public:
	static void checkStability(Cooling* model, std::string _resultDirectory = "");
};