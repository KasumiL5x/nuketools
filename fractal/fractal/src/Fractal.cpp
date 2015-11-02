#include "Fractal.hpp"
#include <DDImage/Knobs.h>
#include "FractalTypes.hpp"

static const char* FRACTAL_CLASS = "Fractal";
static const char* FRACTAL_HELP = "Draws varying procedural fractal patterns.";

static DD::Image::Iop* CreateFractalNode( Node* node ) {
	return new Fractal(node);
}

const DD::Image::Iop::Description Fractal::description(FRACTAL_CLASS, "Patterns/Fractal", CreateFractalNode);

Fractal::Fractal( Node* node )
	: DrawIop(node), _fractalType(0),
		_debugInt1(0), _debugInt2(0), _debugDouble1(0.0), _debugDouble2(0.0) {
}

Fractal::~Fractal() {
}

void Fractal::knobs( DD::Image::Knob_Callback f ) {
	// inputs
	input_knobs(f);
	DD::Image::Enumeration_knob(f, &_fractalType, FractalTypes::strings, "Fractal_Type");
	_mandelbrot.setupKnobs(f);
	_julia.setupKnobs(f);

	// debug stuff
	DD::Image::BeginClosedGroup(f, "Debug_Stuff");
		DD::Image::Int_knob(f, &_debugInt1, "Debug int 1.");
		DD::Image::Int_knob(f, &_debugInt2, "Debug int 2.");
		DD::Image::Float_knob(f, &_debugDouble1, "Debug double 1.");
		DD::Image::Float_knob(f, &_debugDouble2, "Debug double 2.");
	DD::Image::EndGroup(f);

	// outputs
	output_knobs(f);
}

void Fractal::_validate( bool for_real ) {
	DrawIop::_validate(for_real);
}

bool Fractal::draw_engine( int y, int x, int r, float* buffer ) {
	switch( _fractalType ) {
		case 0: {
			_mandelbrot.fillRow(y, x, r, buffer);
			return true;
		}

		case 1: {
			_julia.fillRow(y, x, r, buffer);
			return true;
		}

		default: {
			return false;
		}
	}
}

const char* Fractal::Class() const {
	return FRACTAL_CLASS;
}

const char* Fractal::node_help() const {
	return FRACTAL_HELP;
}