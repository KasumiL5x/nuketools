#ifndef __fractal__
#define __fractal__

#include <DDImage/DrawIop.h>
#include "Mandelbrot.hpp"
#include "Julia.hpp"

class Fractal : public DD::Image::DrawIop {
public:
	Fractal( Node* node );
	virtual ~Fractal();

	virtual void knobs( DD::Image::Knob_Callback f ) override;
	virtual void _validate( bool for_real ) override;
	virtual bool draw_engine( int y, int x, int r, float* buffer ) override;
	virtual const char* Class() const override;
	virtual const char* node_help() const override;

public:
	static const DD::Image::Iop::Description description;

private:
	int _fractalType;
	Mandelbrot _mandelbrot;
	Julia _julia;

	// debug stuff
	int _debugInt1;
	int _debugInt2;
	double _debugDouble1;
	double _debugDouble2;
};

#endif /* __fractal__ */