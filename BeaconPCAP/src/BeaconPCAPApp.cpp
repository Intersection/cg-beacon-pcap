#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/ImageIo.h"

#include "cinder/params/Params.h"

#include "Resources.h"
#include "Constants.h"

#include <pcap/pcap.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include "cinder/Thread.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BeaconPCAPApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	void setup();
	void mouseDown( MouseEvent event );
	void keyDown( KeyEvent event );
	void resize( ResizeEvent event );
	void update();
	void draw();
	
	void togglePacketCapture();
	void startPacketCapture();
	void stopPacketCapture();
	void doPacketCaptureFn();
	
	
	gl::Texture				mTexture;
	gl::GlslProg			mShader;
	gl::Fbo					mFbo;
	
	bool					mPacketCaptureRunning;
	bool					mPacketCaptureShouldStop;
	std::thread				mPacketCaptureThread;

	vector<std::string>		mPings;
	
};

void BeaconPCAPApp::togglePacketCapture()
{
	if(mPacketCaptureRunning){
		stopPacketCapture();
		console() << "Stopping packet capture." << std::endl;
	}else{
		startPacketCapture();
		console() << "Starting packet capture." << std::endl;
	}
}
void BeaconPCAPApp::startPacketCapture()
{
	if(mPacketCaptureRunning) return;
	try {
		mPacketCaptureShouldStop = false;
		mPacketCaptureThread = thread( bind( &BeaconPCAPApp::doPacketCaptureFn, this ) );
		mPacketCaptureRunning = true;
	}catch(...){
		mPacketCaptureRunning = false;
	}
	
}

void BeaconPCAPApp::stopPacketCapture()
{
	if(!mPacketCaptureRunning) return;
	mPacketCaptureShouldStop = true;
	mPacketCaptureThread.join();
	mPacketCaptureRunning = false;
}

void BeaconPCAPApp::doPacketCaptureFn()
{
	ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread

	int i;
	char *dev;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* descr;
	const u_char *packet;
	struct pcap_pkthdr hdr;			/* pcap.h */
	struct ether_header *eptr;	/* net/ethernet.h */
	
	u_char *ptr; /* printing out hardware header info */
	
	/* grab a device to peak into... */
	dev = pcap_lookupdev(errbuf);
	
	if(dev == NULL)
	{
		printf("%s\n",errbuf);
		exit(1);
	}
	
	descr = pcap_open_live(dev, BUFSIZ, 1, 10, errbuf);
	
	if(descr == NULL)
	{
		printf("Error with pcap_open_live(): %s\n", errbuf);
		exit(1);
	}
	
	while(!mPacketCaptureShouldStop) {
		packet = pcap_next(descr,&hdr);
		
		if(packet == NULL)
		{
			// Didn't grab packet
			continue;
		}
		
		// Get header
		eptr = (struct ether_header *) packet;
		
		// Get packet type (IP, ARP, other)
		if(ntohs (eptr->ether_type) == ETHERTYPE_IP){
			//printf("Ethernet type hex:%x dec:%d is an IP packet\n", ntohs(eptr->ether_type), ntohs(eptr->ether_type));
		}else if(ntohs (eptr->ether_type) == ETHERTYPE_ARP){
			//printf("Ethernet type hex:%x dec:%d is an ARP packet\n", ntohs(eptr->ether_type), ntohs(eptr->ether_type));
		}else {
			//printf("Ethernet type %x not IP", ntohs(eptr->ether_type));
			continue;
		}
		
		// Get the source address
		ptr = eptr->ether_shost;
		i = ETHER_ADDR_LEN;
		string addy;
		do {
			char tmp[10];
			snprintf(tmp, sizeof(tmp), "%s%x", (i == ETHER_ADDR_LEN) ? "" : ":", *ptr++);
			addy += tmp;
			
		} while(--i > 0);
		
		mPings.push_back(addy);
	}
	
}


void BeaconPCAPApp::prepareSettings( Settings *settings )
{
	settings->setFrameRate( kFrameRate );
	settings->setWindowSize( kWindowWidth, kWindowHeight );
}

void BeaconPCAPApp::setup()
{
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

void BeaconPCAPApp::resize( ResizeEvent event )
{
	mFbo = gl::Fbo( getWindowWidth(), getWindowHeight() );
}


void BeaconPCAPApp::update()
{
	// Do something with your texture here.
	console() << mPings.size() << std::endl;
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
	gl::drawSolidRect( getWindowBounds() );
	mTexture.unbind();
	mShader.unbind();
	mFbo.unbindFramebuffer();
	
	gl::Texture fboTexture = mFbo.getTexture();
	fboTexture.setFlipped();
	gl::draw( fboTexture );
	
}


CINDER_APP_BASIC( BeaconPCAPApp, RendererGl(0) )
