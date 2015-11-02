#ifndef __julia__
#define __julia__

#include <complex>
#include <DDImage/Op.h>

class Julia {
public:
	Julia();
	~Julia();

	void setupKnobs( DD::Image::Knob_Callback f );
	void fillRow( const int y, const int x, const int r, float* buffer );

private:
	double calcColor( std::complex<double>& z, std::complex<double> c );

private:
	double _width;
	double _height;
	double _zoom;
	double _maxValueExtent;
	double _moveX;
	double _moveY;
	double _cReal;
	double _cImag;
	int _maxIterations;
	int _outputType;
	double _rangedMaskLimit;
};

#endif /* __julia__ */