#ifndef __fractal_types__
#define __fractal_types__
 
struct FractalTypes {
	static const char* strings[];
};

const char* FractalTypes::strings[] = {
	"Mandelbrot",
	"Julia",
	0
};

#endif /* __fractal_types__ */