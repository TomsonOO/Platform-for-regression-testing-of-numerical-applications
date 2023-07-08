#include "sensitivityAnalysis.h"

sensitivityAnalysisResult::sensitivityAnalysisResult(int _coefficientNumber,
    std::map<enumTransformationResult, double> _sensitivityFactorsParameters,
    std::map<enumTransformationResult, double> _sensitivityFactorsHistograms) :
    coefficientNumber(_coefficientNumber), sensitivityFactorsParameters(_sensitivityFactorsParameters), 
    sensitivityFactorsHistograms(_sensitivityFactorsHistograms) {}

bool operator== (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2)
{
    return sar1.coefficientNumber == sar2.coefficientNumber;
}
bool operator!= (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2)
{
    return sar1.coefficientNumber != sar2.coefficientNumber;
}
bool operator< (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2)
{
    return sar1.coefficientNumber < sar2.coefficientNumber;
}
bool operator> (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2)
{
    return sar1.coefficientNumber > sar2.coefficientNumber;
}
bool operator<= (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2)
{
    return sar1.coefficientNumber <= sar2.coefficientNumber;
}
bool operator>= (const sensitivityAnalysisResult& sar1, const sensitivityAnalysisResult& sar2)
{
    return sar1.coefficientNumber >= sar2.coefficientNumber;
}

double computeSensitivityFactor(double coefficient, double deltaCoefficient, double baseY, double analysedY)
{
    return ((coefficient / deltaCoefficient) * ((analysedY - baseY) / baseY));
}

void writeResultOfSensitivityAnalysisToFile(std::vector<sensitivityAnalysisResult> vectorOfResults, std::string filename)
{
    std::vector<enumTransformationResult>* vectorEnumTransformationResults = new std::vector<enumTransformationResult>
    { enumTransformationResult::FERRITE_TEMPERATURE_START, enumTransformationResult::PEARLITE_TEMPERATURE_START,
    enumTransformationResult::PEARLITE_TEMPERATURE_END, enumTransformationResult::BAINITE_TEMPERATURE_START,
    enumTransformationResult::BAINITE_TEMPERATURE_END, enumTransformationResult::MARTENSITE_TEMPERATURE_START,
    enumTransformationResult::FERRITE_VOLUME_FRACTION, enumTransformationResult::PEARLITE_VOLUME_FRACTION,
    enumTransformationResult::BAINITE_VOLUME_FRACTION, enumTransformationResult::MARTENSITE_VOLUME_FRACTION,
    enumTransformationResult::FERRITE_GRAIN_SIZE };

    std::sort(vectorOfResults.begin(), vectorOfResults.end());

    std::ofstream file(filename);
    file << "coefficient;Fstart factor;Pstart factor;Pend factor;Bstart factor;Bend factor;Mstart factor;Ferrite Fraction factor;"
        << "Pearlite Fraction factor;Bainite Fraction factor;Martensite Fraction factor;Ferrite Grain Size factor;"
        << "Fstart histogram factor;Pstart histogram factor;Pend histogram factor;Bstart histogram factor;Bend histogram factor;"
        << "Mstart histogram factor;Ferrite Fraction histogram factor;Pearlite Fraction histogram factor;"
        << "Bainite Fraction histogram factor;Martensite Fraction histogram factor;Ferrite Grain Size histogram factor\n";

    for (int i = 0; i < vectorOfResults.size(); i++)
    {
        file << "a" << vectorOfResults[i].coefficientNumber << ";";
        for (int j = 0; j < vectorEnumTransformationResults->size(); j++)
            file << vectorOfResults[i].sensitivityFactorsParameters[(*vectorEnumTransformationResults)[j]] << ";";
        for (int j = 0; j < vectorEnumTransformationResults->size(); j++)
            file << vectorOfResults[i].sensitivityFactorsHistograms[(*vectorEnumTransformationResults)[j]] << ";";
        file << "\n";
    }

    file.close();
    delete vectorEnumTransformationResults;
}

sensitivityAnalysisResult performSensitivityAnalysis(Cooling* model, HeatTreatmentResult* baseResult, std::array<double, 32>* coefficients,
	int coefficientNumber, double sensitivityAnalysisRatio, distanceBetweenHistograms distanceBetweenHistogramsType)
{
    coefficientNumber--;    //because of 1-based indexing
    std::vector<enumTransformationResult>* vectorEnumTransformationResults = new std::vector<enumTransformationResult>
    { enumTransformationResult::FERRITE_TEMPERATURE_START, enumTransformationResult::PEARLITE_TEMPERATURE_START,
    enumTransformationResult::PEARLITE_TEMPERATURE_END, enumTransformationResult::BAINITE_TEMPERATURE_START,
    enumTransformationResult::BAINITE_TEMPERATURE_END, enumTransformationResult::MARTENSITE_TEMPERATURE_START,
    enumTransformationResult::FERRITE_VOLUME_FRACTION, enumTransformationResult::PEARLITE_VOLUME_FRACTION,
    enumTransformationResult::BAINITE_VOLUME_FRACTION, enumTransformationResult::MARTENSITE_VOLUME_FRACTION,
    enumTransformationResult::FERRITE_GRAIN_SIZE };

    std::map<enumTransformationResult, double> sensitivityFactorsParameters;
    std::map<enumTransformationResult, double> sensitivityFactorsHistograms;

    if (coefficientNumber > coefficients->size() || coefficientNumber < 0)
    {
        ;       //throw an exception?
    }

    if ((*coefficients)[coefficientNumber] == 0.)   //it's impossible to make sensitivity analysis for coefficient which equals 0
    {
        for (int i = 0; i < vectorEnumTransformationResults->size(); i++)
        {
            sensitivityFactorsParameters[(*vectorEnumTransformationResults)[i]] = 0.;
            sensitivityFactorsHistograms[(*vectorEnumTransformationResults)[i]] = 0.;
        }
        delete vectorEnumTransformationResults;
        coefficientNumber++;        //because of 1-based indexing
        sensitivityAnalysisResult result = sensitivityAnalysisResult(
            coefficientNumber, sensitivityFactorsParameters, sensitivityFactorsHistograms);
        return result;
    }

    double coefficientOldValue = (*coefficients)[coefficientNumber];
    double deltaCoefficient = sensitivityAnalysisRatio * (*coefficients)[coefficientNumber];
    (*coefficients)[coefficientNumber] += deltaCoefficient;

    HeatTreatmentResult* analysedResult = model->stochasticCool();
    //we have to check only one value because ,,histogram-creating" public method of HeatTreatmentResult makes them all
    if (baseResult->getHistograms()[enumTransformationResult::FERRITE_TEMPERATURE_START].getNumberOfValues() == 0)
        baseResult->createHistogramsOutOfData(10);
    analysedResult->createHistogramsOutOfData(baseResult);

    for (int i = 0; i < vectorEnumTransformationResults->size(); i++)
    {
        sensitivityFactorsParameters[(*vectorEnumTransformationResults)[i]] =
            computeSensitivityFactor(coefficientOldValue, deltaCoefficient,
                baseResult->countAverageValue((*vectorEnumTransformationResults)[i]),
                analysedResult->countAverageValue((*vectorEnumTransformationResults)[i]));
        sensitivityFactorsHistograms[(*vectorEnumTransformationResults)[i]] =
            ((coefficientOldValue / deltaCoefficient) * computeDistanceBetweenHistograms(
                baseResult->getHistograms()[(*vectorEnumTransformationResults)[i]],
                analysedResult->getHistograms()[(*vectorEnumTransformationResults)[i]],
                distanceBetweenHistogramsType));
    }

    (*coefficients)[coefficientNumber] = coefficientOldValue;

    coefficientNumber++;        //because of 1-based indexing
    sensitivityAnalysisResult result = sensitivityAnalysisResult(
        coefficientNumber, sensitivityFactorsParameters, sensitivityFactorsHistograms);

    delete vectorEnumTransformationResults;
    delete analysedResult;

    return result;
}