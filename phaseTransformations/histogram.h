#pragma once
#include <vector>
#include <algorithm>
#include <cmath>

enum class histogramType
{
	SIMPLE,
	CARE_ABOUT_OUTLIERS
};

enum class distanceBetweenHistograms
{
	BHATTACHARYYA,
	EARTH_MOVER
};

struct histogram
{
private:

	double minValue;
	double maxValue;
	int numberOfValues;
	std::vector<int> bins;		//value of each bin is its height
	std::vector<double> lowerBinsBoundaries;

	//TODO - check if in these methods there are some mutual parts and if they can be extracted to the constructor
	void createHistogramSimple(std::vector<double> values, int numberOfBins);
	void createHistogramCareAboutOutliers(std::vector<double> values, int numberOfBins);

public:

	//IMPORTANT NOTE: this constructor will sort input vector
	histogram(std::vector<double> values, int numberOfBins, histogramType hType = histogramType::CARE_ABOUT_OUTLIERS);
	//IMPORTANT NOTE: this constructor will sort input vector
	histogram(std::vector<double> values, histogram* referenceHistogram);
	histogram();

	//getters
	double getMinValue();
	double getMaxValue();
	int getNumberOfValues();
	std::vector<int> getBins();
	std::vector<double> getLowerBinsBoundaries();
};

double computeDistanceBetweenHistograms(histogram first, histogram second, distanceBetweenHistograms distanceType);