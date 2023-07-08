#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

enum class temperatureLoaderMode {
	SIMULATION,
	OPTIMIZATION
};

//auxiliary struct containing data of single segment from the cooling curve
struct temperatureSegment
{
	double t;		//time of cooling at this segment
	double v;		//velocity of cooling at this segment
};

//auxiliary struct containing data of single point in the temperatures distribution
struct pointTemperature
{
	double time;
	double temperature;
};
//operator overloader for displaying data in pointTemperature
std::ostream& operator<<(std::ostream& OS, const pointTemperature& pt);

class TemperatureLoader
{
	//vector with temperature at each time step
	std::vector<pointTemperature> data;
	void fillData(std::vector<temperatureSegment>* coolingCurve, double T0, temperatureLoaderMode mode);

public:
	//constructor loading data from file - to be implemented later
	TemperatureLoader(std::string filename);
	//constructor computing temperatures according to cooling curve and initial temperature
	//coolingCurve - cooling curve, T0 - initial temperature
	TemperatureLoader(std::vector<temperatureSegment>* coolingCurve, double T0, temperatureLoaderMode mode);
	//constructor computing temperatures according to cooling curve and steel parameters
	//coolingCurve - cooling curve ; c0, cGammaAlpha0, cGammaAlpha1 - steel parameters
	TemperatureLoader(std::vector<temperatureSegment>* coolingCurve, double c0, double cGammaAlpha0, double cGammaAlpha1,
		temperatureLoaderMode mode);

	std::vector<pointTemperature> getData() const;
	double getDeltaTime(int index) const;
	pointTemperature operator[](int a) const;
};

