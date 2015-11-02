#include "Check.hpp"
#include <DDImage/Knobs.h>

static const char* CLASS = "Check";
static const char* HELP = "Makes a badass checkerboard.";

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
	inline T lerp( T p0, T p1, float s ) {
		return p0 + (p1 - p0) * s;
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

	float smoothPulse( float a1, float a2, float b1, float b2, float x ) {
		if ((x < a1) || (x >= b2)) {
			return 0.0F;
		}
		if (x >= a2) {
			if (x < b1) {
				return 1.0F;
			}
			x = (x - b1) / (b2 - b1);
			return 1.0F - x * x * (3.0F - 2.0F * x);
		}
		x = (x - a1) / (a2 - a1);
		return x * x * (3.0F - 2.0F * x);
	}
}

static DD::Image::Iop* CreateFractalNode( Node* node ) {
	return new Check(node);
}

static const DD::Image::CurveDescription lookupCurvesDefaults[] = {
	{ "falloff", "curve x0 0 s1 x1 1 s1" }, // xcoord ycoord tangent; ...
  { 0 }
};

const DD::Image::Iop::Description Check::description(CLASS, "KasumiL5x/Check", CreateFractalNode);

Check::Check( Node* node )
	: DrawIop(node) {
	_scaleX = 8;
	_scaleY = 8;
	_fuzzy = 0;
	_angle = 0.0f;
}

Check::~Check() {
}

void Check::knobs( DD::Image::Knob_Callback f ) {
	// inputs
	input_knobs(f);

	DD::Image::Float_knob(f, &_scaleX, "scalex", "Scale X");
	DD::Image::Float_knob(f, &_scaleY, "scaley", "Scale Y");
	DD::Image::Float_knob(f, &_fuzzy, "fuzzy", "Fuzzy");
	DD::Image::Float_knob(f, &_angle, "angle", "Angle");

	// outputs
	output_knobs(f);
}

void Check::_validate( bool for_real ) {
	DrawIop::_validate(for_real);
}

bool Check::draw_engine( int y, int x, int r, float* buffer ) {

	const float c = cosf(_angle);
	const float s = sinf(_angle);
	const float m00 = c;
	const float m01 = s;
	const float m10 = -s;
	const float m11 = c;

	for( int currX = x; currX < r; ++currX ) {
		const float nx = (m00 * static_cast<float>(currX) + m01 * static_cast<float>(y)) / _scaleX;
		const float ny = (m10 * static_cast<float>(currX) + m11 * static_cast<float>(y)) / _scaleY;
		float f = (int)(nx + 100000.0f) % 2 != (int)(ny + 100000.0f) % 2 ? 1.0f : 0.0f;
		if( _fuzzy != 0 ) {
			const float fuzz = _fuzzy / 100.0f;
			const float fx = ccmath::smoothPulse(0.0f, fuzz, 1.0f - fuzz, 1.0f, fmodf(nx, 1.0f));
			const float fy = ccmath::smoothPulse(0.0f, fuzz, 1.0f - fuzz, 1.0f, fmodf(ny, 1.0f));
			f *= fx * fy;
		}
		const float newColor = ccmath::lerp<float>(0.0f, 1.0f, f);
		buffer[currX] = newColor;
	}
	
	return true;
}

const char* Check::Class() const {
	return CLASS;
}

const char* Check::node_help() const {
	return HELP;
}