﻿#include "PhotonLib.h"
#include <iostream>

#if defined _EG_WINDOWS_PLATFORM
#	define SLEEP(ms) Sleep(ms)
#else
#	if defined _EG_LINUX_PLATFORM || defined _EG_MARMALADE_PLATFORM || defined _EG_ANDROID_PLATFORM || defined _EG_BLACKBERRY_PLATFORM
#		include "unistd.h"
#	endif
#	define SLEEP(ms) usleep(ms*1000)
#endif

PhotonLib::PhotonLib(void)
{
    peer = new PhotonPeer(*this);
    mState = State::INITIALIZED;
}

PhotonLib::~PhotonLib(void)
{
    delete peer;
}

void PhotonLib::startwork(const JString& ipAddr, char* airstrikeIP){
    while(true){
        printf(".\n");
        switch(mState){
            case State::INITIALIZED:
                peer->connect(ipAddr);
                mState = State::CONNECTING;
                printf("Connecting \n");
                break;

            case State::CONNECTED:
                {
                    printf("Connected \n");
                    OperationRequestParameters op;
                    op.put((nByte)1, ValueObject<JString>(airstrikeIP));
		    op.put((nByte)2, ValueObject<JString>("1234"));
		    op.put((nByte)3, ValueObject<JString>("amphie 10"));
		    OperationRequest opRequest = OperationRequest((nByte)1, op);
		    showMsg(opRequest.toString(true, true));
                    peer->opCustom(opRequest, true);
		    mState = State::SENDING_IPADRESS;
                    break;
                }

            case State::DISCONNECTED:
                printf("Disconnected \n");
                break;

            default:
                break;
        }
        peer->service();
        SLEEP(500);
	if(mState == State::SENDED) break;
    }

}

void PhotonLib::onStatusChanged(int statusCode)
{
	switch(statusCode)
	{
	case StatusCode::CONNECT:
		mState = State::CONNECTED;
		break;
	case StatusCode::DISCONNECT:
		mState = State::DISCONNECTED;
		break;
	default:
		break;
	}
}

void PhotonLib::onOperationResponse(const OperationResponse& opResponse)
{
	JString msg("Test");
	switch(opResponse.getOperationCode())
	{
	case 1:
        	cout << opResponse.getParameters().toString() << endl;
		//msg = ((ExitGames::Common::ValueObject<JString>)opResponse.getParameterForCode(1)).getDataCopy();
		//msg = opResponse.getParameters().getValue(1);
		msg = opResponse.toString(true, true, true);
		showMsg(msg);
		mState = State::SENDED;
		break;
	default:
		showMsg(opResponse.toString(true, true, true));
		break;
	}
}

void PhotonLib::onEvent(const EventData& eventData){

}

void PhotonLib::debugReturn(ExitGames::Common::DebugLevel::DebugLevel debugLevel, const ExitGames::Common::JString& string){

}

void NotifyPhotonServer(char * photonIP)
{	
	char photonIPport[20];
    	strcpy(photonIPport, photonIP);
    	strcat(photonIPport, ":5055");
	char * airstrikeIP = GetLocalIp();
	printf("AirStrike IP adress = %s\n",airstrikeIP);
	printf("Photon IP adress = %s\n",photonIPport);

        JString ipAddrPhoton(photonIPport);
        PhotonLib * photonLib = new PhotonLib();
        photonLib->startwork(ipAddrPhoton, airstrikeIP);
        delete photonLib;
}

char* GetLocalIp()    
{          
    int MAXINTERFACES=16;    
    char *ip;    
    int fd, intrface, retn = 0;      
    struct ifreq buf[MAXINTERFACES];      
    struct ifconf ifc;      
  
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0){      
        ifc.ifc_len = sizeof(buf);      
        ifc.ifc_buf = (caddr_t)buf;      
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)){      
            intrface = ifc.ifc_len / sizeof(struct ifreq);       
            while (intrface-- > 0){      
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface]))){      
                    ip=(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));      
                    break;    
                }                          
            }    
        }      
        close (fd);      
        return ip;      
    }    
}

void showMsg(JString s){
	int size = s.length();
	int i;
	for(i=0;i<size;i++)printf("%c",(char) s.charAt(i));
	printf("\n");
}