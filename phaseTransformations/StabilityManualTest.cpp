#include "StabilityManualTest.h"

bool StabilityManualTest::checkStabilityOfAverageResult(std::vector<std::map<enumTransformationResult, double>>* averageResults, double maxValueDifference,
	enumTransformationResult resultParameter)
{
	bool isAverageResultStable = true;
	for (int i = 0; i < averageResults->size(); i++) {
		for (int j = (i+1); j < averageResults->size(); j++) {
			if (abs((*averageResults)[i][resultParameter] - (*averageResults)[j][resultParameter]) > maxValueDifference) {
				isAverageResultStable = false;
				break;
			}
		}
		if (!isAverageResultStable) break;
	}
	return isAverageResultStable;
}

void StabilityManualTest::checkStability(Cooling* model, std::string _resultDirectory)
{
	int noRuns = 5;
	int noBins = 10;

	std::vector<enumTransformationResult>* vectorEnumTransformationResults = new std::vector<enumTransformationResult>
	{ enumTransformationResult::FERRITE_TEMPERATURE_START, enumTransformationResult::PEARLITE_TEMPERATURE_START,
	enumTransformationResult::PEARLITE_TEMPERATURE_END, enumTransformationResult::BAINITE_TEMPERATURE_START,
	enumTransformationResult::BAINITE_TEMPERATURE_END, enumTransformationResult::MARTENSITE_TEMPERATURE_START,
	enumTransformationResult::FERRITE_VOLUME_FRACTION, enumTransformationResult::PEARLITE_VOLUME_FRACTION,
	enumTransformationResult::BAINITE_VOLUME_FRACTION, enumTransformationResult::MARTENSITE_VOLUME_FRACTION,
	enumTransformationResult::FERRITE_GRAIN_SIZE };

	auto currentTime = std::time(nullptr);
	auto currentLocalTime = *std::localtime(&currentTime);
	std::stringstream ss;
	ss << std::put_time(&currentLocalTime, "%F-%H-%M-%S");
	std::string currentTimeFormatted = ss.str();
	std::string resultDirectory;
	_resultDirectory.find_first_not_of(' ') != std::string::npos ?
		resultDirectory = _resultDirectory + "/" + currentTimeFormatted :
		resultDirectory = currentTimeFormatted;
	std::filesystem::create_directories(resultDirectory);

	std::vector<HeatTreatmentResult*>* results = new std::vector<HeatTreatmentResult*>();
	results->resize(noRuns);
	std::vector<std::map<enumTransformationResult, double>>* averageResults = new std::vector<std::map<enumTransformationResult, double>>();
	averageResults->resize(noRuns);

	//computations
	std::string detailedResultFilenameTemplate = "detailedResult_run";
	for (int i = 0; i < noRuns; i++) {
		(*results)[i] = model->stochasticCool();
		(*results)[i]->saveToCSV(resultDirectory + "/" + detailedResultFilenameTemplate + std::to_string(i + 1) + ".csv");
		for (enumTransformationResult resultParameter : (*vectorEnumTransformationResults)) {
			(*averageResults)[i][resultParameter] = (*results)[i]->countAverageValue(resultParameter);
		}
	}

	//save average results to a file
	std::string averageResultsFilename = resultDirectory + "/averageResult.csv";
	std::ofstream averageResultsFile(averageResultsFilename);
	averageResultsFile << "noRun;Fstart;Pstart;Pend;Bstart;Bend;Mstart;Ferrite Fraction;Pearlite Fraction;Bainite Fraction;Martensite Fraction;Ferrite Grain Size\n";
	for (int i = 0; i < noRuns; i++) {
		averageResultsFile << (i + 1) << ';'
			<< (*averageResults)[i][enumTransformationResult::FERRITE_TEMPERATURE_START] << ';'
			<< (*averageResults)[i][enumTransformationResult::PEARLITE_TEMPERATURE_START] << ';'
			<< (*averageResults)[i][enumTransformationResult::PEARLITE_TEMPERATURE_END] << ';'
			<< (*averageResults)[i][enumTransformationResult::BAINITE_TEMPERATURE_START] << ';'
			<< (*averageResults)[i][enumTransformationResult::BAINITE_TEMPERATURE_END] << ';'
			<< (*averageResults)[i][enumTransformationResult::MARTENSITE_TEMPERATURE_START] << ';'
			<< (*averageResults)[i][enumTransformationResult::FERRITE_VOLUME_FRACTION] << ';'
			<< (*averageResults)[i][enumTransformationResult::PEARLITE_VOLUME_FRACTION] << ';'
			<< (*averageResults)[i][enumTransformationResult::BAINITE_VOLUME_FRACTION] << ';'
			<< (*averageResults)[i][enumTransformationResult::MARTENSITE_VOLUME_FRACTION] << ';'
			<< (*averageResults)[i][enumTransformationResult::FERRITE_GRAIN_SIZE] << '\n';
	}
	averageResultsFile.close();

	//check if average results are OK and save the check results to a file
	double maxValueDifferenceTemperature = 2.;			//TODO: is this value appropriate?
	double maxValueDifferenceVolumeFraction = 0.004;	//TODO: is this value appropriate?
	double maxValueDifferenceFerriteGrainSize = maxValueDifferenceTemperature;	//TODO after taking FGS into account: give distinct value
	std::string stabilityCheckAverageResultsFilename = resultDirectory + "/stabilityCheckAverageResults.csv";
	std::ofstream stabilityCheckAverageResultsFile(stabilityCheckAverageResultsFilename);
	stabilityCheckAverageResultsFile << ";Fstart;Pstart;Pend;Bstart;Bend;Mstart;Ferrite Fraction;Pearlite Fraction;Bainite Fraction;Martensite Fraction;Ferrite Grain Size\n";
	stabilityCheckAverageResultsFile << "Is average result stable? (1 - OK   0 - not OK)" << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceTemperature, enumTransformationResult::FERRITE_TEMPERATURE_START) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceTemperature, enumTransformationResult::PEARLITE_TEMPERATURE_START) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceTemperature, enumTransformationResult::PEARLITE_TEMPERATURE_END) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceTemperature, enumTransformationResult::BAINITE_TEMPERATURE_START) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceTemperature, enumTransformationResult::BAINITE_TEMPERATURE_END) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceTemperature, enumTransformationResult::MARTENSITE_TEMPERATURE_START) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceVolumeFraction, enumTransformationResult::FERRITE_VOLUME_FRACTION) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceVolumeFraction, enumTransformationResult::PEARLITE_VOLUME_FRACTION) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceVolumeFraction, enumTransformationResult::BAINITE_VOLUME_FRACTION) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceVolumeFraction, enumTransformationResult::MARTENSITE_VOLUME_FRACTION) << ';'
		<< checkStabilityOfAverageResult(averageResults, maxValueDifferenceFerriteGrainSize, enumTransformationResult::FERRITE_GRAIN_SIZE) << '\n';
	stabilityCheckAverageResultsFile.close();

	//create R script for creating histograms out of results
	std::string stabilityCheckHistogramsFilename = resultDirectory + "/stabilityCheckHistograms.r";
	std::string rScriptResultDataNameTemplate = "results_run";
	std::ofstream stabilityCheckHistogramsFile(stabilityCheckHistogramsFilename);
	std::string rHistogramTheme = "theme_bw()";
	double rHistogramTitleHJust = 0.5;
	int rHistogramTitleRelSize = 2;
	std::string borderColour = "black";
	std::string ferriteColour = "firebrick2";
	std::string pearliteColour = "darkorange1";
	std::string bainiteColour = "olivedrab1";
	std::string martensiteColour = "darkslategray3";
	std::map<enumTransformationResult, std::string> outputParametersNames;
	outputParametersNames[enumTransformationResult::FERRITE_TEMPERATURE_START] = "Fstart";
	outputParametersNames[enumTransformationResult::PEARLITE_TEMPERATURE_START] = "Pstart";
	outputParametersNames[enumTransformationResult::PEARLITE_TEMPERATURE_END] = "Pend";
	outputParametersNames[enumTransformationResult::BAINITE_TEMPERATURE_START] = "Bstart";
	outputParametersNames[enumTransformationResult::BAINITE_TEMPERATURE_END] = "Bend";
	outputParametersNames[enumTransformationResult::MARTENSITE_TEMPERATURE_START] = "Mstart";
	outputParametersNames[enumTransformationResult::FERRITE_VOLUME_FRACTION] = "Ferrite.Fraction";
	outputParametersNames[enumTransformationResult::PEARLITE_VOLUME_FRACTION] = "Pearlite.Fraction";
	outputParametersNames[enumTransformationResult::BAINITE_VOLUME_FRACTION] = "Bainite.Fraction";
	outputParametersNames[enumTransformationResult::MARTENSITE_VOLUME_FRACTION] = "Martensite.Fraction";
	outputParametersNames[enumTransformationResult::FERRITE_GRAIN_SIZE] = "Ferrite.Grain.Size";
	std::map<enumTransformationResult, std::string> rScriptColourNames;
	rScriptColourNames[enumTransformationResult::FERRITE_TEMPERATURE_START] = ferriteColour;
	rScriptColourNames[enumTransformationResult::PEARLITE_TEMPERATURE_START] = pearliteColour;
	rScriptColourNames[enumTransformationResult::PEARLITE_TEMPERATURE_END] = pearliteColour;
	rScriptColourNames[enumTransformationResult::BAINITE_TEMPERATURE_START] = bainiteColour;
	rScriptColourNames[enumTransformationResult::BAINITE_TEMPERATURE_END] = bainiteColour;
	rScriptColourNames[enumTransformationResult::MARTENSITE_TEMPERATURE_START] = martensiteColour;
	rScriptColourNames[enumTransformationResult::FERRITE_VOLUME_FRACTION] = ferriteColour;
	rScriptColourNames[enumTransformationResult::PEARLITE_VOLUME_FRACTION] = pearliteColour;
	rScriptColourNames[enumTransformationResult::BAINITE_VOLUME_FRACTION] = bainiteColour;
	rScriptColourNames[enumTransformationResult::MARTENSITE_VOLUME_FRACTION] = martensiteColour;
	rScriptColourNames[enumTransformationResult::FERRITE_GRAIN_SIZE] = ferriteColour;
	stabilityCheckHistogramsFile << "#install.packages(\"ggplot2\")   #install this package if not installed\n";
	stabilityCheckHistogramsFile << "library(ggplot2)\n\n";
	stabilityCheckHistogramsFile << "#load detailed results\n";
	for (int i = 0; i < noRuns; i++) {
		stabilityCheckHistogramsFile << rScriptResultDataNameTemplate + std::to_string(i + 1) + " <- read.csv(\""
			+ detailedResultFilenameTemplate + std::to_string(i + 1) + ".csv\")\n";
	}
	stabilityCheckHistogramsFile << "\n#categorize all results by run and put them in one data container\n";
	for (int i = 0; i < noRuns; i++) {
		stabilityCheckHistogramsFile << rScriptResultDataNameTemplate + std::to_string(i + 1) + 
			"$run <- " + std::to_string(i + 1) + "\n";
	}
	stabilityCheckHistogramsFile << "results_combined <- rbind(";
	for (int i = 0; i < (noRuns-1); i++) {
		stabilityCheckHistogramsFile << rScriptResultDataNameTemplate + std::to_string(i + 1) + ", ";
	}
	stabilityCheckHistogramsFile << rScriptResultDataNameTemplate + std::to_string(noRuns) + ")\n";
	stabilityCheckHistogramsFile << "\n#remove redundant data\n";
	for (int i = 0; i < noRuns; i++) {
		stabilityCheckHistogramsFile << "remove(" << rScriptResultDataNameTemplate + std::to_string(i + 1) + ")\n";
	}
	stabilityCheckHistogramsFile << "\n#create histograms for each result parameter for each run\n";
	for (enumTransformationResult resultParameter : (*vectorEnumTransformationResults)) {
		stabilityCheckHistogramsFile << "#" << outputParametersNames[resultParameter] << "\n";
		for (int i = 0; i < noRuns; i++) {
			stabilityCheckHistogramsFile << "ggplot(results_combined[results_combined$run==" << std::to_string(i + 1) << ",],"
				<< "aes(x = " << outputParametersNames[resultParameter] << ")) +\n"
				<< "geom_histogram(bins = " << std::to_string(noBins) << ", fill = \"" << rScriptColourNames[resultParameter] << "\""
				<< ", colour = \"" << borderColour << "\") + " << rHistogramTheme << " + \n"
				<< "labs(x = \"" << outputParametersNames[resultParameter] << " value\", y = \"noOccurences\""
				<< ", title = \"" << outputParametersNames[resultParameter] << " run " << std::to_string(i + 1) << "\") + \n"
				<< "theme(plot.title = element_text(hjust = " << std::to_string(rHistogramTitleHJust)
				<< ", size = rel(" << std::to_string(rHistogramTitleRelSize) << ")))\n";
		}
		stabilityCheckHistogramsFile << "\n";
	}
	stabilityCheckHistogramsFile << "#create histograms to compare results for each run\n";
	for (enumTransformationResult resultParameter : (*vectorEnumTransformationResults)) {
		stabilityCheckHistogramsFile << "#" << outputParametersNames[resultParameter] << "\n"
			<< "ggplot(results_combined,"
			<< "aes(x = " << outputParametersNames[resultParameter] << ", group = run, fill = run)) +\n"
			<< "geom_histogram(position = \"dodge\", bins = " << std::to_string(noBins) << ", colour = \"" << borderColour << "\")"
			<< " + " << rHistogramTheme << " + \n"
			<< "labs(x = \"" << outputParametersNames[resultParameter] << " value\", y = \"noOccurences\""
			<< ", title = \"" << outputParametersNames[resultParameter] << " in each run\") + \n"
			<< "theme(plot.title = element_text(hjust = " << std::to_string(rHistogramTitleHJust)
			<< ", size = rel(" << std::to_string(rHistogramTitleRelSize) << ")))\n";
	}
	stabilityCheckHistogramsFile.close();

	//save histograms distances to a file
	//TODO: possible improvement: set upper limits for histograms distances values and check if computed distances are within them
	//	however, the limit must differ for different noBins values
	(*results)[0]->createHistogramsOutOfData(noBins);
	for (int i = 1; i < results->size(); i++) (*results)[i]->createHistogramsOutOfData((*results)[0]);
	std::string histogramsDistancesFilename = resultDirectory + "/histogramsDistances.csv";
	std::ofstream histogramsDistancesFile(histogramsDistancesFilename);
	std::vector<distanceBetweenHistograms>* vectorDistanceBetweenHistograms = new std::vector<distanceBetweenHistograms>
	{ distanceBetweenHistograms::BHATTACHARYYA, distanceBetweenHistograms::EARTH_MOVER };
	std::map<distanceBetweenHistograms, std::string> distanceBetweenHistogramsNames;
	distanceBetweenHistogramsNames[distanceBetweenHistograms::BHATTACHARYYA] = "Bhattacharyya";
	distanceBetweenHistogramsNames[distanceBetweenHistograms::EARTH_MOVER] = "Earth-Mover's";
	for (distanceBetweenHistograms distanceType : (*vectorDistanceBetweenHistograms)) {
		for (enumTransformationResult resultParameter : (*vectorEnumTransformationResults)) {
			histogramsDistancesFile << distanceBetweenHistogramsNames[distanceType] << " " << outputParametersNames[resultParameter] << ";";
			for (int i = 0; i < noRuns; i++) {
				histogramsDistancesFile << "hist" + std::to_string(i+1) + ";";
			}
			histogramsDistancesFile << "\n";
			for (int i = 0; i < noRuns; i++) {
				histogramsDistancesFile << "hist" + std::to_string(i+1) + ";";
				for (int j = 0; j < noRuns; j++) {
					histogramsDistancesFile << std::to_string(
						computeDistanceBetweenHistograms((*results)[i]->getHistograms()[resultParameter], 
							(*results)[j]->getHistograms()[resultParameter], distanceType)) + ";";
				}
				histogramsDistancesFile << "\n";
			}
			histogramsDistancesFile << "\n";
		}
		histogramsDistancesFile << "\n\n";
	}
	histogramsDistancesFile.close();

	for (HeatTreatmentResult* result : (*results)) delete result;
	delete results;
	delete averageResults;
	delete vectorEnumTransformationResults;
	delete vectorDistanceBetweenHistograms;
}