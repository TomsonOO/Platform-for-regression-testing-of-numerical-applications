#include "TemperatureLoader.h"

std::ostream& operator<<(std::ostream& OS, const pointTemperature& pt)
{
	OS << "t = " << pt.time << " , T = " << pt.temperature;
	return OS;
}

//to be implemented later
TemperatureLoader::TemperatureLoader(std::string filename)
{
	throw "Not implemented yet!";
}

void TemperatureLoader::fillData(std::vector<temperatureSegment>* coolingCurve, double T0, temperatureLoaderMode mode)
{
	double temperature = T0;
	double dT;									//delta temperature
	double time = 0.;
	double dt;									//delta time
	double maxTemperatureChangePerStep;
	double maxTimeChangePerStep;
	if (mode == temperatureLoaderMode::SIMULATION) {
		maxTemperatureChangePerStep = 0.1;	//TODO after identification - check if simulation values are ,,the best"
		maxTimeChangePerStep = 0.5;
	}
	else if (mode == temperatureLoaderMode::OPTIMIZATION) {
		maxTemperatureChangePerStep = 0.5;
		maxTimeChangePerStep = 0.95;
	}
	int noSteps;
	bool lowerTemperaturesAreNotNeeded = false;
	double valueOfReason = 100.;			//during optimization we won't want to consider too low temperatures

	//assigning first value in the way to have T0 as first temperature
	data.resize(1);
	data[0] = { 0., temperature };

	//iterate through all segments on cooling curve
	for (int i = 0; i < coolingCurve->size(); i++)
	{
		if (lowerTemperaturesAreNotNeeded) break;
		//compute number of steps, delta time and delta temperature at each segment
		dt = maxTemperatureChangePerStep / (*coolingCurve)[i].v;
		//dt can't be too great - if it happens, maxTimeChangePerStep is taken instead
		dt = std::min(dt, maxTimeChangePerStep);
		noSteps = ceil((*coolingCurve)[i].t / dt);
		//dt must depend on noSteps, not the opposite - otherwise the result might contain one incorrect value (imagine e.g. t=35, dt=10)
		dt = (*coolingCurve)[i].t / static_cast<double>(noSteps);
		dT = ((*coolingCurve)[i].t * (*coolingCurve)[i].v) / static_cast<double>(noSteps);
		//enlarge size of vector by number of steps
		data.resize(data.size() + noSteps);
		//assign values in data vector
		for (int j = data.size() - noSteps; j < data.size(); j++)
		{
			time += dt;
			temperature -= dT;
			data[j] = { time, temperature };
			if (mode == temperatureLoaderMode::OPTIMIZATION && temperature < valueOfReason) {
				data.resize(j + 1);
				lowerTemperaturesAreNotNeeded = true;
				break;
			}
		}
	}
}

TemperatureLoader::TemperatureLoader(std::vector<temperatureSegment>* coolingCurve, double T0, temperatureLoaderMode mode)
{
	fillData(coolingCurve, T0, mode);
}

TemperatureLoader::TemperatureLoader(std::vector<temperatureSegment>* coolingCurve, double c0, double cGammaAlpha0,
	double cGammaAlpha1, temperatureLoaderMode mode)
{
	double Ae3 = ((c0 - cGammaAlpha0) / cGammaAlpha1);
	double noDegreesAboveAe3 = 5.;
	double T0 = Ae3 + noDegreesAboveAe3;
	fillData(coolingCurve, T0, mode);
}

std::vector<pointTemperature> TemperatureLoader::getData() const { return data; }
double TemperatureLoader::getDeltaTime(int index) const
{
	if (index == 0) return data[1].time;
	else return data[index].time - data[index - 1].time;
}
pointTemperature TemperatureLoader::operator[](int a) const { return data[a]; }