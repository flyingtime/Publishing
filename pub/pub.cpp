//
//  pub.cpp - RTMP publish sample
//

#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cstdio>

#include "pub.hpp"

using namespace std;

RtmpStream::RtmpStream(void):
        m_pRtmp(nullptr),
        m_nFileBufSize(0),
        m_nCurPos(0)
{
        m_pFileBuf = new char[FILE_BUFFER_SIZE];
        memset(m_pFileBuf, 0, FILE_BUFFER_SIZE);
        m_pRtmp = RTMP_Alloc();
        RTMP_Init(m_pRtmp);
}

RtmpStream::~RtmpStream(void)
{
        Close();
        delete[] m_pFileBuf;
}

bool RtmpStream::Connect(const char *_url)
{
        if (RTMP_SetupURL(m_pRtmp, (char *)_url) == 0) {
                return false;
        }
        RTMP_EnableWrite(m_pRtmp);
        if (RTMP_Connect(m_pRtmp, nullptr) == 0) {
                return false;
        }
        if (RTMP_ConnectStream(m_pRtmp, 0) == 0) {
                return false;
        }
        if (RTMP_IsConnected(m_pRtmp) == 0) {
                return false;
        }
        return true;
}

void RtmpStream::Close()
{
        if (m_pRtmp) {
                RTMP_Close(m_pRtmp);
                RTMP_Free(m_pRtmp);
                m_pRtmp = NULL;
        }
}

bool RtmpStream::SendPacket(unsigned int _nPacketType, const char *_data, unsigned int _size, unsigned int _nTimestamp)
{
        if (m_pRtmp == nullptr) {
                return 0;
        }

        RTMPPacket packet;
        RTMPPacket_Reset(&packet);
        RTMPPacket_Alloc(&packet, _size);

        packet.m_packetType = _nPacketType;
        packet.m_nChannel = 0x04;
        packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
        packet.m_nTimeStamp = _nTimestamp;
        packet.m_nInfoField2 = m_pRtmp->m_stream_id;
        packet.m_nBodySize = _size;
        memcpy(packet.m_body, _data, _size);

        int nRet = RTMP_SendPacket(m_pRtmp, &packet, 0);
        RTMPPacket_Free(&packet);
        if (nRet == 0) {
                return false;
        }
        return true;
}

bool RtmpStream::SendMetadata(RtmpMetadata *_pMeta)
{
        if (_pMeta == nullptr) {
                return false;
        }
        
        char body[1024] = {0};
        char *p = (char *)body;
        
        p = put_byte(p, AMF_STRING);
        p = put_amf_string(p, "@setDataFrame");

        p = put_byte(p, AMF_STRING);
        p = put_amf_string(p, "onMetaData");
        
        p = put_byte(p, AMF_OBJECT);
        p = put_amf_string(p, "copyright");
        p = put_byte(p, AMF_STRING);
        p = put_amf_string(p, "pub");

        p = put_amf_string(p, "width");
        p = put_amf_double(p, _pMeta->nWidth);

        p = put_amf_string(p, "height");
        p = put_amf_double(p, _pMeta->nHeight);

        p = put_amf_string(p, "framerate" );
        p = put_amf_double(p, _pMeta->nFrameRate);

        p = put_amf_string(p, "videocodecid" );
        p = put_amf_double(p, FLV_CODECID_H264 );

        p = put_amf_string(p, "" );
        p = put_byte(p, AMF_OBJECT_END  );

        int index = p - body;

        SendPacket(RTMP_PACKET_TYPE_INFO, (char *)body, index ,0);

        int i = 0;
        body[i++] = 0x17; // 1:keyframe  7:AVC
        body[i++] = 0x00; // AVC sequence header

        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00; // fill in 0;

        // AVCDecoderConfigurationRecord.
        body[i++] = 0x01; // configurationVersion
        body[i++] = _pMeta->sps[1]; // AVCProfileIndication
        body[i++] = _pMeta->sps[2]; // profile_compatibility
        body[i++] = _pMeta->sps[3]; // AVCLevelIndication
        body[i++] = 0xff; // lengthSizeMinusOne

        // sps nums
        body[i++] = 0xE1; //&0x1f
        // sps data length
        body[i++] = _pMeta->nSpsLen >> 8;
        body[i++] = _pMeta->nSpsLen & 0xff;
        // sps data
        memcpy(&body[i], _pMeta->sps, _pMeta->nSpsLen);
        i= i + _pMeta->nSpsLen;

        // pps nums
        body[i++] = 0x01; //&0x1f
        // pps data length
        body[i++] = _pMeta->nPpsLen >> 8;
        body[i++] = _pMeta->nPpsLen & 0xff;
        // sps data
        memcpy(&body[i], _pMeta->pps, _pMeta->nPpsLen);
        i = i + _pMeta->nPpsLen;

        return SendPacket(RTMP_PACKET_TYPE_VIDEO, (char *)body, i, 0);
}

bool RtmpStream::SendH264Packet(const char *_data, unsigned int _size, bool _bIsKeyFrame, unsigned int _nTimeStamp, unsigned int _nCompositionTime)
{
        if (_data == nullptr && _size < 11) {
                return false;
        }

        char *body = new char[_size + 9];

        int i = 0;
        if (_bIsKeyFrame) {
                body[i++] = 0x17; // 1:Iframe 7:AVC
        } else {
                body[i++] = 0x27; // 2:Pframe 7:AVC
        }
        body[i++] = 0x01; // AVC NALU

        // composition time adjustment
        body[i++] = _nCompositionTime >> 16;
        body[i++] = _nCompositionTime >> 8;
        body[i++] = _nCompositionTime & 0xff;

        // NALU size
        body[i++] = _size >> 24;
        body[i++] = _size >> 16;
        body[i++] = _size >> 8;
        body[i++] = _size & 0xff;

        // NALU data
        memcpy(&body[i], _data, _size);

        bool bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + _size, _nTimeStamp);
        delete[] body;
        return bRet;
}

bool RtmpStream::SendH264File(const char *_pFileName)
{
        if (LoadFile(_pFileName) != true) {
                return false;
        }

        RtmpMetadata meta;
        memset(&meta, 0, sizeof(RtmpMetadata));

        NalUnit nalu;
        // read sps
        if (ReadOneNaluFromBuf(nalu) == false) {
                return false;
        }
        meta.nSpsLen = nalu.size;
        memcpy(meta.sps, nalu.data, nalu.size);

        // read pps
        if (ReadOneNaluFromBuf(nalu) == false) {
                return false;
        }
        meta.nPpsLen = nalu.size;
        memcpy(meta.pps, nalu.data, nalu.size);

        int width, height;
        Util264::DecodeSps(meta.sps, meta.nSpsLen, width, height);
        meta.nWidth = width;
        meta.nHeight = height;
        // TODO fps data from 264 stream
        meta.nFrameRate = DEFAULT_H264_FPS;
        m_nFrameRate = meta.nFrameRate;

        cout << "[debug] width=" << width << " height=" << height << endl;

        // send metadata
        SendMetadata(&meta);

        unsigned int tick = 0;
        unsigned int n = 0;
        while (ReadOneNaluFromBuf(nalu)) {
                // notice : for a normal h264 stream, we have 1 sps
                // and 1 pps as initial NALUs, so skip others
                if (nalu.type == 0x06 || nalu.type == 0x07 || nalu.type == 0x08) {
                        cout << " == skip type " << nalu.type << " ==" << endl;
                        continue;
                }

                bool bKeyFrame = (nalu.type == 0x05) ? true : false;

                // send h264 frames
                SendH264Packet(nalu.data, nalu.size, bKeyFrame, tick);
                msleep(1000 / meta.nFrameRate);
                tick += 1000 / meta.nFrameRate;
        }

        return true;
}

bool RtmpStream::GetNextNalUnit(unsigned int _nStart, unsigned int &_nDelimiter, unsigned int &_nNalu)
{
        unsigned int nSeqZeroByte = 0;
        unsigned int i = _nStart;
        for (; i < m_nFileBufSize; i++) {
                if (m_pFileBuf[i] == 0x00) {
                        nSeqZeroByte++;
                } else if (m_pFileBuf[i] == 0x01) {
                        if (nSeqZeroByte >= 2 && nSeqZeroByte <= 3) {
                                // 00 00 01 & 00 00 00 01
                                _nDelimiter = i - nSeqZeroByte;
                                _nNalu = i + 1;
                                if (_nNalu >= m_nFileBufSize) {
                                        cout << "[warning] last NALU payload not valid" << endl;
                                }
                                return true;
                        } else if (nSeqZeroByte > 3) {
                                cout << "[error] missing 4-byte seq "<< nSeqZeroByte << "@" << i << endl;
                        }
                        nSeqZeroByte = 0;
                } else {
                        nSeqZeroByte = 0;
                }
        }
        return false;
}

bool RtmpStream::ReadOneNaluFromBuf(NalUnit &nalu)
{
        unsigned int nDelimiterPos, nNaluPos;
        while (GetNextNalUnit(m_nCurPos, nDelimiterPos, nNaluPos)) {
                unsigned int nThisNalu = nNaluPos;
                if (GetNextNalUnit(nThisNalu, nDelimiterPos, nNaluPos) != true) {
                        nDelimiterPos = m_nFileBufSize;
                }
                nalu.size = nDelimiterPos - nThisNalu;
                nalu.type = m_pFileBuf[nThisNalu] & 0x01f;
                nalu.data = &m_pFileBuf[nThisNalu];

                char chFrameType;
                switch (Util264::GetFrameType(&nalu)){
                case FRAME_TYPE_UNKNOWN: chFrameType = '-'; break;
                case FRAME_TYPE_I: chFrameType = 'I'; break;
                case FRAME_TYPE_P: chFrameType = 'P'; break;
                case FRAME_TYPE_B: chFrameType = 'B'; break;
                }
                cout << "[debug] pos=" << m_nCurPos << " nalu.type=" << nalu.type << " frame.type=" << chFrameType 
                     << " nalu.size=" << nalu.size << endl;

                m_nCurPos = nDelimiterPos;
                return true;
        }
        cout << "[debug] end of publish" << endl;
        return false;
}

bool RtmpStream::SendAdpcmPacket(const char *_pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        return SendAdpcmPacket(_pData, _nSize, _nTimeStamp, SOUND_RATE_44K, SOUND_SIZE_16BIT, SOUND_TYPE_STEREO);
}

bool RtmpStream::SendAdpcmPacket(const char *_pData, unsigned int _nSize, unsigned int _nTimeStamp,
                                 char _chSoundRate, char _chSoundSize, char _chSoundType)
{
        char chHeader = 0;
        chHeader |= _chSoundType & 0x01;
        chHeader |= (_chSoundSize << 1) & 0x02;
        chHeader |= (_chSoundRate << 2) & 0x0c;
        chHeader |= (SOUND_FORMAT_ADPCM << 4) & 0xf0;

        unsigned int nSize = _nSize + 1;
        char *pData = new char[nSize]; // include header
        char *p = pData;
        *p++ = chHeader;
        memcpy(p, _pData, _nSize);
        bool bStatus = SendPacket(RTMP_PACKET_TYPE_AUDIO, pData, nSize, _nTimeStamp);
        delete[] pData;
        return bStatus;
}

bool RtmpStream::SendAacPacket(const char *_pData, unsigned int _nSize, unsigned int _nTimeStamp)
{
        return SendAacPacket(_pData, _nSize, _nTimeStamp, SOUND_RATE_44K, SOUND_SIZE_16BIT, SOUND_TYPE_STEREO);
}

bool RtmpStream::SendAacPacket(const char *_pData, unsigned int _nSize, unsigned int _nTimeStamp,
                               char _chSoundRate, char _chSoundSize, char _chSoundType)
{
        // additional 2-byte audio spec config
        char *body = new char[_nSize + 2];
        unsigned int i = 0;

        // decoder spec info bytes
        body[0] |= _chSoundType & 0x01;
        body[0] |= (_chSoundSize << 1) & 0x02;
        body[0] |= (_chSoundRate << 2) & 0x0c;
        body[0] |= (SOUND_FORMAT_AAC << 4) & 0xf0;
        // this is a sequence header
        body[1] = SOUND_AAC_TYPE_RAW;

        // send aac frame
        memcpy(&body[2], _pData, _nSize);
        bool bStatus = SendPacket(RTMP_PACKET_TYPE_AUDIO, body, _nSize + 2, _nTimeStamp);
        delete[] body;
        return bStatus;
}

bool RtmpStream::SendAacConfig(char _chSoundRate, char _chSoundSize, char _chSoundType, const AdtsHeader *_pAdts)
{
        char body[4] = {0};

        // decoder spec info bytes
        body[0] |= _chSoundType & 0x01;
        body[0] |= (_chSoundSize << 1) & 0x02;
        body[0] |= (_chSoundRate << 2) & 0x0c;
        body[0] |= (SOUND_FORMAT_AAC << 4) & 0xf0;
        // this is a sequence header
        body[1] = SOUND_AAC_TYPE_SEQ_HEADER;

        // spec config bytes
        char nProfile;
        char nSfIndex;
        char nChannel;
        if (_pAdts != nullptr) {
                nProfile = _pAdts->nProfile;
                nSfIndex = _pAdts->nSfIndex;
                nChannel = _pAdts->nChannelConfiguration;
        } else {
                nProfile = ASC_OBJTYPE_AAC_LC;
                nSfIndex = ASC_SF_44100;
                nChannel = ASC_CHAN_FLR;
        }

        unsigned int nAudioSpecConfig = 0;
        nAudioSpecConfig |= ((nProfile << 11) & 0xf800);
        nAudioSpecConfig |= ((nSfIndex << 7) & 0x0780);
        nAudioSpecConfig |= ((nChannel << 3) & 0x78);
        nAudioSpecConfig |= 0 & 0x7;
        body[2] = (nAudioSpecConfig & 0xff00) >> 8;
        body[3] = nAudioSpecConfig & 0xff;

        return SendPacket(RTMP_PACKET_TYPE_AUDIO, body, sizeof(body), 0);
}

bool RtmpStream::SendAacFile(const char *_pFileName)
{
        if (LoadFile(_pFileName) != true) {
                return false;
        }

        // first header
        AdtsHeader adtsHeader;
        if (GetAacHeader(m_pFileBuf, adtsHeader) == false) {
                cout << "Error: AAC header not valid" << endl;
                return false;
        }

        // send sequence header
        if (SendAacConfig(SOUND_RATE_44K, SOUND_SIZE_16BIT, SOUND_TYPE_STEREO, &adtsHeader) == false) {
                cout << "Error: AAC spec not sent" << endl;
                return false;
        }

        // loop
        unsigned int nTimestamp = 0, nRawAacSize, nAdtsHeaderSize;
        m_nCurPos = 0;
        m_nFrameRate = DEFAULT_AAC_FPS;
        while (m_nCurPos < m_nFileBufSize) {
                if (GetAacHeader(&m_pFileBuf[m_nCurPos], adtsHeader) == false) {
                        return false;
                }

                nAdtsHeaderSize = (adtsHeader.nProtectionAbsent == 1 ? 7 : 9);
                nRawAacSize = adtsHeader.nAacFrameLength - nAdtsHeaderSize;
                m_nCurPos += nAdtsHeaderSize;
                cout << "[debug] pos=" << m_nCurPos << " size=" << nRawAacSize << endl;

                nTimestamp += 1000 / m_nFrameRate;
                bool bStatus = SendAacPacket(&m_pFileBuf[m_nCurPos], nRawAacSize, nTimestamp);
                if (bStatus == false) {
                        cout << "[error] not sent" << endl;
                }
                m_nCurPos += nRawAacSize;

                msleep(1000 / m_nFrameRate);
        }

        cout << "Info: AAC: end of publish" << endl;
        return true;
}

bool RtmpStream::GetAacHeader(const char *_pBuffer, AdtsHeader &_header)
{
        // headers begin with FFFxxxxx...
        if ((unsigned char)_pBuffer[0] == 0xff && (((unsigned char)_pBuffer[1] & 0xf0) == 0xf0)) {
                _header.nSyncWord = (_pBuffer[0] << 4) | (_pBuffer[1] >> 4);
                _header.nId = ((unsigned int)_pBuffer[1] & 0x08) >> 3;
                _header.nLayer = ((unsigned int)_pBuffer[1] & 0x06) >> 1;
                _header.nProtectionAbsent = (unsigned int)_pBuffer[1] & 0x01;
                _header.nProfile = ((unsigned int)_pBuffer[2] & 0xc0) >> 6;
                _header.nSfIndex = ((unsigned int)_pBuffer[2] & 0x3c) >> 2;
                _header.nPrivateBit = ((unsigned int)_pBuffer[2] & 0x02) >> 1;
                _header.nChannelConfiguration = ((((unsigned int)_pBuffer[2] & 0x01) << 2) | (((unsigned int)_pBuffer[3] & 0xc0) >> 6));
                _header.nOriginal = ((unsigned int)_pBuffer[3] & 0x20) >> 5;
                _header.nHome = ((unsigned int)_pBuffer[3] & 0x10) >> 4;
                _header.nCopyrightIdentificationBit = ((unsigned int)_pBuffer[3] & 0x08) >> 3;
                _header.nCopyrigthIdentificationStart = (unsigned int)_pBuffer[3] & 0x04 >> 2;
                _header.nAacFrameLength = (((((unsigned int)_pBuffer[3]) & 0x03) << 11) |
                                            (((unsigned int)_pBuffer[4] & 0xFF) << 3) |
                                            ((unsigned int)_pBuffer[5] & 0xE0) >> 5);
                _header.nAdtsBufferFullness = (((unsigned int)_pBuffer[5] & 0x1f) << 6 | ((unsigned int)_pBuffer[6] & 0xfc) >> 2);
                _header.nNoRawDataBlocksInFrame = ((unsigned int)_pBuffer[6] & 0x03);
                return true;
        } else {
                cout << "Warning: Wrong ADTS AAC header" << endl;
                return false;
        }
}

bool RtmpStream::LoadFile(const char *_pFileName)
{
        if (_pFileName == NULL) {
                return false;
        }
        FILE *fp = fopen(_pFileName, "rb");
        if (!fp) {
                cout << "Error: could not open file" << endl;
                return false;
        }
        fseek(fp, 0, SEEK_SET);
        m_nFileBufSize = fread(m_pFileBuf, sizeof(char), FILE_BUFFER_SIZE, fp);
        if (m_nFileBufSize >= FILE_BUFFER_SIZE) {
                cout << "Warning: file is truncated" << endl;
        }
        fclose(fp);
        return true;
}

void RtmpStream::PrintNalUnit(const NalUnit *_pNalu)
{
        cout << "[NALU] pos=" << m_nCurPos << " type=" << _pNalu->type
             << " size=" << _pNalu->size << endl;
}
