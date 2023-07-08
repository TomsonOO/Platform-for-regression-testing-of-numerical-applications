#pragma once
#include "histogram.h"

void histogram::createHistogramSimple(std::vector<double> values, int numberOfBins)
{
	numberOfValues = values.size();
	if (numberOfBins == 1) bins.resize(2);		//corner case
	else bins.resize(numberOfBins);
	lowerBinsBoundaries.resize(bins.size());
	std::sort(values.begin(), values.end());
	minValue = values[0];
	maxValue = values[values.size() - 1];

	double widthOfInterval = (maxValue - minValue) / static_cast<double>(bins.size());
	double endingOfInterval = minValue + widthOfInterval;
	int binHeight = 0;
	int valueNumber = 0;
	lowerBinsBoundaries[0] = minValue;
	//TODO - the following loop can be rewritten to look as in other ,,constructors"
	for (int i = 0; i < bins.size(); i++)
	{
		for (valueNumber; valueNumber < numberOfValues; valueNumber++)
		{
			if (values[valueNumber] <= endingOfInterval) binHeight++;
			else break;
		}
		bins[i] = binHeight;
		binHeight = 0;
		if (i != (bins.size() - 1)) lowerBinsBoundaries[i + 1] = endingOfInterval;
		if (i == (bins.size() - 2)) endingOfInterval = maxValue;
		else endingOfInterval += widthOfInterval;
	}
}

void histogram::createHistogramCareAboutOutliers(std::vector<double> values, int numberOfBins)
{
	numberOfValues = values.size();
	if (numberOfBins == 1 || numberOfBins == 2) bins.resize(3);		//caring about outliers with less than 3 bins has no sense
	else bins.resize(numberOfBins);
	lowerBinsBoundaries.resize(bins.size());
	std::sort(values.begin(), values.end());
	minValue = values[0];
	maxValue = values[values.size() - 1];
	
	double outliersBoundaryFraction = (1. / static_cast<double>(bins.size())) / 2.5;
	double widthOfInterval = (maxValue - minValue) / static_cast<double>(bins.size());
	lowerBinsBoundaries[0] = minValue;
	for (int i = 1; i < bins.size(); i++) {
		lowerBinsBoundaries[i] = lowerBinsBoundaries[i-1] + widthOfInterval;
	}
	double leftOutliersBoundary = lowerBinsBoundaries[0];
	int noLeftBinsOutliers = 0;
	double rightOutliersBoundary = lowerBinsBoundaries[bins.size()-1];
	int noRightBinsOutliers = 0;
	for (int i = 1; i < bins.size(); i++) {
		if (values[(int)((double)values.size() * outliersBoundaryFraction)] >= lowerBinsBoundaries[i]) {
			noLeftBinsOutliers++;
		} else break;
	}
	for (int i = (bins.size() - 1); i >= 0; i--) {
		if (values[(int)((double)values.size() * (1. - outliersBoundaryFraction))] < lowerBinsBoundaries[i]) {
			noRightBinsOutliers++;
		} else break;
	}
	int noBinsExcludingOutliers = bins.size();
	if (noLeftBinsOutliers > 0) noBinsExcludingOutliers--;
	if (noRightBinsOutliers > 0) noBinsExcludingOutliers--;
	leftOutliersBoundary = lowerBinsBoundaries[noLeftBinsOutliers];
	if (noRightBinsOutliers > 0) rightOutliersBoundary = lowerBinsBoundaries[bins.size() - noRightBinsOutliers];
	else rightOutliersBoundary = maxValue;

	widthOfInterval = (rightOutliersBoundary - leftOutliersBoundary) / static_cast<double>(noBinsExcludingOutliers);
	lowerBinsBoundaries[0] = minValue;
	if (noLeftBinsOutliers > 0) lowerBinsBoundaries[1] = leftOutliersBoundary;
	else lowerBinsBoundaries[1] = lowerBinsBoundaries[0] + widthOfInterval;
	for (int i = 2; i < bins.size(); i++) {
		lowerBinsBoundaries[i] = lowerBinsBoundaries[i - 1] + widthOfInterval;
	}

	int binHeight = 0;
	int valueNumber = 0;
	for (int i = 0; i < bins.size(); i++)
	{
		if (i < bins.size() - 1) {
			for (valueNumber; valueNumber < numberOfValues; valueNumber++)
			{
				if (values[valueNumber] <= lowerBinsBoundaries[i + 1]) binHeight++;
				else break;
			}
		} else for (valueNumber; valueNumber < numberOfValues; valueNumber++) binHeight++;
		bins[i] = binHeight;
		binHeight = 0;
	}
	lowerBinsBoundaries[0] = 0.;
}

//IMPORTANT NOTE: this constructor will sort input vector
histogram::histogram(std::vector<double> values, int numberOfBins, histogramType hType)
{
	if (hType == histogramType::SIMPLE) createHistogramSimple(values, numberOfBins);
	else if (hType == histogramType::CARE_ABOUT_OUTLIERS) createHistogramCareAboutOutliers(values, numberOfBins);
}

//IMPORTANT NOTE: this constructor will sort input vector
histogram::histogram(std::vector<double> values, histogram* referenceHistogram)
{
	numberOfValues = values.size();
	bins.resize(referenceHistogram->getBins().size());
	lowerBinsBoundaries.resize(referenceHistogram->getBins().size());
	std::sort(values.begin(), values.end());
	minValue = values[0];
	maxValue = values[values.size() - 1];

	for (int i = 0; i < referenceHistogram->getBins().size(); i++)
		lowerBinsBoundaries[i] = referenceHistogram->getLowerBinsBoundaries()[i];

	int binHeight = 0;
	int valueNumber = 0;
	for (int i = 0; i < bins.size(); i++)
	{
		if (i < bins.size() - 1) {
			for (valueNumber; valueNumber < numberOfValues; valueNumber++)
			{
				if (values[valueNumber] <= lowerBinsBoundaries[i + 1]) binHeight++;
				else break;
			}
		}
		else for (valueNumber; valueNumber < numberOfValues; valueNumber++) binHeight++;
		bins[i] = binHeight;
		binHeight = 0;
	}
}

histogram::histogram()
{
	minValue = 0.;
	maxValue = 0.;
	numberOfValues = 0;
	bins = { 0 };
	lowerBinsBoundaries = { 0 };
}

double histogram::getMinValue() { return minValue; }
double histogram::getMaxValue() { return maxValue; }
int histogram::getNumberOfValues() { return numberOfValues; }
std::vector<int> histogram::getBins() { return bins; }
std::vector<double> histogram::getLowerBinsBoundaries() { return lowerBinsBoundaries; }

double computeDistanceBetweenHistograms(histogram first, histogram second, distanceBetweenHistograms distanceType)
{
	if (first.getBins().size() != second.getBins().size()) return -1.;	//input data validation

	double distance = 0.;
	if (distanceType == distanceBetweenHistograms::BHATTACHARYYA)
	{
		double productOfNumberOfPoints = static_cast<double>(first.getNumberOfValues()) *
			static_cast<double>(second.getNumberOfValues());
		for (int i = 0; i < first.getBins().size(); i++)
			distance += sqrt(
			(static_cast<double>(first.getBins()[i]) * static_cast<double>(second.getBins()[i])) / productOfNumberOfPoints);
		return -log10(distance);
	}
	else if (distanceType == distanceBetweenHistograms::EARTH_MOVER)
	{
		double emdi = 0.;
		for (int i = 0; i < first.getBins().size(); i++)
		{
			emdi += (static_cast<double>(first.getBins()[i]) / static_cast<double>(first.getNumberOfValues())) -
			(static_cast<double>(second.getBins()[i]) / static_cast<double>(second.getNumberOfValues()));
			distance += abs(emdi);
		}
		return distance;
	}
	else return -1.;
}