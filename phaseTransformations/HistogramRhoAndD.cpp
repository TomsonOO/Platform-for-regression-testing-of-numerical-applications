#include "HistogramRhoAndD.h"

std::ostream& operator<<(std::ostream& OS, const pointCooling& pc)
{
	OS << "rho = " << pc.rho << " , D = " << pc.D;
	return OS;
}

HistogramRhoAndD::HistogramRhoAndD(int size, double D, double rho)
{
	data.resize(size);
	for (int i = 0; i < size; i++) data[i] = { rho, D };
}

std::vector<pointCooling> HistogramRhoAndD::getData() { return data; }
pointCooling HistogramRhoAndD::operator[](int a) const { return data[a]; }