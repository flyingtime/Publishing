//
// pub.hpp
//

#ifndef __PUB_HPP__
#define __PUB_HPP__

#include "rtmp.h"
#include "rtmp_sys.h"
#include "amf.h"

#include "asc.hpp"

#include <cstring>

#define FILE_BUFFER_SIZE (1024 * 1024 * 15)
#define DEFAULT_H264_FPS 30
#define FLV_CODECID_H264 7
#define DEFAULT_ADPCM_FPS 58
#define DEFAULT_AAC_FPS 47

// format of SoundData
#define SOUND_FORMAT_LINEAR_PCM_PE   0  // Linear PCM, platform endian
#define SOUND_FORMAT_ADPCM           1  // ADPCM
#define SOUND_FORMAT_MP3             2  // MP3
#define SOUND_FORMAT_LINEAR_PCM_LE   3  // Linear PCM, little endian
#define SOUND_FORMAT_NELLYMOSER_16K  4  // Nellymoser 16 kHz mono
#define SOUND_FORMAT_NELLYMOSER_8K   5  // Nellymose 8 kHz mono
#define SOUND_FORMAT_NELLYMOSER      6  // Nellymoser
#define SOUND_FORMAT_G711A           7  // G.711 A-law logarithmic PCM
#define SOUND_FORMAT_G711U           8  // G.711 mu-law logarithmic PCM
#define SOUND_FORMAT_RESERVED        9  // reserved
#define SOUND_FORMAT_AAC             10 // AAC
#define SOUND_FORMT_SPEEX            11 // Speex
#define SOUND_FORMAT_MP3_8K          14 // MP3 8 kHz
#define SOUND_FORMAT_DEV_SPEC        15 // Device-specific sound

// sampling rate. The following values are defined
#define SOUND_RATE_5_5K  0 // 5.5 kHz
#define SOUND_RATE_11K   1 // 11 kHz
#define SOUND_RATE_22K   2 // 22 kHz
#define SOUND_RATE_44K   3 // 44 kHz

// size of each audio sample
#define SOUND_SIZE_8BIT   0 // 8-bit samples
#define SOUND_SIZE_16BIT  1 // 16-bit samples

// mono or stereo sound
#define SOUND_TYPE_MONO   0 // Mono sound
#define SOUND_TYPE_STEREO 1 // Stereo sound

// AAC specific field
#define SOUND_AAC_TYPE_SEQ_HEADER 0 // AAC sequence header
#define SOUND_AAC_TYPE_RAW        1 // AAC raw

typedef struct _NalUnit
{
        int type;
        int size;
        char *data;
} NalUnit;

typedef struct _RtmpMetadata
{
        unsigned int nWidth;
        unsigned int nHeight;

        // define fps and bps
        unsigned int nFrameRate;
        unsigned int nVideoDataRate;

        unsigned int nSpsLen;
        char sps[1024];
        unsigned int nPpsLen;
        char pps[1024];

        // audio
        bool bHasAudio;
        unsigned int nAudioSampleRate;
        unsigned int nAudioSampleSize;
        unsigned int nAudioChannels;
        char *pAudioSpecCfg;
        unsigned int nAudioSpecCfgLen;
} RtmpMetadata;

#define ADTS_HEADER_LENGTH 7 // bytes

typedef struct _AdtsHeader
{
        unsigned int nSyncWord;
        unsigned int nId;
        unsigned int nLayer;
        unsigned int nProtectionAbsent;
        unsigned int nProfile;
        unsigned int nSfIndex;
        unsigned int nPrivateBit;
        unsigned int nChannelConfiguration;
        unsigned int nOriginal;
        unsigned int nHome;

        unsigned int nCopyrightIdentificationBit;
        unsigned int nCopyrigthIdentificationStart;
        unsigned int nAacFrameLength;
        unsigned int nAdtsBufferFullness;

        unsigned int nNoRawDataBlocksInFrame;
} AdtsHeader;

class RtmpStream
{
public:
        RtmpStream(void);
        ~RtmpStream(void);
        // TODO define as virtual functions
        bool Connect(const char *url);
        void Close();
        bool SendH264File(const char *pFileName);
        bool SendAacFile(const char *pFileName);
protected:
        bool LoadFile(const char *pFileName);
        bool SendMetadata(RtmpMetadata * pMeta);
        bool SendH264Packet(const char *data, unsigned int size, bool bIsKeyFrame, unsigned int nTimeStamp);
        bool ReadOneNaluFromBuf(NalUnit & nalu);
        bool GetNextNalUnit(unsigned int nStart, unsigned int &nDelimiter, unsigned int &nNalu);
        bool SendPacket(unsigned int nPacketType, const char *data, unsigned int size, unsigned int nTimestamp);
        void PrintNalUnit(const NalUnit *pNalu);

        bool SendAdpcmPacket(const char *pData, unsigned int nSize, unsigned int nTimeStamp);
        bool SendAdpcmPacket(const char *pData, unsigned int nSize, unsigned int nTimeStamp,
                             char chSoundRate, char chSoundSize, char chSoundType);

        bool GetAacHeader(const char *pBuffer, AdtsHeader &header);
        bool SendAacPacket(const char *pData, unsigned int nSize, unsigned int nTimeStamp);
        bool SendAacPacket(const char *pData, unsigned int nSize, unsigned int nTimeStamp,
                           char chSoundRate, char chSoundSize, char chSoundType);
        bool SendAacConfig(char chSoundRate, char chSoundSize, char chSoundType, const AdtsHeader *pAdts = nullptr);
protected:
        RTMP *m_pRtmp;
        char *m_pFileBuf;
        unsigned int m_nFileBufSize;
        unsigned int m_nCurPos;
        unsigned int m_nFrameRate;
};

static inline char * put_byte(char *output, uint8_t nVal)
{
        output[0] = nVal;
        return output + 1;
}

static inline char * put_be16(char *output, uint16_t nVal)
{
        output[1] = nVal & 0xff;
        output[0] = nVal >> 8;
        return output + 2;
}

static inline char * put_be24(char *output,uint32_t nVal)
{
        output[2] = nVal & 0xff;
        output[1] = nVal >> 8;
        output[0] = nVal >> 16;
        return output + 3;
}

static inline char * put_be32(char *output, uint32_t nVal)
{
        output[3] = nVal & 0xff;
        output[2] = nVal >> 8;
        output[1] = nVal >> 16;
        output[0] = nVal >> 24;
        return output + 4;
}

static inline char *  put_be64( char *output, uint64_t nVal )
{
        output = put_be32(output, nVal >> 32);
        output = put_be32(output, nVal );
        return output;
}

static inline char * put_amf_string( char *c, const char *str )
{
        uint16_t len = strlen(str);
        c = put_be16(c, len);
        memcpy(c, str, len);
        return c + len;
}

static inline char * put_amf_double(char *c, double d)
{ 
        *c++ = AMF_NUMBER;  /* type: Number */
        { 
                char *ci, *co;
                ci = (char *)&d;
                co = (char *)c;
                co[0] = ci[7];
                co[1] = ci[6];
                co[2] = ci[5];
                co[3] = ci[4];
                co[4] = ci[3];
                co[5] = ci[2];
                co[6] = ci[1];
                co[7] = ci[0];
        } 
        return c + 8;
}

#endif
