#include <DDImage/Iop.h>

class Bumpy : public DD::Image::Iop {
public:
	Bumpy( Node* node );
	virtual ~Bumpy();

	virtual void knobs( DD::Image::Knob_Callback f ) override;
	virtual void _validate( bool for_real ) override;
	virtual void _request( int x, int y, int r, int t, DD::Image::ChannelMask channels, int count ) override;
	virtual void engine( int y, int x, int r, DD::Image::ChannelMask channels, DD::Image::Row& out) override;

public:
	virtual const char* Class() const override;
	virtual const char* node_help() const override;
	static const DD::Image::Iop::Description description;

private:
	DD::Image::Vector3 sobelFilter( const int x, const DD::Image::Row& in0, const DD::Image::Row& in1, const DD::Image::Row& in2 ) const;
	DD::Image::Vector3 scharrFilter( const int x, const DD::Image::Row& in0, const DD::Image::Row& in1, const DD::Image::Row& in2 ) const;
	DD::Image::Vector3 prewittFilter( const int x, const DD::Image::Row& in0, const DD::Image::Row& in1, const DD::Image::Row& in2 ) const;

private:
	DD::Image::Channel _sourceChannel;
	DD::Image::Channel _normalChannels[3];
	float _strength;
	bool _invertX;
	bool _invertY;
	bool _invertZ;
	bool _normalize;

	DD::Image::ChannelSet _inputChannels;
	DD::Image::ChannelSet _outputChannels;

	int _filterType;
};