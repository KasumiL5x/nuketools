#include "Bumpy.hpp"
#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/Tile.h>

static const char* FILTER_TYPES[] = {
	"Sobel 3x3",
	"Scharr 3x3",
	"Prewitt 3x3",
	0
};

static const char* CLASS = "Bumpy";
static const char* HELP = "Bumps.";

static DD::Image::Iop* build( Node* node ) {
	return new Bumpy(node);
}

const DD::Image::Iop::Description Bumpy::description(CLASS, "KasumiL5x/Bumpy", build);

Bumpy::Bumpy( Node* node )
	: Iop(node) {
	_sourceChannel = DD::Image::Chan_Red;
	for( int i = 0; i < 3; ++i ) {
		_normalChannels[i] = DD::Image::Channel(DD::Image::Chan_Red + i);
	}
	_strength = 1.0;
	_invertX = false;
	_invertY = false;
	_invertZ = false;
	_normalize = false;
}

Bumpy::~Bumpy() {
}

void Bumpy::knobs( DD::Image::Knob_Callback f ) {
	DD::Image::Input_Channel_knob(f, &_sourceChannel, 1, 0, "source_channel", "Source Channel");
	DD::Image::Tooltip(f, "Which channel to use in the filter.");
	DD::Image::Newline(f);

	DD::Image::Channel_knob(f, _normalChannels, 3, "normal", "Output Channels");
	DD::Image::Tooltip(f, "Which channels to output the XYZ normal to.");
	DD::Image::Newline(f);

	DD::Image::Float_knob(f, &_strength, DD::Image::IRange(0.1, 100.0), "strength", "Strength");
	DD::Image::Tooltip(f, "Intensity of the filtering.");
	DD::Image::Newline(f);

	DD::Image::Bool_knob(f, &_invertX, "invert_x", "Invert X");
	DD::Image::Bool_knob(f, &_invertY, "invert_y", "Invert Y");
	DD::Image::Bool_knob(f, &_invertZ, "invert_z", "Invert Z");
	DD::Image::Newline(f);

	DD::Image::Bool_knob(f, &_normalize, "normalize", "Normalize");
	DD::Image::Tooltip(f, "Normalizes result into a [0,1] range.");

	DD::Image::Newline(f);

	DD::Image::Enumeration_knob(f, &_filterType, FILTER_TYPES, "filter", "Filter");
	DD::Image::Tooltip(f, "Which filter is used to derive a normal map from the input data.");
}

void Bumpy::_validate( bool for_real ) {
	copy_info();

	// build input mask
	_inputChannels.clear();
	_inputChannels += _sourceChannel;

	// safety check input channel to stop reading invalid data
	if( !DD::Image::intersect(input0().channels(), _inputChannels) ) {
		std::cerr << "Bumpy warning: Input image does not contain requested input channel; skipping.\n";
		set_out_channels(DD::Image::Mask_None);
		info_.turn_on(DD::Image::Mask_None);
		return;
	}

	// build output mask
	_outputChannels.clear();
	for( int i = 0; i < 3; ++i ) {
		_outputChannels += _normalChannels[i];
	}

	// set output channels
	set_out_channels(_outputChannels);
	info_.turn_on(_outputChannels);
}

void Bumpy::_request( int x, int y, int r, int t, DD::Image::ChannelMask channels, int count ) {
	// request input channel and expand the input area for the filter
	DD::Image::ChannelSet newMask(channels);
	newMask += _inputChannels;
	input0().request(x-1, y-1, r+1, t+1, newMask, count+1);
}

void Bumpy::engine( int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out) {
	// copy input
	input0().get(y, x, r, channels, out);

	int currX = x-1;
	const int currR = r+1;

	// read input tile
	const DD::Image::Interest interest(input0(), currX, y-1, currR, r+1, _inputChannels);
	DD::Image::Row in0(currX, currR);
	input0().get(y+1, currX, currR, channels, in0);
	DD::Image::Row in1(currX, currR);
	input0().get(y, currX, currR, channels, in1);
	DD::Image::Row in2(currX, currR);
	input0().get(y-1, currX, currR, channels, in2);

	// output pointers
	float* outR = out.writable(_normalChannels[0]) + x;
	float* outG = out.writable(_normalChannels[1]) + x;
	float* outB = out.writable(_normalChannels[2]) + x;
	const float* END = outR + (r - x);

	// loop through all pixels
	while( outR < END ) {
		// compute normal using a filter
		DD::Image::Vector3 normal;
		switch( _filterType ) {
			case 0: {
				normal = sobelFilter(currX, in0, in1, in2);
				break;
			}

			case 1: {
				normal = scharrFilter(currX, in0, in1, in2);
				break;
			}

			case 2: {
				normal = prewittFilter(currX, in0, in1, in2);
				break;
			}

			default: {
				normal = DD::Image::Vector3(1.0f, 0.0f, 1.0f); // obvious pink error, hopefully
			}
		}
		// normalize for mathematical sanity
		normal.normalize();

		// inversions
		if( _invertX ) {
			normal.x = -normal.x;
		}
		if( _invertY ) {
			normal.y = -normal.y;
		}
		if( _invertZ ) {
			normal.z = -normal.z;
		}

		// output raw normal or [0,1] normal
		if( _normalize ) {
			*outR++= normal.x * 0.5f + 0.5f;
			*outG++= normal.y * 0.5f + 0.5f;
			*outB++= normal.z * 0.5f + 0.5f;
		} else {
			*outR++= normal.x;
			*outG++= normal.y;
			*outB++= normal.z;
		}

		++currX;
	}
}

const char* Bumpy::Class() const {
	return CLASS;
}

const char* Bumpy::node_help() const {
	return HELP;
}

DD::Image::Vector3 Bumpy::sobelFilter( const int x, const DD::Image::Row& in0, const DD::Image::Row& in1, const DD::Image::Row& in2 ) const {
	// idea and algorithm from the following link:
	// http://stackoverflow.com/questions/2368728/can-normal-maps-be-generated-from-a-texture

	const float* ptr = nullptr;

	// get the filter values using the following sobel kernel:
	// TL TC TR
	// CL CC CR -> CC is the current pixel
	// BL BC BR
	//
	// https://en.wikipedia.org/wiki/Sobel_operator
	//
	// tl;dr the X component of the vector is sobel in X
	// and the Y component of the vector is sobel in Y,
	// then the Z can be the "strength"
	//
	ptr = in0[_sourceChannel] + x;       // set pointer to TL
	const float topLeft = (*ptr++);      // get TL and move to TC
	const float topCenter = (*ptr++);    // get TC and move to TR
	const float topRight = (*ptr);       // get TR
	ptr = in1[_sourceChannel] + x;       // set pointer to CL
	const float centerLeft = (*ptr);     // get CL
	ptr += 2;                            // skip CC and set pointer to CR
	const float centerRight = (*ptr);    // get CR
	ptr = in2[_sourceChannel] + x;       // set pointer to BL
	const float bottomLeft = (*ptr++);   // get BL and move to BC
	const float bottomCenter = (*ptr++); // get BC and move to BR
	const float bottomRight = (*ptr);    // get BR

	// compute vector using sobel filter
	const float dx = (topRight + 2.0f * centerRight + bottomRight) - (topLeft + 2.0f * centerLeft + bottomLeft);
	const float dy = (bottomLeft + 2.0f * bottomCenter + bottomRight) - (topLeft + 2.0f * topCenter + topRight);
	const float dz = 1.0f / _strength;

	DD::Image::Vector3 vec(dx, dy, dz);
	return vec;
}

DD::Image::Vector3 Bumpy::scharrFilter( const int x, const DD::Image::Row& in0, const DD::Image::Row& in1, const DD::Image::Row& in2 ) const {
	const float* ptr = nullptr;
	ptr = in0[_sourceChannel] + x;       // set pointer to TL
	const float topLeft = (*ptr++);      // get TL and move to TC
	const float topCenter = (*ptr++);    // get TC and move to TR
	const float topRight = (*ptr);       // get TR
	ptr = in1[_sourceChannel] + x;       // set pointer to CL
	const float centerLeft = (*ptr);     // get CL
	ptr += 2;                            // skip CC and set pointer to CR
	const float centerRight = (*ptr);    // get CR
	ptr = in2[_sourceChannel] + x;       // set pointer to BL
	const float bottomLeft = (*ptr++);   // get BL and move to BC
	const float bottomCenter = (*ptr++); // get BC and move to BR
	const float bottomRight = (*ptr);    // get BR

	const float dx = ((3.0f*topLeft) + (10.0f*centerLeft) + (3.0f*bottomLeft)) - ((3.0f*topRight) + (10.0f*centerRight) + (3.0f*bottomRight));
	const float dy = ((3.0f*topLeft) + (10.0f*topCenter) + (3.0f*topRight)) - ((3.0f*bottomLeft) + (10.0f*bottomCenter) + (3.0f*bottomRight));
	const float dz = 1.0f / _strength;

	DD::Image::Vector3 vec(dx, dy, dz);
	return vec;
}

DD::Image::Vector3 Bumpy::prewittFilter( const int x, const DD::Image::Row& in0, const DD::Image::Row& in1, const DD::Image::Row& in2 ) const {
	const float* ptr = nullptr;
	ptr = in0[_sourceChannel] + x;       // set pointer to TL
	const float topLeft = (*ptr++);      // get TL and move to TC
	const float topCenter = (*ptr++);    // get TC and move to TR
	const float topRight = (*ptr);       // get TR
	ptr = in1[_sourceChannel] + x;       // set pointer to CL
	const float centerLeft = (*ptr);     // get CL
	ptr += 2;                            // skip CC and set pointer to CR
	const float centerRight = (*ptr);    // get CR
	ptr = in2[_sourceChannel] + x;       // set pointer to BL
	const float bottomLeft = (*ptr++);   // get BL and move to BC
	const float bottomCenter = (*ptr++); // get BC and move to BR
	const float bottomRight = (*ptr);    // get BR

	const float dx = (topRight + centerRight + bottomRight) - (topLeft + centerLeft + bottomLeft);
	const float dy = (bottomLeft + bottomCenter + bottomRight) - (topLeft + topCenter + topRight);
	const float dz = 1.0f / _strength;

	DD::Image::Vector3 vec(dx, dy, dz);
	return vec;
}