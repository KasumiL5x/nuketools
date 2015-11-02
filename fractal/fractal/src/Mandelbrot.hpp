#ifndef __mandelbrot__
#define __mandelbrot__

#include <cstdlib>
#include <complex>
#include <DDImage/Op.h>

class Mandelbrot {
public:
	Mandelbrot();
	~Mandelbrot();

	void setupKnobs( DD::Image::Knob_Callback f );
	void fillRow( const int y, const int x, const int r, float* buffer );

private:
	double calcMandelbrotColor( std::complex<double> c );

private:
	double _width;
	double _height;
	double _zoom;
	double _maxValueExtent;
	double _moveX;
	double _moveY;
	int _maxIterations;
	int _outputType;
	double _rangedMaskLimit;
};

#endif /* __mandelbrot__ */