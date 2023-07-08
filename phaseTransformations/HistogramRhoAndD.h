#pragma once
#include <vector>
#include <random>
#include <iostream>

//auxiliary struct
struct pointCooling
{
	double rho;		//density
	double D;		//grain size
};
//operator overloader for displaying data in pointCooling
std::ostream& operator<<(std::ostream& OS, const pointCooling& pc);

//HistogramRhoAndD is the input to the Cooling Model, but in final version input to the Cooling Model will be the output
//from the Hot Deformation Model, so most probably this whole class will be deleted later
class HistogramRhoAndD
{
	std::vector<pointCooling> data;

public:
	//constructor filling data vector with given values
	HistogramRhoAndD(int size, double D = 33., double rho = 1000.);

	std::vector<pointCooling> getData();
	pointCooling operator[](int a) const;
};