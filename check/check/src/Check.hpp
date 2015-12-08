#ifndef __gradient__
#define __gradient__

#include <DDImage/DrawIop.h>
#include <DDImage/LookupCurves.h>

class Check : public DD::Image::DrawIop {
public:
	Check( Node* node );
	virtual ~Check();

	virtual void knobs( DD::Image::Knob_Callback f ) override;
	virtual void _validate( bool for_real ) override;
	virtual bool draw_engine( int y, int x, int r, float* buffer ) override;
	virtual const char* Class() const override;
	virtual const char* node_help() const override;

public:
	static const DD::Image::Iop::Description description;

private:
	float _scaleX;
	float _scaleY;
	float _fuzzy;
	float _angle;
	float _offsetX;
	float _offsetY;
	float _rotationCenter[2];
};

#endif /* __gradient__ */