#ifndef __gradient__
#define __gradient__

#include <DDImage/DrawIop.h>
#include <DDImage/LookupCurves.h>

class Gradient : public DD::Image::DrawIop {
public:
	Gradient( Node* node );
	virtual ~Gradient();

	virtual void knobs( DD::Image::Knob_Callback f ) override;
	virtual void _validate( bool for_real ) override;
	virtual bool draw_engine( int y, int x, int r, float* buffer ) override;
	virtual const char* Class() const override;
	virtual const char* node_help() const override;

public:
	static const DD::Image::Iop::Description description;

private:
	float _position[2];
	float _radius;
	DD::Image::LookupCurves _lookupCurve;
	bool _invert;
};

#endif /* __gradient__ */