#ifndef _CAFCHANNELFORMATS_H_
#define _CAFCHANNELFORMATS_H_

// from CoreAudioBaseTypes.h in the ios sdk, good luck finding it online

enum
{
  // these are identical to the WAVEFORMATEXTENSIBLE bitmask
  kAudioChannelBit_Left                       = (1U<<0),
  kAudioChannelBit_Right                      = (1U<<1),
  kAudioChannelBit_Center                     = (1U<<2),
  kAudioChannelBit_LFEScreen                  = (1U<<3),
  kAudioChannelBit_LeftSurround               = (1U<<4),
  kAudioChannelBit_RightSurround              = (1U<<5),
  kAudioChannelBit_LeftCenter                 = (1U<<6),
  kAudioChannelBit_RightCenter                = (1U<<7),
  kAudioChannelBit_CenterSurround             = (1U<<8),      // WAVE: "Back Center"
  kAudioChannelBit_LeftSurroundDirect         = (1U<<9),
  kAudioChannelBit_RightSurroundDirect        = (1U<<10),
  kAudioChannelBit_TopCenterSurround          = (1U<<11),
  kAudioChannelBit_VerticalHeightLeft         = (1U<<12),     // WAVE: "Top Front Left"
  kAudioChannelBit_VerticalHeightCenter       = (1U<<13),     // WAVE: "Top Front Center"
  kAudioChannelBit_VerticalHeightRight        = (1U<<14),     // WAVE: "Top Front Right"
  kAudioChannelBit_TopBackLeft                = (1U<<15),
  kAudioChannelBit_TopBackCenter              = (1U<<16),
  kAudioChannelBit_TopBackRight               = (1U<<17),
  kAudioChannelBit_LeftTopFront             	= (1U<<18),
  kAudioChannelBit_CenterTopFront           	= (1U<<19),
  kAudioChannelBit_RightTopFront            	= (1U<<20),
  kAudioChannelBit_LeftTopMiddle              = (1U<<21),
  kAudioChannelBit_CenterTopMiddle            = (1U<<22),
  kAudioChannelBit_RightTopMiddle             = (1U<<23),
  kAudioChannelBit_LeftTopRear                = (1U<<24),
  kAudioChannelBit_CenterTopRear              = (1U<<25),
  kAudioChannelBit_RightTopRear               = (1U<<26),
};

enum
{
  // Some channel abbreviations used below:
  // L - left
  // R - right
  // C - center
  // Ls - left surround
  // Rs - right surround
  // Cs - center surround
  // Rls - rear left surround
  // Rrs - rear right surround
  // Lw - left wide
  // Rw - right wide
  // Lsd - left surround direct
  // Rsd - right surround direct
  // Lc - left center
  // Rc - right center
  // Ts - top surround
  // Vhl - vertical height left
  // Vhc - vertical height center
  // Vhr - vertical height right
  // Ltf - left top front
  // Ctf - center top front
  // Rtf - right top front
  // Ltm - left top middle
  // Ctm - center top middle
  // Rtm - right top middle
  // Ltr - left top rear
  // Ctr - center top rear
  // Rtr - right top rear
  // Lt - left matrix total. for matrix encoded stereo.
  // Rt - right matrix total. for matrix encoded stereo.

  //  General layouts
  kAudioChannelLayoutTag_UseChannelDescriptions   = (0U<<16) | 0,     // use the array of AudioChannelDescriptions to define the mapping.
  kAudioChannelLayoutTag_UseChannelBitmap         = (1U<<16) | 0,     // use the bitmap to define the mapping.

  kAudioChannelLayoutTag_Mono                     = (100U<<16) | 1,   // a standard mono stream
  kAudioChannelLayoutTag_Stereo                   = (101U<<16) | 2,   // a standard stereo stream (L R) - implied playback
  kAudioChannelLayoutTag_StereoHeadphones         = (102U<<16) | 2,   // a standard stereo stream (L R) - implied headphone playback
  kAudioChannelLayoutTag_MatrixStereo             = (103U<<16) | 2,   // a matrix encoded stereo stream (Lt, Rt)
  kAudioChannelLayoutTag_MidSide                  = (104U<<16) | 2,   // mid/side recording
  kAudioChannelLayoutTag_XY                       = (105U<<16) | 2,   // coincident mic pair (often 2 figure 8's)
  kAudioChannelLayoutTag_Binaural                 = (106U<<16) | 2,   // binaural stereo (left, right)
  kAudioChannelLayoutTag_Ambisonic_B_Format       = (107U<<16) | 4,   // W, X, Y, Z

  kAudioChannelLayoutTag_Quadraphonic             = (108U<<16) | 4,   // L R Ls Rs  -- 90 degree speaker separation
  kAudioChannelLayoutTag_Pentagonal               = (109U<<16) | 5,   // L R Ls Rs C  -- 72 degree speaker separation
  kAudioChannelLayoutTag_Hexagonal                = (110U<<16) | 6,   // L R Ls Rs C Cs  -- 60 degree speaker separation
  kAudioChannelLayoutTag_Octagonal                = (111U<<16) | 8,   // L R Ls Rs C Cs Lw Rw  -- 45 degree speaker separation
  kAudioChannelLayoutTag_Cube                     = (112U<<16) | 8,   // left, right, rear left, rear right
                                                                      // top left, top right, top rear left, top rear right

  //  MPEG defined layouts
  kAudioChannelLayoutTag_MPEG_1_0                 = kAudioChannelLayoutTag_Mono,         //  C
  kAudioChannelLayoutTag_MPEG_2_0                 = kAudioChannelLayoutTag_Stereo,       //  L R
  kAudioChannelLayoutTag_MPEG_3_0_A               = (113U<<16) | 3,                       //  L R C
  kAudioChannelLayoutTag_MPEG_3_0_B               = (114U<<16) | 3,                       //  C L R
  kAudioChannelLayoutTag_MPEG_4_0_A               = (115U<<16) | 4,                       //  L R C Cs
  kAudioChannelLayoutTag_MPEG_4_0_B               = (116U<<16) | 4,                       //  C L R Cs
  kAudioChannelLayoutTag_MPEG_5_0_A               = (117U<<16) | 5,                       //  L R C Ls Rs
  kAudioChannelLayoutTag_MPEG_5_0_B               = (118U<<16) | 5,                       //  L R Ls Rs C
  kAudioChannelLayoutTag_MPEG_5_0_C               = (119U<<16) | 5,                       //  L C R Ls Rs
  kAudioChannelLayoutTag_MPEG_5_0_D               = (120U<<16) | 5,                       //  C L R Ls Rs
  kAudioChannelLayoutTag_MPEG_5_1_A               = (121U<<16) | 6,                       //  L R C LFE Ls Rs
  kAudioChannelLayoutTag_MPEG_5_1_B               = (122U<<16) | 6,                       //  L R Ls Rs C LFE
  kAudioChannelLayoutTag_MPEG_5_1_C               = (123U<<16) | 6,                       //  L C R Ls Rs LFE
  kAudioChannelLayoutTag_MPEG_5_1_D               = (124U<<16) | 6,                       //  C L R Ls Rs LFE
  kAudioChannelLayoutTag_MPEG_6_1_A               = (125U<<16) | 7,                       //  L R C LFE Ls Rs Cs
  kAudioChannelLayoutTag_MPEG_7_1_A               = (126U<<16) | 8,                       //  L R C LFE Ls Rs Lc Rc
  kAudioChannelLayoutTag_MPEG_7_1_B               = (127U<<16) | 8,                       //  C Lc Rc L R Ls Rs LFE    (doc: IS-13818-7 MPEG2-AAC Table 3.1)
  kAudioChannelLayoutTag_MPEG_7_1_C               = (128U<<16) | 8,                       //  L R C LFE Ls Rs Rls Rrs
  kAudioChannelLayoutTag_Emagic_Default_7_1       = (129U<<16) | 8,                       //  L R Ls Rs C LFE Lc Rc
  kAudioChannelLayoutTag_SMPTE_DTV                = (130U<<16) | 8,                       //  L R C LFE Ls Rs Lt Rt
                                                                                          //      (kAudioChannelLayoutTag_ITU_5_1 plus a matrix encoded stereo mix)
  //  ITU defined layouts
  kAudioChannelLayoutTag_ITU_1_0                  = kAudioChannelLayoutTag_Mono,         //  C
  kAudioChannelLayoutTag_ITU_2_0                  = kAudioChannelLayoutTag_Stereo,       //  L R

  kAudioChannelLayoutTag_ITU_2_1                  = (131U<<16) | 3,                       //  L R Cs
  kAudioChannelLayoutTag_ITU_2_2                  = (132U<<16) | 4,                       //  L R Ls Rs
  kAudioChannelLayoutTag_ITU_3_0                  = kAudioChannelLayoutTag_MPEG_3_0_A,   //  L R C
  kAudioChannelLayoutTag_ITU_3_1                  = kAudioChannelLayoutTag_MPEG_4_0_A,   //  L R C Cs

  kAudioChannelLayoutTag_ITU_3_2                  = kAudioChannelLayoutTag_MPEG_5_0_A,   //  L R C Ls Rs
  kAudioChannelLayoutTag_ITU_3_2_1                = kAudioChannelLayoutTag_MPEG_5_1_A,   //  L R C LFE Ls Rs
  kAudioChannelLayoutTag_ITU_3_4_1                = kAudioChannelLayoutTag_MPEG_7_1_C,   //  L R C LFE Ls Rs Rls Rrs

  // DVD defined layouts
  kAudioChannelLayoutTag_DVD_0                    = kAudioChannelLayoutTag_Mono,         // C (mono)
  kAudioChannelLayoutTag_DVD_1                    = kAudioChannelLayoutTag_Stereo,       // L R
  kAudioChannelLayoutTag_DVD_2                    = kAudioChannelLayoutTag_ITU_2_1,      // L R Cs
  kAudioChannelLayoutTag_DVD_3                    = kAudioChannelLayoutTag_ITU_2_2,      // L R Ls Rs
  kAudioChannelLayoutTag_DVD_4                    = (133U<<16) | 3,                       // L R LFE
  kAudioChannelLayoutTag_DVD_5                    = (134U<<16) | 4,                       // L R LFE Cs
  kAudioChannelLayoutTag_DVD_6                    = (135U<<16) | 5,                       // L R LFE Ls Rs
  kAudioChannelLayoutTag_DVD_7                    = kAudioChannelLayoutTag_MPEG_3_0_A,   // L R C
  kAudioChannelLayoutTag_DVD_8                    = kAudioChannelLayoutTag_MPEG_4_0_A,   // L R C Cs
  kAudioChannelLayoutTag_DVD_9                    = kAudioChannelLayoutTag_MPEG_5_0_A,   // L R C Ls Rs
  kAudioChannelLayoutTag_DVD_10                   = (136U<<16) | 4,                       // L R C LFE
  kAudioChannelLayoutTag_DVD_11                   = (137U<<16) | 5,                       // L R C LFE Cs
  kAudioChannelLayoutTag_DVD_12                   = kAudioChannelLayoutTag_MPEG_5_1_A,   // L R C LFE Ls Rs
  // 13 through 17 are duplicates of 8 through 12.
  kAudioChannelLayoutTag_DVD_13                   = kAudioChannelLayoutTag_DVD_8,        // L R C Cs
  kAudioChannelLayoutTag_DVD_14                   = kAudioChannelLayoutTag_DVD_9,        // L R C Ls Rs
  kAudioChannelLayoutTag_DVD_15                   = kAudioChannelLayoutTag_DVD_10,       // L R C LFE
  kAudioChannelLayoutTag_DVD_16                   = kAudioChannelLayoutTag_DVD_11,       // L R C LFE Cs
  kAudioChannelLayoutTag_DVD_17                   = kAudioChannelLayoutTag_DVD_12,       // L R C LFE Ls Rs
  kAudioChannelLayoutTag_DVD_18                   = (138U<<16) | 5,                       // L R Ls Rs LFE
  kAudioChannelLayoutTag_DVD_19                   = kAudioChannelLayoutTag_MPEG_5_0_B,   // L R Ls Rs C
  kAudioChannelLayoutTag_DVD_20                   = kAudioChannelLayoutTag_MPEG_5_1_B,   // L R Ls Rs C LFE

  // These layouts are recommended for AudioUnit usage
      // These are the symmetrical layouts
  kAudioChannelLayoutTag_AudioUnit_4              = kAudioChannelLayoutTag_Quadraphonic,
  kAudioChannelLayoutTag_AudioUnit_5              = kAudioChannelLayoutTag_Pentagonal,
  kAudioChannelLayoutTag_AudioUnit_6              = kAudioChannelLayoutTag_Hexagonal,
  kAudioChannelLayoutTag_AudioUnit_8              = kAudioChannelLayoutTag_Octagonal,
      // These are the surround-based layouts
  kAudioChannelLayoutTag_AudioUnit_5_0            = kAudioChannelLayoutTag_MPEG_5_0_B,   // L R Ls Rs C
  kAudioChannelLayoutTag_AudioUnit_6_0            = (139U<<16) | 6,                       // L R Ls Rs C Cs
  kAudioChannelLayoutTag_AudioUnit_7_0            = (140U<<16) | 7,                       // L R Ls Rs C Rls Rrs
  kAudioChannelLayoutTag_AudioUnit_7_0_Front      = (148U<<16) | 7,                       // L R Ls Rs C Lc Rc
  kAudioChannelLayoutTag_AudioUnit_5_1            = kAudioChannelLayoutTag_MPEG_5_1_A,   // L R C LFE Ls Rs
  kAudioChannelLayoutTag_AudioUnit_6_1            = kAudioChannelLayoutTag_MPEG_6_1_A,   // L R C LFE Ls Rs Cs
  kAudioChannelLayoutTag_AudioUnit_7_1            = kAudioChannelLayoutTag_MPEG_7_1_C,   // L R C LFE Ls Rs Rls Rrs
  kAudioChannelLayoutTag_AudioUnit_7_1_Front      = kAudioChannelLayoutTag_MPEG_7_1_A,   // L R C LFE Ls Rs Lc Rc

  kAudioChannelLayoutTag_AAC_3_0                  = kAudioChannelLayoutTag_MPEG_3_0_B,   // C L R
  kAudioChannelLayoutTag_AAC_Quadraphonic         = kAudioChannelLayoutTag_Quadraphonic, // L R Ls Rs
  kAudioChannelLayoutTag_AAC_4_0                  = kAudioChannelLayoutTag_MPEG_4_0_B,   // C L R Cs
  kAudioChannelLayoutTag_AAC_5_0                  = kAudioChannelLayoutTag_MPEG_5_0_D,   // C L R Ls Rs
  kAudioChannelLayoutTag_AAC_5_1                  = kAudioChannelLayoutTag_MPEG_5_1_D,   // C L R Ls Rs Lfe
  kAudioChannelLayoutTag_AAC_6_0                  = (141U<<16) | 6,                       // C L R Ls Rs Cs
  kAudioChannelLayoutTag_AAC_6_1                  = (142U<<16) | 7,                       // C L R Ls Rs Cs Lfe
  kAudioChannelLayoutTag_AAC_7_0                  = (143U<<16) | 7,                       // C L R Ls Rs Rls Rrs
  kAudioChannelLayoutTag_AAC_7_1                  = kAudioChannelLayoutTag_MPEG_7_1_B,   // C Lc Rc L R Ls Rs Lfe
  kAudioChannelLayoutTag_AAC_7_1_B                = (183U<<16) | 8,                       // C L R Ls Rs Rls Rrs LFE
  kAudioChannelLayoutTag_AAC_7_1_C                = (184U<<16) | 8,                       // C L R Ls Rs LFE Vhl Vhr
  kAudioChannelLayoutTag_AAC_Octagonal            = (144U<<16) | 8,                       // C L R Ls Rs Rls Rrs Cs

  kAudioChannelLayoutTag_TMH_10_2_std             = (145U<<16) | 16,                      // L R C Vhc Lsd Rsd Ls Rs Vhl Vhr Lw Rw Csd Cs LFE1 LFE2
  kAudioChannelLayoutTag_TMH_10_2_full            = (146U<<16) | 21,                      // TMH_10_2_std plus: Lc Rc HI VI Haptic

  kAudioChannelLayoutTag_AC3_1_0_1                = (149U<<16) | 2,                       // C LFE
  kAudioChannelLayoutTag_AC3_3_0                  = (150U<<16) | 3,                       // L C R
  kAudioChannelLayoutTag_AC3_3_1                  = (151U<<16) | 4,                       // L C R Cs
  kAudioChannelLayoutTag_AC3_3_0_1                = (152U<<16) | 4,                       // L C R LFE
  kAudioChannelLayoutTag_AC3_2_1_1                = (153U<<16) | 4,                       // L R Cs LFE
  kAudioChannelLayoutTag_AC3_3_1_1                = (154U<<16) | 5,                       // L C R Cs LFE

  kAudioChannelLayoutTag_EAC_6_0_A                = (155U<<16) | 6,                       // L C R Ls Rs Cs
  kAudioChannelLayoutTag_EAC_7_0_A                = (156U<<16) | 7,                       // L C R Ls Rs Rls Rrs

  kAudioChannelLayoutTag_EAC3_6_1_A               = (157U<<16) | 7,                       // L C R Ls Rs LFE Cs
  kAudioChannelLayoutTag_EAC3_6_1_B               = (158U<<16) | 7,                       // L C R Ls Rs LFE Ts
  kAudioChannelLayoutTag_EAC3_6_1_C               = (159U<<16) | 7,                       // L C R Ls Rs LFE Vhc
  kAudioChannelLayoutTag_EAC3_7_1_A               = (160U<<16) | 8,                       // L C R Ls Rs LFE Rls Rrs
  kAudioChannelLayoutTag_EAC3_7_1_B               = (161U<<16) | 8,                       // L C R Ls Rs LFE Lc Rc
  kAudioChannelLayoutTag_EAC3_7_1_C               = (162U<<16) | 8,                       // L C R Ls Rs LFE Lsd Rsd
  kAudioChannelLayoutTag_EAC3_7_1_D               = (163U<<16) | 8,                       // L C R Ls Rs LFE Lw Rw
  kAudioChannelLayoutTag_EAC3_7_1_E               = (164U<<16) | 8,                       // L C R Ls Rs LFE Vhl Vhr

  kAudioChannelLayoutTag_EAC3_7_1_F               = (165U<<16) | 8,                        // L C R Ls Rs LFE Cs Ts
  kAudioChannelLayoutTag_EAC3_7_1_G               = (166U<<16) | 8,                        // L C R Ls Rs LFE Cs Vhc
  kAudioChannelLayoutTag_EAC3_7_1_H               = (167U<<16) | 8,                        // L C R Ls Rs LFE Ts Vhc

  kAudioChannelLayoutTag_DTS_3_1                  = (168U<<16) | 4,                        // C L R LFE
  kAudioChannelLayoutTag_DTS_4_1                  = (169U<<16) | 5,                        // C L R Cs LFE
  kAudioChannelLayoutTag_DTS_6_0_A                = (170U<<16) | 6,                        // Lc Rc L R Ls Rs
  kAudioChannelLayoutTag_DTS_6_0_B                = (171U<<16) | 6,                        // C L R Rls Rrs Ts
  kAudioChannelLayoutTag_DTS_6_0_C                = (172U<<16) | 6,                        // C Cs L R Rls Rrs
  kAudioChannelLayoutTag_DTS_6_1_A                = (173U<<16) | 7,                        // Lc Rc L R Ls Rs LFE
  kAudioChannelLayoutTag_DTS_6_1_B                = (174U<<16) | 7,                        // C L R Rls Rrs Ts LFE
  kAudioChannelLayoutTag_DTS_6_1_C                = (175U<<16) | 7,                        // C Cs L R Rls Rrs LFE
  kAudioChannelLayoutTag_DTS_7_0                  = (176U<<16) | 7,                        // Lc C Rc L R Ls Rs
  kAudioChannelLayoutTag_DTS_7_1                  = (177U<<16) | 8,                        // Lc C Rc L R Ls Rs LFE
  kAudioChannelLayoutTag_DTS_8_0_A                = (178U<<16) | 8,                        // Lc Rc L R Ls Rs Rls Rrs
  kAudioChannelLayoutTag_DTS_8_0_B                = (179U<<16) | 8,                        // Lc C Rc L R Ls Cs Rs
  kAudioChannelLayoutTag_DTS_8_1_A                = (180U<<16) | 9,                        // Lc Rc L R Ls Rs Rls Rrs LFE
  kAudioChannelLayoutTag_DTS_8_1_B                = (181U<<16) | 9,                        // Lc C Rc L R Ls Cs Rs LFE
  kAudioChannelLayoutTag_DTS_6_1_D                = (182U<<16) | 7,                        // C L R Ls Rs LFE Cs

  kAudioChannelLayoutTag_WAVE_2_1                 = kAudioChannelLayoutTag_DVD_4,          // 3 channels, L R LFE
  kAudioChannelLayoutTag_WAVE_3_0                 = kAudioChannelLayoutTag_MPEG_3_0_A,     // 3 channels, L R C
  kAudioChannelLayoutTag_WAVE_4_0_A               = kAudioChannelLayoutTag_ITU_2_2,        // 4 channels, L R Ls Rs
  kAudioChannelLayoutTag_WAVE_4_0_B               = (185U<<16) | 4,                        // 4 channels, L R Rls Rrs
  kAudioChannelLayoutTag_WAVE_5_0_A               = kAudioChannelLayoutTag_MPEG_5_0_A,     // 5 channels, L R C Ls Rs
  kAudioChannelLayoutTag_WAVE_5_0_B               = (186U<<16) | 5,                        // 5 channels, L R C Rls Rrs
  kAudioChannelLayoutTag_WAVE_5_1_A               = kAudioChannelLayoutTag_MPEG_5_1_A,     // 6 channels, L R C LFE Ls Rs
  kAudioChannelLayoutTag_WAVE_5_1_B               = (187U<<16) | 6,                        // 6 channels, L R C LFE Rls Rrs
  kAudioChannelLayoutTag_WAVE_6_1                 = (188U<<16) | 7,                        // 7 channels, L R C LFE Cs Ls Rs
  kAudioChannelLayoutTag_WAVE_7_1                 = (189U<<16) | 8,                        // 8 channels, L R C LFE Rls Rrs Ls Rs

  kAudioChannelLayoutTag_HOA_ACN_SN3D             = (190U<<16) | 0,                        // Higher Order Ambisonics, Ambisonics Channel Number, SN3D normalization
                                                                                            // needs to be ORed with the actual number of channels (not the HOA order)
  kAudioChannelLayoutTag_HOA_ACN_N3D              = (191U<<16) | 0,                        // Higher Order Ambisonics, Ambisonics Channel Number, N3D normalization
                                                                                            // needs to be ORed with the actual number of channels (not the HOA order)

  kAudioChannelLayoutTag_Atmos_5_1_2 = (194U<<16) | 8, ///< L R C LFE Ls Rs Ltm Rtm
  kAudioChannelLayoutTag_Atmos_5_1_4 = (195U<<16) | 10, ///< L R C LFE Ls Rs Vhl Vhr Ltr Rtr
  kAudioChannelLayoutTag_Atmos_7_1_2 = (196U<<16) | 10, ///< L R C LFE Ls Rs Rls Rrs Ltm Rtm
  kAudioChannelLayoutTag_Atmos_7_1_4 = (192U<<16) | 12, ///< L R C LFE Ls Rs Rls Rrs Vhl Vhr Ltr Rtr
  kAudioChannelLayoutTag_Atmos_9_1_6 = (193U<<16) | 16, ///< L R C LFE Ls Rs Rls Rrs Lw Rw Vhl Vhr Ltm Rtm Ltr Rtr                       // L R C LFE Ls Rs Ltm Rtm

  kAudioChannelLayoutTag_DiscreteInOrder          = (147U<<16) | 0, ///< needs to be ORed with the actual number of channels                       // needs to be ORed with the actual number of channels

  kAudioChannelLayoutTag_BeginReserved            = 0xF0000000,                            // Channel layout tag values in this range are reserved for internal use
  kAudioChannelLayoutTag_EndReserved              = 0xFFFEFFFF,

  kAudioChannelLayoutTag_Unknown                  = 0xFFFF0000                             // needs to be ORed with the actual number of channels
};

enum
{
  kAudioChannelLabel_Unknown                  = 0xFFFFFFFF,   // unknown or unspecified other use
  kAudioChannelLabel_Unused                   = 0,            // channel is present, but has no intended use or destination
  kAudioChannelLabel_UseCoordinates           = 100,          // channel is described by the mCoordinates fields.

  kAudioChannelLabel_Left                     = 1,
  kAudioChannelLabel_Right                    = 2,
  kAudioChannelLabel_Center                   = 3,
  kAudioChannelLabel_LFEScreen                = 4,
  kAudioChannelLabel_LeftSurround             = 5,
  kAudioChannelLabel_RightSurround            = 6,
  kAudioChannelLabel_LeftCenter               = 7,
  kAudioChannelLabel_RightCenter              = 8,
  kAudioChannelLabel_CenterSurround           = 9,            // WAVE: "Back Center" or plain "Rear Surround"
  kAudioChannelLabel_LeftSurroundDirect       = 10,
  kAudioChannelLabel_RightSurroundDirect      = 11,
  kAudioChannelLabel_TopCenterSurround        = 12,
  kAudioChannelLabel_VerticalHeightLeft       = 13,           // WAVE: "Top Front Left"
  kAudioChannelLabel_VerticalHeightCenter     = 14,           // WAVE: "Top Front Center"
  kAudioChannelLabel_VerticalHeightRight      = 15,           // WAVE: "Top Front Right"

  kAudioChannelLabel_TopBackLeft              = 16,
  kAudioChannelLabel_TopBackCenter            = 17,
  kAudioChannelLabel_TopBackRight             = 18,

  kAudioChannelLabel_RearSurroundLeft         = 33,
  kAudioChannelLabel_RearSurroundRight        = 34,
  kAudioChannelLabel_LeftWide                 = 35,
  kAudioChannelLabel_RightWide                = 36,
  kAudioChannelLabel_LFE2                     = 37,
  kAudioChannelLabel_LeftTotal                = 38,           // matrix encoded 4 channels
  kAudioChannelLabel_RightTotal               = 39,           // matrix encoded 4 channels
  kAudioChannelLabel_HearingImpaired          = 40,
  kAudioChannelLabel_Narration                = 41,
  kAudioChannelLabel_Mono                     = 42,
  kAudioChannelLabel_DialogCentricMix         = 43,

  kAudioChannelLabel_CenterSurroundDirect     = 44,           // back center, non diffuse

  kAudioChannelLabel_Haptic                   = 45,

  kAudioChannelLabel_LeftTopFront             = 46,
  kAudioChannelLabel_CenterTopFront           = 47,
  kAudioChannelLabel_RightTopFront            = 48,
  kAudioChannelLabel_LeftTopMiddle            = 49,
  kAudioChannelLabel_CenterTopMiddle          = 50,
  kAudioChannelLabel_RightTopMiddle           = 51,
  kAudioChannelLabel_LeftTopRear              = 52,
  kAudioChannelLabel_CenterTopRear            = 53,
  kAudioChannelLabel_RightTopRear             = 54,

  // first order ambisonic channels
  kAudioChannelLabel_Ambisonic_W              = 200,
  kAudioChannelLabel_Ambisonic_X              = 201,
  kAudioChannelLabel_Ambisonic_Y              = 202,
  kAudioChannelLabel_Ambisonic_Z              = 203,

  // Mid/Side Recording
  kAudioChannelLabel_MS_Mid                   = 204,
  kAudioChannelLabel_MS_Side                  = 205,

  // X-Y Recording
  kAudioChannelLabel_XY_X                     = 206,
  kAudioChannelLabel_XY_Y                     = 207,

  // Binaural Recording
  kAudioChannelLabel_BinauralLeft             = 208,
  kAudioChannelLabel_BinauralRight            = 209,

  // other
  kAudioChannelLabel_HeadphonesLeft           = 301,
  kAudioChannelLabel_HeadphonesRight          = 302,
  kAudioChannelLabel_ClickTrack               = 304,
  kAudioChannelLabel_ForeignLanguage          = 305,

  // generic discrete channel
  kAudioChannelLabel_Discrete                 = 400,

  // numbered discrete channel
  kAudioChannelLabel_Discrete_0               = (1U<<16) | 0,
  kAudioChannelLabel_Discrete_1               = (1U<<16) | 1,
  kAudioChannelLabel_Discrete_2               = (1U<<16) | 2,
  kAudioChannelLabel_Discrete_3               = (1U<<16) | 3,
  kAudioChannelLabel_Discrete_4               = (1U<<16) | 4,
  kAudioChannelLabel_Discrete_5               = (1U<<16) | 5,
  kAudioChannelLabel_Discrete_6               = (1U<<16) | 6,
  kAudioChannelLabel_Discrete_7               = (1U<<16) | 7,
  kAudioChannelLabel_Discrete_8               = (1U<<16) | 8,
  kAudioChannelLabel_Discrete_9               = (1U<<16) | 9,
  kAudioChannelLabel_Discrete_10              = (1U<<16) | 10,
  kAudioChannelLabel_Discrete_11              = (1U<<16) | 11,
  kAudioChannelLabel_Discrete_12              = (1U<<16) | 12,
  kAudioChannelLabel_Discrete_13              = (1U<<16) | 13,
  kAudioChannelLabel_Discrete_14              = (1U<<16) | 14,
  kAudioChannelLabel_Discrete_15              = (1U<<16) | 15,
  kAudioChannelLabel_Discrete_65535           = (1U<<16) | 65535,

  // generic HOA ACN channel
  kAudioChannelLabel_HOA_ACN                  = 500,

  // numbered HOA ACN channels
  kAudioChannelLabel_HOA_ACN_0                = (2U << 16) | 0,
  kAudioChannelLabel_HOA_ACN_1                = (2U << 16) | 1,
  kAudioChannelLabel_HOA_ACN_2                = (2U << 16) | 2,
  kAudioChannelLabel_HOA_ACN_3                = (2U << 16) | 3,
  kAudioChannelLabel_HOA_ACN_4                = (2U << 16) | 4,
  kAudioChannelLabel_HOA_ACN_5                = (2U << 16) | 5,
  kAudioChannelLabel_HOA_ACN_6                = (2U << 16) | 6,
  kAudioChannelLabel_HOA_ACN_7                = (2U << 16) | 7,
  kAudioChannelLabel_HOA_ACN_8                = (2U << 16) | 8,
  kAudioChannelLabel_HOA_ACN_9                = (2U << 16) | 9,
  kAudioChannelLabel_HOA_ACN_10               = (2U << 16) | 10,
  kAudioChannelLabel_HOA_ACN_11               = (2U << 16) | 11,
  kAudioChannelLabel_HOA_ACN_12               = (2U << 16) | 12,
  kAudioChannelLabel_HOA_ACN_13               = (2U << 16) | 13,
  kAudioChannelLabel_HOA_ACN_14               = (2U << 16) | 14,
  kAudioChannelLabel_HOA_ACN_15               = (2U << 16) | 15,
  kAudioChannelLabel_HOA_ACN_65024            = (2U << 16) | 65024,    // 254th order uses 65025 channels

  kAudioChannelLabel_BeginReserved            = 0xF0000000,           // Channel label values in this range are reserved for internal use
  kAudioChannelLabel_EndReserved              = 0xFFFFFFFE
};

enum
{
  kAudioChannelFlags_AllOff                   = 0,
  kAudioChannelFlags_RectangularCoordinates   = (1U<<0),
  kAudioChannelFlags_SphericalCoordinates     = (1U<<1),
  kAudioChannelFlags_Meters                   = (1U<<2)
};


#endif // _CAFCHANNELFORMATS_H_