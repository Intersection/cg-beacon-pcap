#include <pcap/pcap.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include "cinder/Thread.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

class Beacon {
public:
    Beacon();
    ~Beacon();
    
    void togglePacketCapture();
	void startPacketCapture();
	void stopPacketCapture();
    int getPingCountForMAC(std::string mac);
	
	bool					mPacketCaptureRunning;
	bool					mPacketCaptureShouldStop;
	std::thread				mPacketCaptureThread;

    std::map<std::string, int>	mPings;
	pcap_t*					mPCapDescriptor;

protected:
	void doPacketCaptureFn();

};