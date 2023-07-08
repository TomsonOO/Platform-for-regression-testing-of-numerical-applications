//Jakub Foryś   AGH   2022
#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <omp.h>
#include "TemperatureLoader.h"
#include "HistogramRhoAndD.h"
#include "Cooling.h"
#include "HeatTreatmentResult.h"
#include "steelPhase.cpp"
#include "histogram.h"
#include "stabilityManualTest.h"

//#define PERFORM_SENSITIVITY_ANALYSIS

#ifdef PERFORM_SENSITIVITY_ANALYSIS
#include "sensitivityAnalysis.h"

//sensitivity analysis shall be performed for coefiicients with following numbers
//IMPORTANT: it uses 1-based indexing
std::vector<int> coefficientsToAnalyse{ 2, 3, 7, 18 };
//coefficient delta in sensitivity analysis, 
//i.e. analysed_coefficient will be changed to analysed_coefficient + sensitivityAnalysisRatio*analysed_coefficient
double sensitivityAnalysisRatio = 0.1;
#endif

int main(int argc, char** argv)
{
    //IMPORTANT: there should be set the optimal number of threads for the CPU executing the code
    //otherwise the computations may be slower than single threaded version
    omp_set_num_threads(optimalNumberOfThreads);

    std::array<double, 32>* coefficientsSteel18902Proteus = new std::array<double, 32>;
    if (argc >= (coefficientsSteel18902Proteus->size() + 1))
    {
        for (int i = 0; i < coefficientsSteel18902Proteus->size(); i++)
            (*coefficientsSteel18902Proteus)[i] = stod(static_cast<std::string>(argv[i + 1]));
    }
    else
    {
        //initializing coefficients array with ,,default" coefficients of Steel 1.8902 Proteus
        (*coefficientsSteel18902Proteus)[0] = 3.1947806;
        (*coefficientsSteel18902Proteus)[1] = 2.1965425;
        (*coefficientsSteel18902Proteus)[2] = 0.;
        (*coefficientsSteel18902Proteus)[3] = 0.99591572;
        (*coefficientsSteel18902Proteus)[4] = 0.;
        (*coefficientsSteel18902Proteus)[5] = 20.332262;
        (*coefficientsSteel18902Proteus)[6] = 693.83826;
        (*coefficientsSteel18902Proteus)[7] = 500.66304;
        (*coefficientsSteel18902Proteus)[8] = 2.1991159;
        (*coefficientsSteel18902Proteus)[9] = 1.412384;
        (*coefficientsSteel18902Proteus)[10] = 2.268138;
        (*coefficientsSteel18902Proteus)[11] = 1.3052699;
        (*coefficientsSteel18902Proteus)[12] = 0.;
        (*coefficientsSteel18902Proteus)[13] = 0.12317212;
        (*coefficientsSteel18902Proteus)[14] = 0.;
        (*coefficientsSteel18902Proteus)[15] = 0.67375298;
        (*coefficientsSteel18902Proteus)[16] = 749.99797;
        (*coefficientsSteel18902Proteus)[17] = 500.04924;
        (*coefficientsSteel18902Proteus)[18] = 2.3327698;
        (*coefficientsSteel18902Proteus)[19] = 0.26616424;
        (*coefficientsSteel18902Proteus)[20] = 5.2098437;
        (*coefficientsSteel18902Proteus)[21] = 1.2696094;
        (*coefficientsSteel18902Proteus)[22] = 0.;
        (*coefficientsSteel18902Proteus)[23] = 0.51804687;
        (*coefficientsSteel18902Proteus)[24] = 564.37636;
        (*coefficientsSteel18902Proteus)[25] = 1.2448726;
        (*coefficientsSteel18902Proteus)[26] = 300.0364;
        (*coefficientsSteel18902Proteus)[27] = 499.94268;
        (*coefficientsSteel18902Proteus)[28] = 1.796028;
        (*coefficientsSteel18902Proteus)[29] = 0.84865546;
        (*coefficientsSteel18902Proteus)[30] = 374.03609;
        (*coefficientsSteel18902Proteus)[31] = 18.407596;
    }

    std::array<double, 4>* ferriteGrainSizeCoefficients = new std::array<double, 4>;
    if (argc >= (coefficientsSteel18902Proteus->size() + ferriteGrainSizeCoefficients->size() + 1))
    {
        for (int i = 0; i < ferriteGrainSizeCoefficients->size(); i++)
            (*ferriteGrainSizeCoefficients)[i] =
                stod(static_cast<std::string>(argv[i + coefficientsSteel18902Proteus->size() + 1]));
    }
    else
    {
        //initializing with ,,default" values
        (*ferriteGrainSizeCoefficients)[0] = 6.;
        (*ferriteGrainSizeCoefficients)[1] = 0.4;
        (*ferriteGrainSizeCoefficients)[2] = 0.2;
        (*ferriteGrainSizeCoefficients)[3] = 0.;
    }

    //initializing with ,,default" values
    //below: coefficients of steel 1.8902 Proteus
    //double c0 = 0.2;
    //double cAlpha = 0.02;
    //double cGammaAlpha0 = 4.8264;
    //double cGammaAlpha1 = -0.005759;
    //double cGammaBeta0 = -0.9988;
    //double cGammaBeta1 = 0.002338;
    //below: coefficients of steel S355J2 (for which optimization is being performed)
    double c0 = 0.12;
    double cAlpha = 0.02;
    double cGammaAlpha0 = 4.677;
    double cGammaAlpha1 = -0.00555;
    double cGammaBeta0 = -1.0387;
    double cGammaBeta1 = 0.0024;

    int noVariablesYetToBeAssigned = argc - (coefficientsSteel18902Proteus->size() + ferriteGrainSizeCoefficients->size() + 1);
    if (noVariablesYetToBeAssigned > 6) noVariablesYetToBeAssigned = 6;

    switch (noVariablesYetToBeAssigned)
    {
    case 6:
        cGammaBeta1 = stod(static_cast<std::string>(
            argv[coefficientsSteel18902Proteus->size() + ferriteGrainSizeCoefficients->size() + 6]));
    case 5:
        cGammaBeta0 = stod(static_cast<std::string>(
            argv[coefficientsSteel18902Proteus->size() + ferriteGrainSizeCoefficients->size() + 5]));
    case 4:
        cGammaAlpha1 = stod(static_cast<std::string>(
            argv[coefficientsSteel18902Proteus->size() + ferriteGrainSizeCoefficients->size() + 4]));
    case 3:
        cGammaAlpha0 = stod(static_cast<std::string>(
            argv[coefficientsSteel18902Proteus->size() + ferriteGrainSizeCoefficients->size() + 3]));
    case 2:
        cAlpha = stod(static_cast<std::string>(
            argv[coefficientsSteel18902Proteus->size() + ferriteGrainSizeCoefficients->size() + 2]));
    case 1:
        c0 = stod(static_cast<std::string>(
            argv[coefficientsSteel18902Proteus->size() + ferriteGrainSizeCoefficients->size() + 1]));
        break;
    default: ;
    }

    steelCoefficients* steelCoefficientsSteel18902Proteus = 
        new steelCoefficients(c0, cAlpha, cGammaAlpha0, cGammaAlpha1, cGammaBeta0, cGammaBeta1);

    std::vector<temperatureSegment>* coolingCurve = new std::vector<temperatureSegment>();
    //temperatureSegment isn't ,,endless", i.e. it will end after given t of seconds (here: 90)
    //it is because later coolingCurve will consist of many temperatureSegment pieces
    //in this test it is so long that the martensite transformation should occur
    coolingCurve->push_back({ 90., 10. });
    TemperatureLoader* temperatures = new TemperatureLoader(coolingCurve, c0, cGammaAlpha0, cGammaAlpha1,
        temperatureLoaderMode::OPTIMIZATION);
    delete coolingCurve;

    int noPoints = 8000;
    HistogramRhoAndD* histogramRhoAndD = new HistogramRhoAndD(noPoints, 17.);

    Cooling* model = new Cooling(temperatures, histogramRhoAndD, coefficientsSteel18902Proteus, ferriteGrainSizeCoefficients, steelCoefficientsSteel18902Proteus);
    //HeatTreatmentResult* result = model->stochasticCoolOptimization_PEARLITE_FRACTION_and_BAINITE_START_and_PEARLITE_END();
    //HeatTreatmentResult* result = model->stochasticCoolOptimization_FERRITE_FRACTION_and_PEARLITE_START();
    //HeatTreatmentResult* result = model->stochasticCoolOptimization_FERRITE_START();
    HeatTreatmentResult* result = model->stochasticCool();

    //result->saveToCSV("detailedResults.csv");
    //result->saveAverageResultValuesToCSV("averageResults.csv");

#ifdef PERFORM_SENSITIVITY_ANALYSIS

    int numberOfBins = 10;
    result->createHistogramsOutOfData(numberOfBins);

    std::vector<sensitivityAnalysisResult> vectorOfSensitivityAnalysisResults;

    for (int i = 0; i < coefficientsToAnalyse.size(); i++)
        vectorOfSensitivityAnalysisResults.push_back(performSensitivityAnalysis(
            model, result, coefficientsSteel18902Proteus, coefficientsToAnalyse[i], sensitivityAnalysisRatio,
            distanceBetweenHistograms::BHATTACHARYYA));

    writeResultOfSensitivityAnalysisToFile(vectorOfSensitivityAnalysisResults, "sensitivityAnalysis.csv");

#endif

    delete temperatures;
    delete histogramRhoAndD;
    delete coefficientsSteel18902Proteus;
    delete ferriteGrainSizeCoefficients;
    delete steelCoefficientsSteel18902Proteus;
    delete model;
    delete result;
}