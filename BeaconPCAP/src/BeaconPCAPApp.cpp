#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/ImageIo.h"
#include "cinder/Capture.h"
#include "cinder/Surface.h"

#include "cinder/params/Params.h"

#include "Resources.h"
#include "Constants.h"
#include "Beacon.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BeaconPCAPApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	void setup();
	void mouseDown( MouseEvent event );
	void keyDown( KeyEvent event );
	void resize();
	void update();
	void draw();
	void shutdown();

	void togglePacketCapture();
	
	Capture					mCapture;

	gl::Texture				mTexture;
	gl::GlslProg			mShader;
	gl::Fbo					mFbo;

	Beacon					mBeacon;
	
	std::map<std::string, int> mPingBatch;
};

void BeaconPCAPApp::togglePacketCapture()
{
	mBeacon.togglePacketCapture();
}

void BeaconPCAPApp::prepareSettings( Settings *settings )
{
	settings->setFrameRate( kFrameRate );
	settings->setWindowSize( kWindowWidth, kWindowHeight );
}

void BeaconPCAPApp::shutdown()
{
	mBeacon.stopPacketCapture();
}

void BeaconPCAPApp::setup()
{
	try {
		mCapture = Capture( kCaptureWidth, kCaptureHeight );
		mCapture.start();
	} catch ( ... ) {
		console() << "Error with capture device." << std::endl;
		exit(1);
	}


	try {
		mShader = gl::GlslProg( loadResource( RES_SHADER_PASSTHRU ), loadResource( RES_SHADER_FRAGMENT ) );
	} catch ( gl::GlslProgCompileExc &exc ) {
		console() << "Cannot compile shader: " << exc.what() << std::endl;
		exit(1);
	}catch ( Exception &exc ){
		console() << "Cannot load shader: " << exc.what() << std::endl;
		exit(1);
	}

	
	mFbo = gl::Fbo( kWindowWidth, kWindowHeight );
	try {
		mTexture = gl::Texture( loadImage( loadResource( RES_GRADIENT ) ) );
	}catch ( Exception &exc ){
		console() << "Cannot load texture: " << exc.what() << std::endl;
	}
	
}

void BeaconPCAPApp::mouseDown( MouseEvent event )
{
}

void BeaconPCAPApp::keyDown( KeyEvent event )
{
	if ( event.getCode() == KeyEvent::KEY_f ){
		setFullScreen( !isFullScreen() );
	}
	
	if (event.getCode() == KeyEvent::KEY_p) {
		togglePacketCapture();
	}
}

void BeaconPCAPApp::resize()
{
	mFbo = gl::Fbo( getWindowWidth(), getWindowHeight() );
}


void BeaconPCAPApp::update()
{
	if( mCapture && mCapture.checkNewFrame() ) mTexture = gl::Texture( mCapture.getSurface() );

	// If more than the max MACs, clear and start over
	if((float)mPingBatch.begin()->second > kMaxPings) {
		mPingBatch = mBeacon.getAndClearPings();
		console() << "Clearing all pings." << std::endl;
	} else {
		mPingBatch = mBeacon.getPings();
	}
}

void BeaconPCAPApp::draw()
{
	// clear out the window with black
	gl::clear( kClearColor );
	if( !mTexture ) return;
	mFbo.bindFramebuffer();
	mTexture.enableAndBind();
	mShader.bind();
	mShader.uniform( "tex", 0 );
	mShader.uniform( "bright", 0.99f );
	mShader.uniform( "ledCount", 400.0f );
	mShader.uniform( "aspect", kWindowHeight / kWindowWidth );
	
	mShader.uniform( "p1", (float)mPingBatch.begin()->second );
	
	gl::drawSolidRect( getWindowBounds() );
	mTexture.unbind();
	mShader.unbind();
	mFbo.unbindFramebuffer();
	
	gl::Texture fboTexture = mFbo.getTexture();
	fboTexture.setFlipped();
	gl::draw( fboTexture );
	
}


CINDER_APP_BASIC( BeaconPCAPApp, RendererGl(4) )
