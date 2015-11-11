#ifndef ASC_HPP
#define ASC_HPP

//
// audio object types
//

#define ASC_OBJTYPE_NULL		0
#define ASC_OBJTYPE_AAC_MAIN		1
#define ASC_OBJTYPE_AAC_LC		2
#define ASC_OBJTYPE_AAC_SSR             3
#define ASC_OBJTYPE_AAC_LTP             4
#define ASC_OBJTYPE_SBR                 5
#define ASC_OBJTYPE_AAC_SCALABLE        6
#define ASC_OBJTYPE_TWINVQ              7
#define ASC_OBJTYPE_CELP                8
#define ASC_OBJTYPE_HXVC                9
#define ASC_OBJTYPE_RESERVED1           10
#define ASC_OBJTYPE_RESERVED2           11
#define ASC_OBJTYPE_TTSI                12
#define ASC_OBJTYPE_MAIN_SYNTHESIS      13
#define ASC_OBJTYPE_WAVETABLE_SYNTHESIS 14
#define ASC_OBJTYPE_GENERAL_MIDI        15
#define ASC_OBJTYPE_ALGORITHMIC_SYNTHESIS_AND_AUDIO_EFFECTS 16
#define ASC_OBJTYPE_ER_AAC_LC           17
#define ASC_OBJTYPE_RESERVED3           18
#define ASC_OBJTYPE_ER_AAC_LTP          19
#define ASC_OBJTYPE_ER_AAC_SCALABLE     20
#define ASC_OBJTYPE_ER_TWINVQ           21
#define ASC_OBJTYPE_ER_BSAC             22
#define ASC_OBJTYPE_ER_AAC_LD           23
#define ASC_OBJTYPE_ER_CELP             24
#define ASC_OBJTYPE_ER_HVXC             25
#define ASC_OBJTYPE_ER_HILN             26
#define ASC_OBJTYPE_ER_PARAMETRIC       27
#define ASC_OBJTYPE_SSC                 28
#define ASC_OBJTYPE_PS                  29
#define ASC_OBJTYPE_MPEG_SURROUND       30
#define ASC_OBJTYPE_ESCAPE_VALUE        31
#define ASC_OBJTYPE_LAYER_1             32
#define ASC_OBJTYPE_LAYER_2             33
#define ASC_OBJTYPE_LAYER_3             34
#define ASC_OBJTYPE_DST                 35
#define ASC_OBJTYPE_ALS                 36
#define ASC_OBJTYPE_SLS                 37
#define ASC_OBJTYPE_SLS_NON_CORE        38
#define ASC_OBJTYPE_ER_AAC_ELD          39
#define ASC_OBJTYPE_SMR_SIMPLE          40
#define ASC_OBJTYPE_SMR_MAIN            41
#define ASC_OBJTYPE_USAC_NO_SBR         42
#define ASC_OBJTYPE_SAOC                43
#define ASC_OBJTYPE_LD_MPEG_SURROUND    44
#define ASC_OBJTYPE_USAC                45

//
// sampling frequencies
//

#define ASC_SF_96000      0
#define ASC_SF_88200      1
#define ASC_SF_64000      2
#define ASC_SF_48000      3
#define ASC_SF_44100      4
#define ASC_SF_32000      5
#define ASC_SF_24000      6
#define ASC_SF_22050      7
#define ASC_SF_16000      8
#define ASC_SF_12000      9
#define ASC_SF_11025      10
#define ASC_SF_8000       11
#define ASC_SF_7350       12
#define ASC_SF_RESERVED_1 13
#define ASC_SF_RESERVED_2 14
#define ASC_SF_CUSTOM     15

//
// channel configurations
//

#define ASC_CHAN_AOT_SPEC         0
#define ASC_CHAN_FC               1
#define ASC_CHAN_FLR              2
#define ASC_CHAN_FCLR             3
#define ASC_CHAN_FCLR_BC          4
#define ASC_CHAN_FCLR_BLR         5
#define ASC_CHAN_FCLR_BLR_LFE     6
#define ASC_CHAN_FCLR_SLR_BLR_LFE 7
#define ASC_CHAN_RESERVED         8

#endif
