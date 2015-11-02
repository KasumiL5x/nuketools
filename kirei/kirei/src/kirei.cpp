#include "Kirei.hpp"
#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/Tile.h>

static const char* CLASS = "Kirei";
static const char* HELP = "Kirei da yo ne.";

static const char* FILTER_TYPES[] = {
	"Passthrough",
	"Vignette",
	"Invert",
	"Threshold",
	"Sepia",
	"Blur",
	"Sharpen",
	"Edge Enhance",
	"Temperature",
	"Channel Mixer",
	"Playground",
	0
};

struct FilterTypes {
	enum Type {
		Passthrough=0,
		Vignette,
		Invert,
		Threshold,
		Sepia,
		Blur,
		Sharpen,
		EdgeEnhance,
		Temperature,
		ChannelMixer,
		Playground
	};
};

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
	inline T lerp( T p0, T p1, float s ) {
		return p0 + (p1 - p0) * s;
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

static DD::Image::Iop* build( Node* node ) {
	return new Kirei(node);
}

const DD::Image::Iop::Description Kirei::description(CLASS, "KasumiL5x/Kirei", build);

Kirei::Kirei( Node* node )
	: PixelIop(node) {
	_inputWidth = 1;
	_inputHeight = 1;
	_filterType = 0;

	_vignetteRadius = 0.75f;
	_vignetteSoftness = 0.45f;

	_thresholdLuminanceLimit = 0.5f;

	_blurSize = 4;

	_sharpenStrength = 1.0f;

	_edgeEnhanceStrength = 1.0f;

	_temperature = 6650.0f;

	_channelMixerBlueGreen = 0.0f;
	_channelMixerBGIntoRed = 0.0f;
	_channelMixerRedBlue = 0.0f;
	_channelMixerRBIntoGreen = 0.0f;
	_channelMixerGreenRed = 0.0f;
	_channelMixerGRIntoBlue = 0.0f;
}

Kirei::~Kirei() {
}

int Kirei::minimum_inputs() const {
	return 1;
}

int Kirei::maximum_inputs() const {
	return 1;
}

void Kirei::in_channels( int input, DD::Image::ChannelSet& channels ) const {
	// turn on other color channels if any are requested
	DD::Image::ChannelSet done;
	foreach(z, channels) {
		// if it is red, green, or blue...
		if( DD::Image::colourIndex(z) < 3 ) {
			// ... add all three to the "done" set
			if( !(done & z) ) {
				done.addBrothers(z, 3);
			}
		}
	}
	// add colors to channels we need
	channels += done;
}

void Kirei::_validate( bool for_real ) {
	const DD::Image::Box& inputBox = input0().requestedBox();
	_inputWidth = inputBox.w();
	_inputHeight = inputBox.h();
	printf("requested box {x:%d; y:%d; w:%d; h:%d}\n", inputBox.x(), inputBox.y(), _inputWidth, _inputHeight);

	set_out_channels(DD::Image::Mask_All);

	if( FilterTypes::Blur == _filterType ) {
		info_.pad(_blurSize);
	} else if( FilterTypes::Sharpen == _filterType ) {
		info_.pad(1);
	} else if( FilterTypes::EdgeEnhance == _filterType ) {
		info_.pad(1);
	}

	PixelIop::_validate(for_real);
}

void Kirei::_request( int x, int y, int r, int t, DD::Image::ChannelMask channels, int count ) {
	if( FilterTypes::Blur == _filterType ) {
		input(0)->request(x-_blurSize, y-_blurSize, r+_blurSize, t+_blurSize, channels, count);
	} else if( FilterTypes::Sharpen == _filterType ) {
		input(0)->request(x-1, y-1, r+1, t+1, channels, count);
	} else if( FilterTypes::EdgeEnhance == _filterType ) {
		input(0)->request(x-1, y-1, r+1, t+1, channels, count);
	}
	PixelIop::_request(x, y, r, t, channels, count);
}

void Kirei::knobs( DD::Image::Knob_Callback f ) {
	DD::Image::Enumeration_knob(f, &_filterType, FILTER_TYPES, "filter_type", "Filter Type");

	DD::Image::BeginGroup(f, "vignette");
		DD::Image::Float_knob(f, &_vignetteRadius, "radius");
		DD::Image::Float_knob(f, &_vignetteSoftness, "softness");
	DD::Image::EndGroup(f);

	DD::Image::BeginGroup(f, "threshold");
		DD::Image::Float_knob(f, &_thresholdLuminanceLimit, "luminance_limit", "Luminance Limit");
	DD::Image::EndGroup(f);

	DD::Image::BeginGroup(f, "Blur");
		DD::Image::Int_knob(f, &_blurSize, "blur_size", "Size");
	DD::Image::EndGroup(f);

	DD::Image::BeginGroup(f, "Sharpen");
		DD::Image::Float_knob(f, &_sharpenStrength, "sharpen_strength", "Strength");
	DD::Image::EndGroup(f);

	DD::Image::BeginGroup(f, "Edge Enhance");
		DD::Image::Float_knob(f, &_edgeEnhanceStrength, "edge_enhance_strength", "Strength");
	DD::Image::EndGroup(f);

	DD::Image::BeginGroup(f, "Temperature");
		DD::Image::Float_knob(f, &_temperature, DD::Image::IRange(1000.0f, 10000.0f), "temperature", "Cool/Heat");
	DD::Image::EndGroup(f);

	DD::Image::BeginGroup(f, "Channel Mixer");
		DD::Image::Float_knob(f, &_channelMixerBlueGreen, "channel_mixer_b_to_g", "Mix B/G");
		DD::Image::ClearFlags(f, DD::Image::Knob::ENDLINE); DD::Image::SetFlags(f, DD::Image::Knob::HIDE_ANIMATION_AND_VIEWS);
		DD::Image::Float_knob(f, &_channelMixerBGIntoRed, "channel_mixer_into_r", "into R");
		DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE); DD::Image::SetFlags(f, DD::Image::Knob::HIDE_ANIMATION_AND_VIEWS);

		DD::Image::Float_knob(f, &_channelMixerRedBlue, "channel_mixer_r_to_b", "Mix R/B");
		DD::Image::ClearFlags(f, DD::Image::Knob::ENDLINE); DD::Image::SetFlags(f, DD::Image::Knob::HIDE_ANIMATION_AND_VIEWS);
		DD::Image::Float_knob(f, &_channelMixerRBIntoGreen, "channel_mixer_into_g", "into G");
		DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE); DD::Image::SetFlags(f, DD::Image::Knob::HIDE_ANIMATION_AND_VIEWS);

		DD::Image::Float_knob(f, &_channelMixerGreenRed, "channel_mixer_g_to_r", "Mix G/R");
		DD::Image::ClearFlags(f, DD::Image::Knob::ENDLINE); DD::Image::SetFlags(f, DD::Image::Knob::HIDE_ANIMATION_AND_VIEWS);
		DD::Image::Float_knob(f, &_channelMixerGRIntoBlue, "channel_mixer_into_b", "into B");
		DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE); DD::Image::SetFlags(f, DD::Image::Knob::HIDE_ANIMATION_AND_VIEWS);
	DD::Image::EndGroup(f);
}

void Kirei::pixel_engine( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	switch( _filterType ) {
		case FilterTypes::Passthrough: {
			passthrough(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::Vignette: {
			vignette(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::Invert: {
			invert(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::Threshold: {
			threshold(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::Sepia: {
			sepia(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::Blur: {
			blur(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::Sharpen: {
			sharpen(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::EdgeEnhance: {
			edgeEnhance(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::Temperature: {
			temperature(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::ChannelMixer: {
			mixChannels(in, y, x, r, channels, out);
			break;
		}

		case FilterTypes::Playground: {
			playground(in, y, x, r, channels, out);
			break;
		}
	}
}

void Kirei::passthrough( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		const float* rIn = in[rChan] + x;
		const float* gIn = in[gChan] + x;
		const float* bIn = in[bChan] + x;
		const float* end = rIn + (r - x);

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		while( rIn < end ) {
			*rOut++= *rIn++;
			*gOut++= *gIn++;
			*bOut++= *bIn++;
		}
	}
}

void Kirei::vignette( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	const DD::Image::Vector2 SCREEN_SIZE = DD::Image::Vector2(static_cast<float>(_inputWidth), static_cast<float>(_inputHeight));

	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		const float* rIn = in[rChan] + x;
		const float* gIn = in[gChan] + x;
		const float* bIn = in[bChan] + x;
		const float* end = rIn + (r - x);

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		int currX = x;
		while( rIn < end ) {
			const float RADIUS = _vignetteRadius;
			const float SOFTNESS = _vignetteSoftness;
			const DD::Image::Vector2 position = DD::Image::Vector2(static_cast<float>(currX), static_cast<float>(y)) / SCREEN_SIZE - DD::Image::Vector2(0.5f, 0.5f);
			const float len = position.length();
			const float vignette = ccmath::smoothstep<float>(RADIUS, RADIUS - SOFTNESS, len);

			const float currRed = *rIn++;
			const float currGreen = *gIn++;
			const float currBlue = *bIn++;

			*rOut++= ccmath::lerp<float>(currRed, currRed * vignette, 0.5f);
			*gOut++= ccmath::lerp<float>(currGreen, currGreen * vignette, 0.5f);
			*bOut++= ccmath::lerp<float>(currBlue, currBlue * vignette, 0.5f);

			currX += 1;
		}
	}
}

void Kirei::invert( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		const float* rIn = in[rChan] + x;
		const float* gIn = in[gChan] + x;
		const float* bIn = in[bChan] + x;
		const float* end = rIn + (r - x);

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		while( rIn < end ) {
			*rOut++= 1.0f - *rIn++;
			*gOut++= 1.0f - *gIn++;
			*bOut++= 1.0f - *bIn++;
		}
	}
}

void Kirei::threshold( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		const float* rIn = in[rChan] + x;
		const float* gIn = in[gChan] + x;
		const float* bIn = in[bChan] + x;
		const float* end = rIn + (r - x);

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		while( rIn < end ) {
			const float r = *rIn++;
			const float g = *gIn++;
			const float b = *bIn++;
			*rOut++= *gOut++= *bOut++= (pixelLuminance(r, g, b) < _thresholdLuminanceLimit) ? 0.0f : 1.0f;
		}
	}
}

void Kirei::sepia( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	// http://stackoverflow.com/questions/1061093/how-is-a-sepia-tone-created

	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		const float* rIn = in[rChan] + x;
		const float* gIn = in[gChan] + x;
		const float* bIn = in[bChan] + x;
		const float* end = rIn + (r - x);

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		while( rIn < end ) {
			const float r = *rIn++;
			const float g = *gIn++;
			const float b = *bIn++;

			*rOut++= ccmath::minimum<float>((r * 0.393f) + (g * 0.769f) + (b * 0.189f), 1.0f);
			*gOut++= ccmath::minimum<float>((r * 0.349f) + (g * 0.686f) + (b * 0.168f), 1.0f);
			*bOut++= ccmath::minimum<float>((r * 0.272f) + (g * 0.534f) + (b * 0.131f), 1.0f);
		}
	}
}

void Kirei::blur( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	const DD::Image::Tile tile(input0(), x - _blurSize, y - _blurSize, r + _blurSize, y + _blurSize, channels);
	if( Op::aborted() ) {
		return;
	}

	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		for( int currX = x; currX < r; ++currX ) {
			float red = 0.0f;
			float green = 0.0f;
			float blue = 0.0f;
			int count = 0;
			for( int px = -_blurSize; px <= _blurSize; ++px ) {
				for( int py = -_blurSize; py <= _blurSize; ++py ) {
					red += tile[rChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					green += tile[gChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					blue += tile[bChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					count += 1;
				}
			}
			if( count != 0 ) {
				red /= static_cast<float>(count);
				green /= static_cast<float>(count);
				blue /= static_cast<float>(count);
			}

			*rOut++= red;
			*gOut++= green;
			*bOut++= blue;
		}
	}
}

void Kirei::sharpen( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	// http://cis.k.hosei.ac.jp/~wakahara/sharpen.c

	const DD::Image::Tile tile(input0(), x - 1, y - 1, r + 1, y + 1, channels);
	if( Op::aborted() ) {
		return;
	}

	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		const float kernel[3][3] = {
			{1.0f,  1.0f, 1.0f},
			{1.0f, -8.0f, 1.0f},
			{1.0f,  1.0f, 1.0f}
		};

		for( int currX = x; currX < r; ++currX ) {
			float red = 0.0f;
			float green = 0.0f;
			float blue = 0.0f;
			for( int px = -1; px <= 1; ++px ) {
				for( int py = -1; py <= 1; ++py ) {
					red   += kernel[px+1][py+1] * tile[rChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					green += kernel[px+1][py+1] * tile[gChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					blue  += kernel[px+1][py+1] * tile[bChan][tile.clampy(y + py)][tile.clampx(currX + px)];
				}
			}

			*rOut++= tile[rChan][tile.clampy(y)][tile.clampx(currX)] - (red * _sharpenStrength);
			*gOut++= tile[gChan][tile.clampy(y)][tile.clampx(currX)] - (green * _sharpenStrength);
			*bOut++= tile[bChan][tile.clampy(y)][tile.clampx(currX)] - (blue * _sharpenStrength);
		}
	}
}

void Kirei::edgeEnhance( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	const DD::Image::Tile tile(input0(), x - 1, y - 1, r + 1, y + 1, channels);
	if( Op::aborted() ) {
		return;
	}

	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		const float kernel[3][3] = {
			{0.0f,  1.0f, 0.0f},
			{-1.0f * _edgeEnhanceStrength, 1.0f * _edgeEnhanceStrength, 0.0f},
			{0.0f,  0.0f, 0.0f}
		};

		for( int currX = x; currX < r; ++currX ) {
			float red = 0.0f;
			float green = 0.0f;
			float blue = 0.0f;
			for( int px = -1; px <= 1; ++px ) {
				for( int py = -1; py <= 1; ++py ) {
					red   += kernel[px+1][py+1] * tile[rChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					green += kernel[px+1][py+1] * tile[gChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					blue  += kernel[px+1][py+1] * tile[bChan][tile.clampy(y + py)][tile.clampx(currX + px)];
				}
			}

			*rOut++= red;
			*gOut++= green;
			*bOut++= blue;
		}
	}
}

void Kirei::temperature( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		const float* rIn = in[rChan] + x;
		const float* gIn = in[gChan] + x;
		const float* bIn = in[bChan] + x;
		const float* end = rIn + (r - x);

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		const float blackBodyRGB[] = { 1.0f, 0.0337f, 0.0f, 1.0f, 0.0592f, 0.0f, 1.0f, 0.0846f, 0.0f, 1.0f, 0.1096f, 0.0f, 1.0f, 0.1341f, 0.0f, 1.0f, 0.1578f, 0.0f, 1.0f, 0.1806f, 0.0f, 1.0f, 0.2025f, 0.0f, 1.0f, 0.2235f, 0.0f, 1.0f, 0.2434f, 0.0f, 1.0f, 0.2647f, 0.0033f, 1.0f, 0.2889f, 0.012f, 1.0f, 0.3126f, 0.0219f, 1.0f, 0.336f, 0.0331f, 1.0f, 0.3589f, 0.0454f, 1.0f, 0.3814f, 0.0588f, 1.0f, 0.4034f, 0.0734f, 1.0f, 0.425f, 0.0889f, 1.0f, 0.4461f, 0.1054f, 1.0f, 0.4668f, 0.1229f, 1.0f, 0.487f, 0.1411f, 1.0f, 0.5067f, 0.1602f, 1.0f, 0.5259f, 0.18f, 1.0f, 0.5447f, 0.2005f, 1.0f, 0.563f, 0.2216f, 1.0f, 0.5809f, 0.2433f, 1.0f, 0.5983f, 0.2655f, 1.0f, 0.6153f, 0.2881f, 1.0f, 0.6318f, 0.3112f, 1.0f, 0.648f, 0.3346f, 1.0f, 0.6636f, 0.3583f, 1.0f, 0.6789f, 0.3823f, 1.0f, 0.6938f, 0.4066f, 1.0f, 0.7083f, 0.431f, 1.0f, 0.7223f, 0.4556f, 1.0f, 0.736f, 0.4803f, 1.0f, 0.7494f, 0.5051f, 1.0f, 0.7623f, 0.5299f, 1.0f, 0.775f, 0.5548f, 1.0f, 0.7872f, 0.5797f, 1.0f, 0.7992f, 0.6045f, 1.0f, 0.8108f, 0.6293f, 1.0f, 0.8221f, 0.6541f, 1.0f, 0.833f, 0.6787f, 1.0f, 0.8437f, 0.7032f, 1.0f, 0.8541f, 0.7277f, 1.0f, 0.8642f, 0.7519f, 1.0f, 0.874f, 0.776f, 1.0f, 0.8836f, 0.8f, 1.0f, 0.8929f, 0.8238f, 1.0f, 0.9019f, 0.8473f, 1.0f, 0.9107f, 0.8707f, 1.0f, 0.9193f, 0.8939f, 1.0f, 0.9276f, 0.9168f, 1.0f, 0.9357f, 0.9396f, 1.0f, 0.9436f, 0.9621f, 1.0f, 0.9513f, 0.9844f, 0.9937f, 0.9526f, 1.0f, 0.9726f, 0.9395f, 1.0f, 0.9526f, 0.927f, 1.0f, 0.9337f, 0.915f, 1.0f, 0.9157f, 0.9035f, 1.0f, 0.8986f, 0.8925f, 1.0f, 0.8823f, 0.8819f, 1.0f, 0.8668f, 0.8718f, 1.0f, 0.852f, 0.8621f, 1.0f, 0.8379f, 0.8527f, 1.0f, 0.8244f, 0.8437f, 1.0f, 0.8115f, 0.8351f, 1.0f, 0.7992f, 0.8268f, 1.0f, 0.7874f, 0.8187f, 1.0f, 0.7761f, 0.811f, 1.0f, 0.7652f, 0.8035f, 1.0f, 0.7548f, 0.7963f, 1.0f, 0.7449f, 0.7894f, 1.0f, 0.7353f, 0.7827f, 1.0f, 0.726f, 0.7762f, 1.0f, 0.7172f, 0.7699f, 1.0f, 0.7086f, 0.7638f, 1.0f, 0.7004f, 0.7579f, 1.0f, 0.6925f, 0.7522f, 1.0f, 0.6848f, 0.7467f, 1.0f, 0.6774f, 0.7414f, 1.0f, 0.6703f, 0.7362f, 1.0f, 0.6635f, 0.7311f, 1.0f, 0.6568f, 0.7263f, 1.0f, 0.6504f, 0.7215f, 1.0f, 0.6442f, 0.7169f, 1.0f, 0.6382f, 0.7124f, 1.0f, 0.6324f, 0.7081f, 1.0f, 0.6268f, 0.7039f, 1.0f };

		while( rIn < end ) {
			const float r = *rIn++;
			const float g = *gIn++;
			const float b = *bIn++;

			_temperature = ccmath::maximum<float>(1000.0f, ccmath::minimum<float>(10000.0f, _temperature));
			const int t = 3 * static_cast<int>((_temperature - 1000.0f) / 100.0f);
			float rFactor = 1.0f / blackBodyRGB[t];
			float gFactor = 1.0f / blackBodyRGB[t+1];
			float bFactor = 1.0f / blackBodyRGB[t+2];
			const float m = ccmath::maximum<float>(ccmath::maximum<float>(rFactor, gFactor), bFactor);
			rFactor /= m;
			gFactor /= m;
			bFactor /= m;

			*rOut++= r * rFactor;
			*gOut++= g * gFactor;
			*bOut++= b * bFactor;
		}
	}
}

void Kirei::mixChannels( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		const float* rIn = in[rChan] + x;
		const float* gIn = in[gChan] + x;
		const float* bIn = in[bChan] + x;
		const float* end = rIn + (r - x);

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		while( rIn < end ) {
			const float r = *rIn++;
			const float g = *gIn++;
			const float b = *bIn++;
			const float newRed   = (_channelMixerBGIntoRed   * (_channelMixerBlueGreen * g + (1.0f - _channelMixerBlueGreen) * b) + (1.0f - _channelMixerBGIntoRed)   * r);
			const float newGreen = (_channelMixerRBIntoGreen * (_channelMixerRedBlue   * b + (1.0f - _channelMixerRedBlue)   * r) + (1.0f - _channelMixerRBIntoGreen) * g);
			const float newBlue  = (_channelMixerGRIntoBlue  * (_channelMixerGreenRed  * r + (1.0f - _channelMixerGreenRed)  * g) + (1.0f - _channelMixerGRIntoBlue)  * b);
			*rOut++= ccmath::clamp<float>(newRed, 0.0f, 1.0f);;
			*gOut++= ccmath::clamp<float>(newGreen, 0.0f, 1.0f);
			*bOut++= ccmath::clamp<float>(newBlue, 0.0f, 1.0f);
		}
	}
}

void Kirei::playground( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) {
	// http://docs.gimp.org/en/plug-in-convmatrix.html

	const DD::Image::Tile tile(input0(), x - 1, y - 1, r + 1, y + 1, channels);
	if( Op::aborted() ) {
		return;
	}

	DD::Image::ChannelSet done;
	foreach(z, channels) {
		if( done & z ) {
			continue;
		}

		if( DD::Image::colourIndex(z) >= 3 ) {
			out.copy(in, z, x, r);
			continue;
		}

		const DD::Image::Channel rChan = DD::Image::brother(z, 0);
		done += rChan;
		const DD::Image::Channel gChan = DD::Image::brother(z, 1);
		done += gChan;
		const DD::Image::Channel bChan = DD::Image::brother(z, 2);
		done += bChan;

		float* rOut = out.writable(rChan) + x;
		float* gOut = out.writable(gChan) + x;
		float* bOut = out.writable(bChan) + x;

		const float kernel[3][3] = {
			{0.0f,  1.0f, 0.0f},
			{1.0f, -4.0f, 1.0f},
			{0.0f,  1.0f, 0.0f}
		};

		for( int currX = x; currX < r; ++currX ) {
			float red = 0.0f;
			float green = 0.0f;
			float blue = 0.0f;
			for( int px = -1; px <= 1; ++px ) {
				for( int py = -1; py <= 1; ++py ) {
					red   += kernel[px+1][py+1] * tile[rChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					green += kernel[px+1][py+1] * tile[gChan][tile.clampy(y + py)][tile.clampx(currX + px)];
					blue  += kernel[px+1][py+1] * tile[bChan][tile.clampy(y + py)][tile.clampx(currX + px)];
				}
			}

			*rOut++= red;
			*gOut++= green;
			*bOut++= blue;
		}
	}
}

float Kirei::pixelLuminance( const float r, const float g, const float b ) const {
	return (r * 0.3f) + (g * 0.59f) + (b * 0.11f);
}

const char* Kirei::Class() const {
	return CLASS;
}

const char* Kirei::node_help() const {
	return HELP;
}