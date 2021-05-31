#include "ofxE131Client.h"

#ifdef WIN32
#include <iphlpapi.h>
#include <codecvt>
#else
#include <ifaddrs.h>
#endif

void ofxE131Client::setup(string host, string mask)
{
	ipAddress = host;
    // Universally Unique Identifier generated at:  https://www.uuidgenerator.net/
    const vector<char> cid = { char(0x71), char(0x2d), char(0xfb), char(0x4c), char(0xe6), char(0xa3), char(0x4f), char(0xeb), char(0xaa), char(0x99), char(0xe5), char(0x94), char(0x84), char(0xb4), char(0x5b), char(0x1d) };

    setSourceName("Sosolimited device");
    setCid(cid);
    setLengthFlags();
    setPriority(200); // Default top priority
    setUniverse(1);

    // Set default data send interval to be equivalent to 30 Hz
    if (framerate > 0) {
        dataSendInterval = 1.0f / framerate;
    }

    // Create a timer for timing cycling and data sending
    timer = clock();
	
    // Connect to UDP endpoint
    connectUDP();
}

void ofxE131Client::connectUDP()
{
    if (!udpSetup) {
		udp.Create();
		udp.SetEnableBroadcast(false);
		udp.SetReuseAddress(true);
		udp.SetNonBlocking(true);
		udp.SetSendBufferSize(4096);
		udp.SetTimeoutSend(1);
		udp.ConnectMcast((char*)"239.255.0.1", destPort);
    }
}

void ofxE131Client::setChannel(int channel, u_char value, int universe)
{
	setChannels(channel, &value, 1, universe);
}


void ofxE131Client::setChannels(int startChannel, u_char* values, size_t size, int universe)
{
    // Check if universe is already in our map
    if (universePackets.count(universe) == 0) {
        std::array<char, 512> emptyPayload = { { char(512) } };

        // Create packet
        UniverseData data = UniverseData{
            char(0),
            emptyPayload
        };

        universePackets[universe] = data;
    }

    setUniverse(universe);

    // Expect values between 1 - 512, inclusive
    if ((startChannel > 0) && (startChannel <= 512)) {

        auto &dataPacket = universePackets.at(universe);
		for(int channel = startChannel ; channel < startChannel+size && channel <= 512; channel++){
			dataPacket.payload.at(channel - 1) = (char)*(values+channel-startChannel);
		}
    }
    else {
        ofLog() << "Channel must be between 1 and 512 for DMX protocol. " << startChannel;
    }
}

void ofxE131Client::setUniverse(int universe)
{
    // Set header with appropriate universe, high and low byte
    try {
        sac_packet.at(113) = universe >> 8; // Grabs high byte
        sac_packet.at(114) = universe; // Just the low byte
    }
    catch (std::exception &e) {
        cout << "set universe" << endl;
    }
}

void ofxE131Client::setPriority(int priority)
{
    if ((priority >= 0) && (priority <= 200)) {
        sac_packet.at(108) = char(priority);
    }
    else {
		ofLog() << "Priority must be between 0-200";
    }
}

// This should remain the same per physical device
void ofxE131Client::setCid(const std::vector<char> cid)
{
    int length = cid.size();

    if (length != 16) {
        ofLog() << "CID must be of length 16!";
        return;
    }

    int start_index = 22;
    for (int i = 0; i < 16; i++) {
        sac_packet.at(start_index + i) = cid[i];
    }
}

// UTF-8 encoded string, null terminated
void ofxE131Client::setSourceName(std::string name)
{
    char *cstr = new char[name.length() + 1];
    std::strcpy(cstr, name.c_str());

    int max_length = strlen(cstr);

    // Field is 64 bytes, with a null terminator
    if (max_length > 63) {
        max_length = 63;
    };

    for (int i = 0; i < max_length; i++) {
        sac_packet.at(44 + i) = cstr[i];
    }
    sac_packet.at(107) = '\n';
}

void ofxE131Client::setLengthFlags()
{
    // 16-bit field with the PDU (Protocol Data Unit) length
    // encoded in the lower 12 bits
    // and 0x7 encoded in the top 4 bits

    // There are 3 PDUs in each packet (Root layer, Framing Layer, and DMP layer)

    // To calculate PDU length for RLP section, subtract last index in DMP layer
    // (637 for full payload) from the index before RLP length flag (15)

    // Set length for
    short val = 0x026e; // Index 637-15 = 622 (0x026e)
    char lowByte = 0xff & val; // Get the lower byte
    char highByte = (0x7 << 4) | (val >> 8); // bitshift so 0x7 is in the top 4 bits

    // Set length for Root Layer (RLP)

    sac_packet.at(16) = highByte;
    sac_packet.at(17) = lowByte;

    val = 0x0258; // Index 637-37 = 600 (0x0258)
    lowByte = 0xff & val; // Get the lower byte
    highByte = (0x7 << 4) | (val >> 8); // bitshift so 0x7 is in the top 4 bits

    // Set length for Framing Layer

    sac_packet.at(38) = highByte;
    sac_packet.at(39) = lowByte; // different length!

    val = 0x020B; // Index 637-114 = 523 (0x020B)
    lowByte = 0xff & val; // Get the lower byte
    highByte = (0x7 << 4) | (val >> 8); // bitshift so 0x7 is in the top 4 bits

    // Set length for DMP Layer
    sac_packet.at(115) = highByte;
    sac_packet.at(116) = lowByte;
}

// Set our data sending framerate
void ofxE131Client::setFramerate(float iFPS)
{
    if (iFPS > 0) {
        dataSendInterval = 1.0 / iFPS;
    }
    framerate = iFPS;
}

// Check if we should send data, according to our
// data framerate
bool ofxE131Client::shouldSendData(float iTime)
{
    if (!useFramerate) {
        return true;
    }
    else {
        float diff = iTime - lastDataSendTime;

        if ((diff >= dataSendInterval) && (framerate > 0)) {
            return true;
        }
    }
    return false;
}

void ofxE131Client::sendDMX()
{
    // Send for all universes
    for (auto &pair : universePackets) {
        auto universe = pair.first;
        auto &dataPacket = universePackets.at(universe);
        auto payload = dataPacket.payload;

        setUniverse(universe);

        // TODO: Probably a better way to do this.
        for (int i = 0; i < payload.size(); i++) {
            sac_packet.at(126 + i) = payload.at(i);
        }

        // Handle exceptions
        try {
            sac_packet.at(111) = dataPacket.universeSequenceNum;
			int xxx = universe >> 8 & 0x00ff;
			int yyy = universe & 0x00ff;
			//udp.ConnectMcast((char*)("239.255." + ofToString(xxx) + "." + ofToString(yyy)).c_str(), destPort);
			udp.SendAll(sac_packet.data(), packet_length);
			//udp.Close();
//            _socket->send(asio::buffer(sac_packet, packet_length));
            dataPacket.universeSequenceNum = dataPacket.universeSequenceNum + 1;
        }
        catch (std::exception &e) {
            if (!loggedException) {
                ofLog() << "Could not send sACN data - are you plugged into the device?";
                loggedException = true;
            }
        }
    }
    // Increment current universe counter.
}

// Update all strands, send data if it's time
// Update any test cycles
void ofxE131Client::update()
{
    // Use system time for timing purposes
    clock_t thisTime = clock();
    float timeDiff = (thisTime - timer) / 100000.0f;

    elapsedTime += timeDiff;

    float iTime = elapsedTime;

    // If we've enabled auto data sending, send data if
    // enough time has passed according to our data send interval
    if (autoSendingEnabled && shouldSendData(iTime)) {
        lastDataSendTime = iTime;
        sendDMX();
    }

    timer = clock();
}
