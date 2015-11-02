#include <DDImage/PixelIop.h>

class Kirei : public DD::Image::PixelIop {
public:
	Kirei( Node* node );
	virtual ~Kirei();

	virtual int minimum_inputs() const override;
	virtual int maximum_inputs() const override;
	virtual void in_channels( int input, DD::Image::ChannelSet& channels ) const override;
	virtual void _validate( bool for_real ) override;
	virtual void _request( int x, int y, int r, int t, DD::Image::ChannelMask channels, int count ) override;
	virtual void knobs( DD::Image::Knob_Callback f ) override;
	virtual void pixel_engine( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out ) override;

private:
	void passthrough( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void vignette( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void invert( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void threshold( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void sepia( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void blur( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void sharpen( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void edgeEnhance( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void playground( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void temperature( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );
	void mixChannels( const DD::Image::Row &in, int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out );

private:
	float pixelLuminance( const float r, const float g, const float b ) const;

public:
	virtual const char* Class() const override;
	virtual const char* node_help() const override;
	static const DD::Image::Iop::Description description;

private:
	int _inputWidth;
	int _inputHeight;
	int _filterType;

	// vignette
	float _vignetteRadius;
	float _vignetteSoftness;

	// threshold
	float _thresholdLuminanceLimit;

	// blur
	int _blurSize;

	// sharpen
	float _sharpenStrength;

	// edge enhance
	float _edgeEnhanceStrength;

	// temperature
	float _temperature;

	// channel mixer
	float _channelMixerBlueGreen;
	float _channelMixerBGIntoRed;
	float _channelMixerRedBlue;
	float _channelMixerRBIntoGreen;
	float _channelMixerGreenRed;
	float _channelMixerGRIntoBlue;
};