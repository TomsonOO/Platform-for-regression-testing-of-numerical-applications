#include "HeatTreatmentResult.h"

void HeatTreatmentResult::createHistogram(enumTransformationResult whichField, int numberOfBins, histogramType hType)
{
	std::vector<double>* histogramValues = new std::vector<double>();
	histogramValues->resize(data.size());
	for (int i = 0; i < histogramValues->size(); i++) (*histogramValues)[i] = data[i][whichField];
	histograms[whichField] = histogram((*histogramValues), numberOfBins, hType);
	delete histogramValues;
}

void HeatTreatmentResult::createHistogram(enumTransformationResult whichField, histogram* referenceHistogram)
{
	std::vector<double>* histogramValues = new std::vector<double>();
	histogramValues->resize(data.size());
	for (int i = 0; i < histogramValues->size(); i++) (*histogramValues)[i] = data[i][whichField];
	histograms[whichField] = histogram((*histogramValues), referenceHistogram);
	delete histogramValues;
}

const double HeatTreatmentResult::DOES_NOT_EXIST_SPECIAL_VALUE = -1.;
const double HeatTreatmentResult::ZERO_SPECIAL_VALUE = 0.;

void HeatTreatmentResult::fillWithInitialData(int noPoints)
{
	data.resize(noPoints);
	for (int i = 0; i < noPoints; i++)
		data[i] = { {enumTransformationResult::FERRITE_TEMPERATURE_START, 0.},
		{enumTransformationResult::PEARLITE_TEMPERATURE_START, 0.},
		{enumTransformationResult::PEARLITE_TEMPERATURE_END, 0.},
		{enumTransformationResult::BAINITE_TEMPERATURE_START, 0.},
		{enumTransformationResult::BAINITE_TEMPERATURE_END, 0.},
		{enumTransformationResult::MARTENSITE_TEMPERATURE_START, 0.},
		{enumTransformationResult::FERRITE_VOLUME_FRACTION, 0.},
		{enumTransformationResult::PEARLITE_VOLUME_FRACTION, 0.},
		{enumTransformationResult::BAINITE_VOLUME_FRACTION, 0.},
		{enumTransformationResult::MARTENSITE_VOLUME_FRACTION, 0.},
		{enumTransformationResult::FERRITE_GRAIN_SIZE, 0.} };
}

HeatTreatmentResult::HeatTreatmentResult(int noPoints)
{
	fillWithInitialData(noPoints);
}

HeatTreatmentResult::HeatTreatmentResult(std::string filename)
{
	std::ifstream file(filename);
	if (!file.good()) {
		std::cout << "Failed to open file " << filename << " - returning an empty object\n";
		fillWithInitialData(1);
	} else {
		std::vector<enumTransformationResult>* vectorEnumTransformationResults = new std::vector<enumTransformationResult>
		{ enumTransformationResult::FERRITE_TEMPERATURE_START, enumTransformationResult::PEARLITE_TEMPERATURE_START,
		enumTransformationResult::PEARLITE_TEMPERATURE_END, enumTransformationResult::BAINITE_TEMPERATURE_START,
		enumTransformationResult::BAINITE_TEMPERATURE_END, enumTransformationResult::MARTENSITE_TEMPERATURE_START,
		enumTransformationResult::FERRITE_VOLUME_FRACTION, enumTransformationResult::PEARLITE_VOLUME_FRACTION,
		enumTransformationResult::BAINITE_VOLUME_FRACTION, enumTransformationResult::MARTENSITE_VOLUME_FRACTION,
		enumTransformationResult::FERRITE_GRAIN_SIZE };
		std::map<enumTransformationResult, double> emptyMap;

		std::string line;
		//if the first line of the file contains values names, it must be omitted
		if (!std::isdigit(file.peek())) std::getline(file, line);

		std::getline(file, line);	//get a first line with the values
		char delimiter;
		if (line.find(',') != std::string::npos) delimiter = ',';
		else if (line.find(';') != std::string::npos) delimiter = ';';

		int i = 0;
		double value;
		while (!file.eof()) {
			data.push_back(emptyMap);
			for (int j = 0; j < vectorEnumTransformationResults->size(); j++) {
				value = std::stod(line.substr(0, line.find(delimiter)));
				data[i][(*vectorEnumTransformationResults)[j]] = value;
				line.erase(0, line.find(delimiter) + 1);
			}
			std::getline(file, line);
			i++;
		}

		file.close();
		delete vectorEnumTransformationResults;
	}
}

std::vector<std::map<enumTransformationResult, double>> HeatTreatmentResult::getData() { return data; }

std::map<enumTransformationResult, histogram> HeatTreatmentResult::getHistograms() { return histograms; }

std::map<enumTransformationResult, double> HeatTreatmentResult::operator[](int a) const { return data[a]; }

void HeatTreatmentResult::setData(int noPoint, enumTransformationResult whichField, double value)
{
	data[noPoint][whichField] = value;
}

double HeatTreatmentResult::countAverageValue(enumTransformationResult whichField)
{
	double minBoundaryFractionOfAppearances = 0.5;		//later: modify this value?
	int minNumberOfAppearances = static_cast<int>(static_cast<double>(data.size())* minBoundaryFractionOfAppearances);

	double sum = 0.;
	int numberOfAppearances = 0;
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i][whichField] != 0.)	//(for temperatures) can real temperature in this process be 0.?
		{
			numberOfAppearances++;
			sum += data[i][whichField];
		}
	}

	if (whichField == enumTransformationResult::FERRITE_TEMPERATURE_START ||
		whichField == enumTransformationResult::PEARLITE_TEMPERATURE_START ||
		whichField == enumTransformationResult::PEARLITE_TEMPERATURE_END ||
		whichField == enumTransformationResult::BAINITE_TEMPERATURE_START ||
		whichField == enumTransformationResult::BAINITE_TEMPERATURE_END ||
		whichField == enumTransformationResult::MARTENSITE_TEMPERATURE_START ||
		whichField == enumTransformationResult::FERRITE_GRAIN_SIZE)
	{
		if (numberOfAppearances < minNumberOfAppearances) return DOES_NOT_EXIST_SPECIAL_VALUE;
		else return sum / static_cast<double>(numberOfAppearances);
	}
	else
	{
		if (numberOfAppearances < minNumberOfAppearances) return ZERO_SPECIAL_VALUE;
		else return sum / data.size();
	}
}

void HeatTreatmentResult::createHistogramsOutOfData(int numberOfBins, histogramType hType)
{
	std::vector<enumTransformationResult>* vectorEnumTransformationResults = new std::vector<enumTransformationResult>
	{ enumTransformationResult::FERRITE_TEMPERATURE_START, enumTransformationResult::PEARLITE_TEMPERATURE_START,
	enumTransformationResult::PEARLITE_TEMPERATURE_END, enumTransformationResult::BAINITE_TEMPERATURE_START,
	enumTransformationResult::BAINITE_TEMPERATURE_END, enumTransformationResult::MARTENSITE_TEMPERATURE_START,
	enumTransformationResult::FERRITE_VOLUME_FRACTION, enumTransformationResult::PEARLITE_VOLUME_FRACTION,
	enumTransformationResult::BAINITE_VOLUME_FRACTION, enumTransformationResult::MARTENSITE_VOLUME_FRACTION,
	enumTransformationResult::FERRITE_GRAIN_SIZE };

	for (int i = 0; i < vectorEnumTransformationResults->size(); i++)
		createHistogram((*vectorEnumTransformationResults)[i], numberOfBins, hType);

	delete vectorEnumTransformationResults;
}

void HeatTreatmentResult::createHistogramsOutOfData(HeatTreatmentResult* referenceResult)
{
	std::vector<enumTransformationResult>* vectorEnumTransformationResults = new std::vector<enumTransformationResult>
	{ enumTransformationResult::FERRITE_TEMPERATURE_START, enumTransformationResult::PEARLITE_TEMPERATURE_START,
	enumTransformationResult::PEARLITE_TEMPERATURE_END, enumTransformationResult::BAINITE_TEMPERATURE_START,
	enumTransformationResult::BAINITE_TEMPERATURE_END, enumTransformationResult::MARTENSITE_TEMPERATURE_START,
	enumTransformationResult::FERRITE_VOLUME_FRACTION, enumTransformationResult::PEARLITE_VOLUME_FRACTION,
	enumTransformationResult::BAINITE_VOLUME_FRACTION, enumTransformationResult::MARTENSITE_VOLUME_FRACTION,
	enumTransformationResult::FERRITE_GRAIN_SIZE };

	for (int i = 0; i < vectorEnumTransformationResults->size(); i++)
		createHistogram((*vectorEnumTransformationResults)[i], &(referenceResult->getHistograms()[(*vectorEnumTransformationResults)[i]]));

	delete vectorEnumTransformationResults;
}

void HeatTreatmentResult::saveToCSV(std::string filename)
{
	std::ofstream file(filename);
	file << "Fstart,Pstart,Pend,Bstart,Bend,Mstart,Ferrite Fraction,Pearlite Fraction,Bainite Fraction,Martensite Fraction,Ferrite Grain Size\n";
	for (int i = 0; i < data.size(); i++)
	{
		file << data[i][enumTransformationResult::FERRITE_TEMPERATURE_START] << ','
			<< data[i][enumTransformationResult::PEARLITE_TEMPERATURE_START] << ','
			<< data[i][enumTransformationResult::PEARLITE_TEMPERATURE_END] << ','
			<< data[i][enumTransformationResult::BAINITE_TEMPERATURE_START] << ','
			<< data[i][enumTransformationResult::BAINITE_TEMPERATURE_END] << ','
			<< data[i][enumTransformationResult::MARTENSITE_TEMPERATURE_START] << ','
			<< data[i][enumTransformationResult::FERRITE_VOLUME_FRACTION] << ','
			<< data[i][enumTransformationResult::PEARLITE_VOLUME_FRACTION] << ','
			<< data[i][enumTransformationResult::BAINITE_VOLUME_FRACTION] << ','
			<< data[i][enumTransformationResult::MARTENSITE_VOLUME_FRACTION] << ','
			<< data[i][enumTransformationResult::FERRITE_GRAIN_SIZE] << '\n';
	}
	file.close();
}

void HeatTreatmentResult::saveAverageResultValuesToCSV(std::string filename)
{
	std::ofstream file(filename);
	file << "Fstart;Pstart;Pend;Bstart;Bend;Mstart;Ferrite Fraction;Pearlite Fraction;Bainite Fraction;Martensite Fraction;Ferrite Grain Size\n";
	file << countAverageValue(enumTransformationResult::FERRITE_TEMPERATURE_START) << ';'
		<< countAverageValue(enumTransformationResult::PEARLITE_TEMPERATURE_START) << ';'
		<< countAverageValue(enumTransformationResult::PEARLITE_TEMPERATURE_END) << ';'
		<< countAverageValue(enumTransformationResult::BAINITE_TEMPERATURE_START) << ';'
		<< countAverageValue(enumTransformationResult::BAINITE_TEMPERATURE_END) << ';'
		<< countAverageValue(enumTransformationResult::MARTENSITE_TEMPERATURE_START) << ';'
		<< countAverageValue(enumTransformationResult::FERRITE_VOLUME_FRACTION) << ';'
		<< countAverageValue(enumTransformationResult::PEARLITE_VOLUME_FRACTION) << ';'
		<< countAverageValue(enumTransformationResult::BAINITE_VOLUME_FRACTION) << ';'
		<< countAverageValue(enumTransformationResult::MARTENSITE_VOLUME_FRACTION) << ';'
		<< countAverageValue(enumTransformationResult::FERRITE_GRAIN_SIZE);
	file.close();
}