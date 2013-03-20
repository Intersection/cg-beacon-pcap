#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/ImageIo.h"

#include "cinder/params/Params.h"

#include "Resources.h"
#include "Constants.h"
#include "Beacon.h"


using namespace ci;
using namespace ci::app;
using namespace std;
Beacon::Beacon()
{
    mPacketCaptureRunning = false;
	mPacketCaptureShouldStop = false;
}

void Beacon::togglePacketCapture()
{
	if(mPacketCaptureRunning){
		stopPacketCapture();
		console() << "Stopping packet capture." << std::endl;
	}else{
		startPacketCapture();
		console() << "Starting packet capture." << std::endl;
	}
}

void Beacon::startPacketCapture()
{
	console() << "Starting packet capture." << std::endl;
	if(mPacketCaptureRunning) return;
	try {
		mPacketCaptureShouldStop = false;
		char errbuf[PCAP_ERRBUF_SIZE];
        
		mPCapDescriptor = pcap_open_live("en0", BUFSIZ, 1, 10, errbuf);
		
		if(mPCapDescriptor == NULL)
		{
			printf("Error with pcap_open_live(): %s\n", errbuf);
			return;
		}
		
        
		mPacketCaptureThread = thread( bind( &Beacon::doPacketCaptureFn, this ) );
		mPacketCaptureRunning = true;
	}catch(...){
		console() << "Error opening device." << std::endl;
		mPacketCaptureRunning = false;
	}
	
}

void Beacon::stopPacketCapture()
{
	if(!mPacketCaptureRunning) return;
	mPacketCaptureShouldStop = true;
	mPacketCaptureThread.join();
	mPacketCaptureRunning = false;
}

void Beacon::doPacketCaptureFn()
{
	ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
    
	const u_char *packet;
	struct pcap_pkthdr hdr;			/* pcap.h */
	struct ether_header *eptr;	/* net/ethernet.h */
	
	int i;
	u_char *ptr; /* printing out hardware header info */
	
	while(!mPacketCaptureShouldStop) {
		packet = pcap_next(mPCapDescriptor, &hdr);
		
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
		
		mPings[addy]++;
	}
	
}

int Beacon::getPingCountForMAC(std::string mac)
{
    return mPings[mac];
}

Beacon::~Beacon()
{
    
}

std::map<std::string, int> Beacon::getPings()
{
    return mPings;
}

std::map<std::string, int> Beacon::getAndClearPings()
{
    std::map<std::string, int> tmpMap;
    tmpMap.insert(mPings.begin(), mPings.end());
    mPings.clear();
    return tmpMap;
}
