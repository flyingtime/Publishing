#include "rtmp_packet.hpp"
#include <cstring>
#include <iostream>

using namespace std;

#ifdef __STD_THREAD_SUPPORT__

//
// RtmpPacket for async mode only
//

RtmpPacket::RtmpPacket(const char *_pData, unsigned int _nLength, unsigned int _nTimestamp):
        m_buffer(_pData, _pData + _nLength),
        m_nTimestamp(_nTimestamp)
{
}

char* RtmpPacket::GetBuffer()
{
        return m_buffer.data();
}

unsigned int RtmpPacket::GetSize()
{
        return m_buffer.size();
}

unsigned int RtmpPacket::GetTimestamp()
{
        return m_nTimestamp;
}

#endif // __STD_THREAD_SUPPORT__

//
// RtmpPacketSender
//

RtmpPacketSender::RtmpPacketSender(string &_url):
        m_publishUrl(_url)
#ifdef __STD_THREAD_SUPPORT__
        ,m_queue()
        ,m_bIsAsync(false)
        ,m_threads()
#endif
{
        SetStatus(UNINITIALIZED);

        m_nTimestamp = 0;
        m_nFps = 25;

        ResetSps();
        ResetPps();
        ResetSei();

        memset(m_spsData, 0, SPS_BUFFER_SIZE);
        memset(m_ppsData, 0, PPS_BUFFER_SIZE);
        memset(m_seiData, 0, SEI_BUFFER_SIZE);

        SetStatus(INITIALIZED);
}

RtmpPacketSender::~RtmpPacketSender()
{
}

void RtmpPacketSender::ResetSps()
{
        m_sps.size = 0;
        m_sps.data = nullptr;
}

void RtmpPacketSender::ResetPps()
{
        m_pps.size = 0;
        m_pps.data = nullptr;
}

void RtmpPacketSender::ResetSei()
{
        m_sei.size = 0;
        m_sei.data = nullptr;
}

bool RtmpPacketSender::Connect()
{
        if (RtmpStream::Connect(m_publishUrl.c_str()) != true) {
                cout << "Not connected:" << m_publishUrl << endl;
                return false;
        }
        return true;
}

bool RtmpPacketSender::Connect(bool _bIsAsync)
{
        if (Connect() == false) {
                return false;
        }

#ifdef __STD_THREAD_SUPPORT__
        // async mode, all threads are managed inside
        if (_bIsAsync == true) {
                m_bIsAsync = true;
                m_threads.push_back(thread(bind(&RtmpPacketSender::SenderLoop, this)));
        }
#endif
        return true;
}

void RtmpPacketSender::Close()
{
#ifdef __STD_THREAD_SUPPORT__
        StopAllThreads();
#endif
        RtmpStream::Close();
}

#ifdef __STD_THREAD_SUPPORT__
void RtmpPacketSender::StopAllThreads()
{
        // stop sender thread
        if (m_bIsAsync == true) {
                m_bExitNow = true;
                for (auto &thr : m_threads) {
                        thr.join();
                }
        }
}

void RtmpPacketSender::SenderLoop()
{
        unsigned int nIntval = 0;

        while (m_bExitNow.load() == false) {
                RtmpPacket packet(m_queue.Pop());
                bool bStatus = SendPacket(RTMP_PACKET_TYPE_VIDEO, packet.GetBuffer(), packet.GetSize(), packet.GetTimestamp());
                if (bStatus == false) {
                        cout << "Error:packet not sent :" << m_publishUrl << endl;
                        // TODO reconnect and send configuration
                }
#ifdef __PRINT_QUEUE_STATE__
                cout << "Q_LEN #" << m_queue.Size() << " TS #" << packet.GetTimestamp() << " : " << m_publishUrl << endl;
#endif
        }
}
#endif // __STD_THREAD_SUPPORT__

void RtmpPacketSender::Sps(const char *_pData, unsigned int _nLength)
{
        if (_nLength > SPS_BUFFER_SIZE) {
                cout << "Warning: buffer is full for sps length " << _nLength << endl;
        }
        memcpy(m_spsData, _pData, _nLength);
        m_sps.size = _nLength;
        m_sps.data = m_spsData;
}

void RtmpPacketSender::Pps(const char *_pData, unsigned int _nLength)
{
        if (_nLength > PPS_BUFFER_SIZE) {
                cout << "Warning: buffer is full for pps length " << _nLength << endl;
        }
        memcpy(m_ppsData, _pData, _nLength);
        m_pps.size = _nLength;
        m_pps.data = m_ppsData;
}

void RtmpPacketSender::Sei(const char *_pData, unsigned int _nLength)
{
        if (_nLength > SEI_BUFFER_SIZE) {
                cout << "Warning: buffer is full for sei length " << _nLength << endl;
        }
        memcpy(m_seiData, _pData, _nLength);
        m_sei.size = _nLength;
        m_sei.data = m_seiData;
}

bool RtmpPacketSender::SendIdrOnly(const char *_pData, unsigned int _nLength, unsigned int _nCompositionTime)
{
        m_nTimestamp += (1000 / m_nFps);

#ifdef __STD_THREAD_SUPPORT__
        if (m_bIsAsync == true) {
                int i = 0;
                char *body = new char[_nLength + 9];

                // key frame
                body[i++] = 0x17;
                body[i++] = 0x01;
                body[i++] = _nCompositionTime >> 16;
                body[i++] = _nCompositionTime >> 8;
                body[i++] = _nCompositionTime & 0xff;

                i += WriteNalDataToBuffer(&body[i], _pData, _nLength);
                m_queue.Push(RtmpPacket(body, i, m_nTimestamp));
                delete[] body;
                return true;
        }
#endif

        return SendH264Packet(_pData, _nLength, true, m_nTimestamp, _nCompositionTime);
}

bool RtmpPacketSender::SendConfig()
{
        int i = 0;
        char body[1024];

        // sanity check
        if (m_sps.size == 0 || m_pps.size == 0) {
                cout << "Sanity check failed: empty SPS or PPS" << endl;
                return false;
        }

        // header
        body[i++] = 0x17; // 1:keyframe  7:AVC
        body[i++] = 0x00; // AVC sequence header

        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00; // fill in 0;

        // AVCDecoderConfigurationRecord.
        body[i++] = 0x01; // configurationVersion
        body[i++] = m_spsData[1]; // AVCProfileIndication
        body[i++] = m_spsData[2]; // profile_compatibility
        body[i++] = m_spsData[3]; // AVCLevelIndication
        body[i++] = 0xff; // lengthSizeMinusOne

        // sps nums
        body[i++] = 0xE1; //&0x1f
        // sps data length
        body[i++] = m_sps.size >> 8;
        body[i++] = m_sps.size & 0xff;
        // sps data
        memcpy(&body[i], m_sps.data, m_sps.size);
        i = i + m_sps.size;

        // pps nums
        body[i++] = 0x01; //&0x1f
        // pps data length
        body[i++] = m_pps.size >> 8;
        body[i++] = m_pps.size & 0xff;
        // sps data
        memcpy(&body[i], m_pps.data, m_pps.size);
        i = i + m_pps.size;

        return SendPacket(RTMP_PACKET_TYPE_VIDEO, (char *)body, i, 0);
}

//
// Notice : Because we want to send SPS/PPS/SEI/IDR in one packet, this function almost re-writes the parent member
//            'SendH264Packet()' and it will finally call 'SendPacket()' directly.
//
bool RtmpPacketSender::SendIdrAll(const char *_pData, unsigned int _nLength, unsigned int _nCompositionTime)
{
        // at least we have ever received SPS
        if (m_sps.size == 0) {
                SendIdrOnly(_pData, _nLength);
        }

        if (_pData == nullptr && _nLength < 11) {
                return false;
        }

        // this is a bit different, the body consists of [header][nalu_size][nalu][nalu_size][nalu][...] 
        int nNalUnitNum = 2 + (m_sei.size > 0 ? 1 : 0) + 1; // SPS+PPS + SEI + IDR
        int nSize = m_sps.size + m_pps.size + m_sei.size + _nLength + 5 + nNalUnitNum * 4;
        char *body = new char[nSize];
        int i = 0;
        bool bRet;

        //
        // header (5bytes)
        //
        
        body[i++] = 0x17; // 1:Iframe 7:AVC
        body[i++] = 0x01; // AVC NALU

        // composition time adjustment
        body[i++] = _nCompositionTime >> 16;
        body[i++] = _nCompositionTime >> 8;
        body[i++] = _nCompositionTime & 0xff;

        //
        // NAL Units (at most 4)
        //

        unsigned int nBytes;

        // SPS & PPS
        nBytes = WriteNalUnitToBuffer(&body[i], &m_sps);
        i += nBytes;
        nBytes = WriteNalUnitToBuffer(&body[i], &m_pps);
        i += nBytes;

        // SEI
        if (m_sei.size > 0) {
                nBytes = WriteNalUnitToBuffer(&body[i], &m_sei);
                i += nBytes;
        }

        // IDR
        WriteNalDataToBuffer(&body[i], _pData, _nLength);

        // send and cleanup
        m_nTimestamp += (1000 / m_nFps);

#ifdef __STD_THREAD_SUPPORT__
        if (m_bIsAsync == true) {
                m_queue.Push(RtmpPacket(body, nSize, m_nTimestamp));
                delete[] body;
                return true;
        }
#endif

        bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, nSize, m_nTimestamp);
        // refresh SPS
        ResetSps();
        ResetPps();
        ResetSei();

        delete[] body;
        return bRet;
}

unsigned int RtmpPacketSender::WriteNalDataToBuffer(char *_pBuffer, const char *_pData, unsigned int _nLength)
{
        int i = 0;

        // NALU size
        _pBuffer[i++] = _nLength >> 24;
        _pBuffer[i++] = _nLength >> 16;
        _pBuffer[i++] = _nLength >> 8;
        _pBuffer[i++] = _nLength & 0xff;

        // NALU data
        memcpy(&_pBuffer[i], _pData, _nLength);
        return _nLength + 4;
}

unsigned int RtmpPacketSender::WriteNalUnitToBuffer(char *_pBuffer, NalUnit *_pNalu)
{
        return WriteNalDataToBuffer(_pBuffer, _pNalu->data, _pNalu->size);
}

bool RtmpPacketSender::SendNonIdr(const char *_pData, unsigned int _nLength, unsigned int _nCompositionTime)
{
        m_nTimestamp += (1000 / m_nFps);

#ifdef __STD_THREAD_SUPPORT__
        if (m_bIsAsync == true) {
                int i = 0;
                char *body = new char[_nLength + 9];

                // not a key frame
                body[i++] = 0x27;
                body[i++] = 0x01;
                body[i++] = _nCompositionTime >> 16;
                body[i++] = _nCompositionTime >> 8;
                body[i++] = _nCompositionTime & 0xff;

                i += WriteNalDataToBuffer(&body[i], _pData, _nLength);
                m_queue.Push(RtmpPacket(body, i, m_nTimestamp));
                delete[] body;
                return true;
        }
#endif

        return SendH264Packet(_pData, _nLength, false, m_nTimestamp, _nCompositionTime);
}

void RtmpPacketSender::SetStatus(RtmpPacketSenderStatusType _nStatus)
{
        m_nStatus = _nStatus;
}
