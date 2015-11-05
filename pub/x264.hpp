#ifndef __X264_HPP__
#define __X264_HPP__

#define NAL_UNKNOWN         0x0
#define NAL_TYPE_SLICE      0x1
#define NAL_SLICE_DPA       0x2
#define NAL_SLICE_DPB       0x3
#define NAL_SLICE_DPC       0x4
#define NAL_TYPE_SLICE_IDR  0x5
#define NAL_TYPE_SEI        0x6
#define NAL_TYPE_SPS        0x7
#define NAL_TYPE_PPS        0x8

#define FRAME_TYPE_UNKNOWN  0x0
#define FRAME_TYPE_I        0x1
#define FRAME_TYPE_P        0x2
#define FRAME_TYPE_B        0x3

typedef unsigned int uint32_t;
typedef struct _NalUnit
{
        int type;
        int size;
        char *data;
} NalUnit;

class Util264
{
public:
        static bool DecodeSps(char * buf,unsigned int nLen,int &width,int &height);
        static int GetFrameType(const NalUnit *pNalUnit);
private:
        static unsigned int Ue(char *_pBuf, unsigned int _nLen, unsigned int &_nStartBit);
        static int Se(char *_pBuf, unsigned int _nLen, unsigned int &_nStartBit);
        static uint32_t u(unsigned int _nBitCount, char * _pBuf, unsigned int &_nStartBit);
};

#endif
