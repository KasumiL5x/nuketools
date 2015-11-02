#include "Gradient.hpp"
#include <DDImage/Knobs.h>

static const char* CLASS = "Gradient";
static const char* HELP = "Makes a gradient mask.";

namespace ccmath {
template<typename T>
	inline T minimum( T a, T b ) {
		return (a >= b) ? b : a;
	}

	template<typename T>
	inline T maximum( T a, T b ) {
		return (a <= b) ? b : a;
	}

	template<typename T>
	inline T saturate( T value ) {
		return maximum<T>(0.0, minimum<T>(1.0, value));
	}

	template<typename T>
	inline T smoothstep( T p0, T p1, float s ) {
		T t = saturate<T>((s-p0)/(p1-p0));
		return t*t*(static_cast<T>(3.0)-(static_cast<T>(2.0)*t));
	}

	template<typename T>
	inline T percent( T min, T max, T val ) {
		return (val - min) / (max - min);
	}

	template<typename T>
	inline T clamp( T val, T min, T max ) {
		if( val < min ) {
			return min;
		}

		if( val > max ) {
			return max;
		}

		return val;
	}
}

static DD::Image::Iop* CreateFractalNode( Node* node ) {
	return new Gradient(node);
}

static const DD::Image::CurveDescription lookupCurvesDefaults[] = {
	{ "falloff", "curve x0 0 s1 x1 1 s1" }, // xcoord ycoord tangent; ...
  { 0 }
};

const DD::Image::Iop::Description Gradient::description(CLASS, "KasumiL5x/Gradient", CreateFractalNode);

Gradient::Gradient( Node* node )
	: DrawIop(node), _radius(1000.0f), _lookupCurve(lookupCurvesDefaults) {
	_position[0] = 1024.0f;
	_position[1] = 768.0f;
}

Gradient::~Gradient() {
}

void Gradient::knobs( DD::Image::Knob_Callback f ) {
	// inputs
	input_knobs(f);

	DD::Image::XY_knob(f, &_position[0], "position", "Position");
	DD::Image::Float_knob(f, &_radius, "radius", "Radius");
	DD::Image::Newline(f);
	DD::Image::Bool_knob(f, &_invert, "invert", "Invert result");
	DD::Image::LookupCurves_knob(f, &_lookupCurve, "curve");

	// outputs
	output_knobs(f);
}

void Gradient::_validate( bool for_real ) {
	DrawIop::_validate(for_real);
}

bool Gradient::draw_engine( int y, int x, int r, float* buffer ) {
	const float GRADIENT_DISTANCE = _radius;
	const DD::Image::Vector2 GRADIENT_CENTER = DD::Image::Vector2(_position[0], _position[1]);

	for( int currX = x; currX < r; ++currX ) {
		// current position of pixel
		const DD::Image::Vector2 pixel = DD::Image::Vector2(static_cast<float>(currX), static_cast<float>(y));
		// vector between gradient center and pixel
		const DD::Image::Vector2 diff = (pixel - GRADIENT_CENTER);
		// distance between gradient center and pixel
		const float length = diff.length();
		// percentage the distance is relative to the gradient distance (todo: clamp or not?)
		const float thePercent = ccmath::percent<float>(0.0f, GRADIENT_DISTANCE, length);
		// weight from user curve
		const float weight = _lookupCurve.getValue(0, thePercent); // curves can be negative; todo: clamp?
		if( _invert ) {
			buffer[currX] = ccmath::clamp<float>(1.0f - weight, 0.0f, 1.0f);
		} else {
			buffer[currX] = ccmath::clamp<float>(weight, 0.0f, 1.0f);
		}
	}
	
	return true;
}

const char* Gradient::Class() const {
	return CLASS;
}

const char* Gradient::node_help() const {
	return HELP;
}