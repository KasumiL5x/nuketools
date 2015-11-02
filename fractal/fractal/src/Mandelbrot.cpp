#include "Mandelbrot.hpp"
#include <DDImage/Knobs.h>

static const char* MandelbrotOutputStrings[] = {
	"Smooth",
	"Sharp",
	"Ranged",
	0
};

Mandelbrot::Mandelbrot()
	: _width(2048.0), _height(1556.0), _zoom(1.0), _maxValueExtent(2.0), _moveX(-0.5), _moveY(0.0), _maxIterations(300), _rangedMaskLimit(0.5) {
}

Mandelbrot::~Mandelbrot() {
}

void Mandelbrot::setupKnobs( DD::Image::Knob_Callback f ) {
	DD::Image::BeginGroup(f, "Mandelbrot_Settings");
		DD::Image::Float_knob(f, &_width, "Fractal_Width");
		DD::Image::Float_knob(f, &_height, "Fractal_Height");
		DD::Image::Int_knob(f, &_maxIterations, "Maximum_Iterations.");
		DD::Image::Float_knob(f, &_zoom, "Fractal_Zoom");
		DD::Image::Float_knob(f, &_maxValueExtent, "Max_Value_Extent");
		DD::Image::Float_knob(f, &_moveX, "Move_X");
		DD::Image::Float_knob(f, &_moveY, "Move_Y");
		DD::Image::BeginGroup(f, "Output");
			DD::Image::Enumeration_knob(f, &_outputType, MandelbrotOutputStrings, "Output Type");
			DD::Image::Tooltip(f, "Smooth will shade the fractal with 0.0 and interpolate towards 1.0 for areas approaching the edge of the fractal.\n"
				"Sharp will shade the fractal with 0.0 and anything outside of the fractal with 1.0.\n"
				"Ranged outputs 1.0 if the final number of iterations divided by the maximum number of iterations is less than a given range, and outputs 1.0 otherwise.");
			DD::Image::Float_knob(f, &_rangedMaskLimit, "Range");
		DD::Image::EndGroup(f);
	DD::Image::EndGroup(f);
}

void Mandelbrot::fillRow( const int y, const int x, const int r, float* buffer ) {
	const double scale = _zoom * _maxValueExtent / std::min<double>(_width, _height);

	const double dy = (static_cast<double>(_height) / 2.0 - static_cast<double>(y)) * scale + _moveY;
	for( int currX = x; currX < r; ++currX ) {
		const double dx = (static_cast<double>(currX) - static_cast<double>(_width) / 2.0) * scale + _moveX;
		const double color = calcMandelbrotColor(std::complex<double>(dx, dy));
		buffer[currX] = static_cast<float>(color);
	}
}

double Mandelbrot::calcMandelbrotColor( std::complex<double> c ) {
	const double MAX_NORM = _maxValueExtent * _maxValueExtent;

	int iteration = 0;
	std::complex<double> z;
	do {
		z = z * z + c;
		iteration++;
	} while( std::norm(z) < MAX_NORM && iteration < _maxIterations );

	switch( _outputType ) {
		case 0: {
			return (iteration < _maxIterations) ? static_cast<double>(iteration) / static_cast<double>(_maxIterations) : 0.0;
		}

		case 1: {
			return (iteration == _maxIterations) ? 0.0 : 1.0;
		}

		case 2: {
			return (static_cast<double>(iteration) / static_cast<double>(_maxIterations)) < _rangedMaskLimit ? 1.0 : 0.0;
		}
	}
}