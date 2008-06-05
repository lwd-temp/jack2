
#include "types.h"
#include "JackConstants.h"
#include "JackMidiPort.h"

#include <string>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

namespace Jack
{
	typedef struct _session_params session_params_t;
	typedef struct _packet_header packet_header_t;
	typedef struct _midi_portbuf_desc midi_portbuf_desc_t;
	typedef struct sockaddr socket_address_t;
	typedef struct in_addr address_t;
	typedef jack_default_audio_sample_t sample_t;


//session params ******************************************************************************

	struct _session_params
	{
		char fPacketType[7];			//packet type ('param')
		char fProtocolVersion;			//version
		int fPacketID;				//indicates the packet type
		char fMasterNetName[256];		//master hostname (network)
		char fSlaveNetName[256];		//slave hostname (network)
		size_t fMtu;				//connection mtu
		size_t fID;				//slave's ID
		int fSendAudioChannels;			//number of master->slave channels
		int fReturnAudioChannels;		//number of slave->master channels
		int fSendMidiChannels;			//number of master->slave midi channels
		int fReturnMidiChannels;		//number of slave->master midi channels
		size_t fSampleRate;			//session sample rate
		size_t fPeriodSize;			//period size
		size_t fFramesPerPacket;		//complete frames per packet
		size_t fBitdepth;			//samples bitdepth (unused)
		char fName[JACK_CLIENT_NAME_SIZE];	//slave's name
	};


//net status **********************************************************************************

	enum  _net_status
	{
		SOCKET_ERROR,
		CONNECT_ERROR,
		NET_ERROR,
		SEND_ERROR,
		RECV_ERROR,
		CONNECTED,
		ROLLING
	};

	typedef enum _net_status net_status_t;


//sync packet type ****************************************************************************

	enum _sync_packet_type
	{
		INVALID,		//...
		SLAVE_AVAILABLE,	//a slave is available
		SLAVE_SETUP,		//slave configuration
		START_MASTER,		//slave is ready, start master
		START_SLAVE,		//master is ready, activate slave
		KILL_MASTER		//master must stop
	};

	typedef enum _sync_packet_type sync_packet_type_t;


//packet header *******************************************************************************

	struct _packet_header
	{
		char fPacketType[7];		//packet type ( 'headr' )
		char fDataType;			//a for audio, m for midi
		char fDataStream;		//s for send, r for return
		size_t fID;			//to identify the slave
		size_t fBitdepth;		//bitdepth of the data samples
		size_t fMidiDataSize;		//size of midi data (if packet is 'midi typed') in bytes
		size_t fNMidiPckt;		//number of midi packets of the cycle
		size_t fCycle;			//process cycle counter
		size_t fSubCycle;		//midi/audio subcycle counter
		char fIsLastPckt;		//is it the last packet of a given cycle ('y' or 'n')
		char fFree[13];             	//unused
	};

//midi data ***********************************************************************************

	class NetMidiBuffer
	{
		private:
			int fNPorts;
			size_t fMaxBufsize;
			int fMaxPcktSize;
			//data
			char* fBuffer;
			char* fNetBuffer;
		public:
			NetMidiBuffer ( session_params_t* params, size_t nports, char* net_buffer );
			~NetMidiBuffer();

			JackMidiBuffer** fPortBuffer;

			void Reset();
			size_t GetSize();
			//utility
			void DisplayEvents();
			//jack<->buffer
			int RenderFromJackPorts();
			int RenderToJackPorts();
			//network<->buffer
			int RenderFromNetwork ( size_t subcycle, size_t copy_size );
			int RenderToNetwork ( size_t subcycle, size_t copy_size );
	};

// audio data *********************************************************************************

	class NetAudioBuffer
	{
		private:
			int fNPorts;
			jack_nframes_t fPeriodSize;
			jack_nframes_t fSubPeriodSize;
			size_t fSubPeriodBytesSize;
			char* fNetBuffer;
		public:
			NetAudioBuffer ( session_params_t* params, size_t nports, char* net_buffer );
			~NetAudioBuffer();

			sample_t** fPortBuffer;	

			size_t GetSize();
			//jack<->buffer
			void  RenderFromJackPorts ( size_t subcycle );
			void RenderToJackPorts ( size_t subcycle );
	};

//utility *************************************************************************************

	//n<-->h functions
	void SessionParamsHToN ( session_params_t* params );
	void SessionParamsNToH ( session_params_t* params );
	void PacketHeaderHToN ( packet_header_t* header );
	void PacketHeaderNToH ( packet_header_t* header );
	//display session parameters
	void SessionParamsDisplay ( session_params_t* params );
	//display packet header
	void PacketHeaderDisplay ( packet_header_t* header );
	//get the packet type from a sesion parameters
	sync_packet_type_t GetPacketType ( session_params_t* params );
	//set the packet type in a session parameters
	int SetPacketType ( session_params_t* params, sync_packet_type_t packet_type );
	//step of network initialization
	size_t SetFramesPerPacket ( session_params_t* params );
	//get the midi packet number for a given cycle
	size_t GetNMidiPckt ( session_params_t* params, size_t data_size );
	//set the recv timeout on a socket
	int SetRxTimeout ( int* sockfd, session_params_t* params );
	//check if 'next' packet is really the next after 'previous'
	bool IsNextPacket ( packet_header_t* previous, packet_header_t* next, size_t subcycles );
}
