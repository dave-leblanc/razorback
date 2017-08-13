#ifndef _SWF_SCANNER_H
#define _SWF_SCANNER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <zlib.h>
#include <arpa/inet.h>
#include "swf_detection.h"

//#define VERBOSE
#ifdef VERBOSE
    #define VERBOSE_SWF(x) x
#else
    #define VERBOSE_SWF(x)
#endif

#ifndef READ_LITTLE_32
    #define READ_LITTLE_32(p) (uint32_t)((*((uint8_t *)(p) + 3) << 24) \
                        | (*((uint8_t *)(p) + 2) << 16) \
                        | (*((uint8_t *)(p) + 1) << 8)  \
                        | (*(p)))
#endif

#ifndef READ_LITTLE_16
    #define READ_LITTLE_16(p) (uint16_t)((*((uint8_t *)(p) + 1) << 8) | (*(p)))
#endif

typedef struct _TAGList
{
    void *tag_head;       // this needs to be casted to TAG *
    void *tag_tail;    // this needs to be casted to TAG *
} TAGList;

typedef struct _TAG
{
    uint16_t TagCode;     // stores 10 bits tag code
    uint32_t Length;      // length is either 6 bits or 4 bytes

    struct _TAG *prev;
    struct _TAG *next;

    uint8_t     *tag_start;   // points to TagCodeAndLength
    uint8_t     *tag_data;    // points to the beginning of record data

    /* to handle DefineSprite */
    TAGList tags;
} TAG;

typedef struct
{
    uint8_t   Nbits;
    uint32_t  Xmin;
    uint32_t  Xmax;
    uint32_t  Ymin;
    uint32_t  Ymax;
} RECT;

typedef struct
{
    uint8_t   Signature[3];
    uint8_t   Version;
    uint32_t  FileLength;
    RECT      FrameSize;
    uint16_t  FrameRate;
    uint16_t  FrameCount;
} HEADER;

typedef struct
{
    HEADER header;

    TAGList tags;

    uint8_t *bof; // beginning of swf file
    uint8_t *eof; // end if swf file

    uint8_t compressed;          // If compressed is 1, decompressed_data must be freed after scanning
    uint8_t *decompressed_data;  // buffer for decompressed data, it needs to be free upon scan completion

} SWFFileInfo;

/* Tag code values */
/* TYPE_{Tag name}                         {Tag value} */
#define TYPE_End                            0
#define TYPE_ShowFrame                      1
#define TYPE_DefineShape                    2
#define TYPE_PlaceObject                    4
#define TYPE_RemoveObject                   5
#define TYPE_DefineBits                     6
#define TYPE_DefineButton                   7
#define TYPE_JPEGTables                     8
#define TYPE_SetBackgroundColor             9
#define TYPE_DefineFont                     10
#define TYPE_DefineText                     11
#define TYPE_DoAction                       12
#define TYPE_DefineFontInfo                 13
#define TYPE_DefineSound                    14
#define TYPE_StartSound                     15
#define TYPE_DefineButtonSound              17
#define TYPE_SoundStreamHead                18
#define TYPE_SoundStreamBlock               19
#define TYPE_DefineBitsLossless             20
#define TYPE_DefineBitsJPEG2                21
#define TYPE_DefineShape2                   22
#define TYPE_DefineButtonCxform             23
#define TYPE_Protect                        24
#define TYPE_PlaceObject2                   26
#define TYPE_RemoveObject2                  28
#define TYPE_DefineShape3                   32
#define TYPE_DefineText2                    33
#define TYPE_DefineButton2                  34
#define TYPE_DefineBitsJPEG3                35
#define TYPE_DefineBitsLossless2            36
#define TYPE_DefineEditText                 37
#define TYPE_DefineSprite                   39
#define TYPE_FrameLabel                     43
#define TYPE_SoundStreamHead2               45
#define TYPE_DefineMorphShape               46
#define TYPE_DefineFont2                    48
#define TYPE_ExportAssets                   56
#define TYPE_ImportAssets                   57
#define TYPE_EnableDebugger                 58
#define TYPE_DoInitAction                   59
#define TYPE_DefineVideoStream              60
#define TYPE_VideoFrame                     61
#define TYPE_DefineFontInfo2                62
#define TYPE_EnableDebugger2                64
#define TYPE_ScriptLimits                   65
#define TYPE_SetTabIndex                    66
#define TYPE_FileAttributes                 69
#define TYPE_PlaceObject3                   70
#define TYPE_ImportAssets2                  71
#define TYPE_DefineFontAlignZones           73
#define TYPE_CSMTextSettings                74
#define TYPE_DefineFont3                    75
#define TYPE_SymbolClass                    76
#define TYPE_Metadata                       77
#define TYPE_DefineScalingGrid              78
#define TYPE_DoABC                          82
#define TYPE_DefineShape4                   83
#define TYPE_DefineMorphShape2              84
#define TYPE_DefineSceneAndFrameLabelData   86
#define TYPE_DefineBinaryData               87
#define TYPE_DefineFontName                 88
#define TYPE_StartSound2                    89
#define TYPE_DefineBitsJPEG4                90
#define TYPE_DefineFont4                    91

#define MAX_TYPE_CODE                       92

const char *TypeName[MAX_TYPE_CODE] = {
    "End",                           // 0
    "ShowFrame",                     // 1
    "DefineShape",                   // 2
    "Unknown",
    "PlaceObject",                   // 4
    "RemoveObject",                  // 5
    "DefineBits",                    // 6
    "DefineButton",                  // 7
    "JPEGTables",                    // 8
    "SetBackgroundColor",            // 9
    "DefineFont",                    // 10
    "DefineText",                    // 11
    "DoAction",                      // 12
    "DefineFontInfo",                // 13
    "DefineSound",                   // 14
    "StartSound",                    // 15
    "Unknown",                       // 16
    "DefineButtonSound",             // 17
    "SoundStreamHead",               // 18
    "SoundStreamBlock",              // 19
    "DefineBitsLossless",            // 20
    "DefineBitsJPEG2",               // 21
    "DefineShape2",                  // 22
    "DefineButtonCxform",            // 23
    "Protect",                       // 24
    "Unknown",
    "PlaceObject2",                  // 26
    "Unknown",
    "RemoveObject2",                 // 28
    "Unknown",
    "Unknown",
    "Unknown",
    "DefineShape3",                  // 32
    "DefineText2",                   // 33
    "DefineButton2",                 // 34
    "DefineBitsJPEG3",               // 35
    "DefineBitsLossless2",           // 36
    "DefineEditText",                // 37
    "Unknown",
    "DefineSprite",                  // 39
    "Unknown",
    "Unknown",
    "Unknown",
    "FrameLabel",                    // 43
    "Unknown",
    "SoundStreamHead2",              // 45
    "DefineMorphShape",              // 46
    "Unknown",
    "DefineFont2",                   // 48
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "ExportAssets",                  // 56
    "ImportAssets",                  // 57
    "EnableDebugger",                // 58
    "DoInitAction",                  // 59
    "DefineVideoStream",             // 60
    "VideoFrame",                    // 61
    "DefineFontInfo2",               // 62
    "Unknown",
    "EnableDebugger2",               // 64
    "ScriptLimits",                  // 65
    "SetTabIndex",                   // 66
    "Unknown",
    "Unknown",
    "FileAttributes",                // 69
    "PlaceObject3",                  // 70
    "ImportAssets2",                 // 71
    "Unknown",
    "DefineFontAlignZones",          // 73
    "CSMTextSettings",               // 74
    "DefineFont3",                   // 75
    "SymbolClass",                   // 76
    "Metadata",                      // 77
    "DefineScalingGrid",             // 78
    "Unknown",
    "Unknown",
    "Unknown",
    "DoABC",                         // 82
    "DefineShape4",                  // 83
    "DefineMorphShape2",             // 84
    "Unknown",
    "DefineSceneAndFrameLabelData",  // 86
    "DefineBinaryData",              // 87
    "DefineFontName",                // 88
    "StartSound2",                   // 89
    "DefineBitsJPEG4",               // 90
    "DefineFont4"                    // 91
};

/* Action codes */
/* Followed the order of the specification */

/* SWF 3 actions */
#define ACTIONCODE_ActionGotoFrame     0x81
#define ACTIONCODE_ActionGetURL        0x83
#define ACTIONCODE_ActionNextFrame     0x04
#define ACTIONCODE_ActionPrevFrame     0x05
#define ACTIONCODE_ActionPlay          0x06
#define ACTIONCODE_ActionStop          0x07
#define ACTIONCODE_ActionToggleQualty  0x08
#define ACTIONCODE_ActionStopSounds    0x09
#define ACTIONCODE_ActionWaitForFrame  0x8A
#define ACTIONCODE_ActionSetTarget     0x8B
#define ACTIONCODE_ActionGoToLabel     0x8C

/* SWF 4 actions */
#define ACTIONCODE_ActionPush          0x96
#define ACTIONCODE_ActionPop           0x17
#define ACTIONCODE_ActionAdd           0x0A
#define ACTIONCODE_ActionSubtract      0x0B
#define ACTIONCODE_ActionMultiply      0x0C
#define ACTIONCODE_ActionDivide        0x0D
#define ACTIONCODE_ActionEquals        0x0E
#define ACTIONCODE_ActionLess          0x0F
#define ACTIONCODE_ActionAnd           0x10
#define ACTIONCODE_ActionOr            0x11
#define ACTIONCODE_ActionNot           0x12
#define ACTIONCODE_ActionStringEquals  0x13
#define ACTIONCODE_ActionStringLength  0x14
#define ACTIONCODE_ActionStringAdd     0x21
#define ACTIONCODE_ActionStringExtract 0x15
#define ACTIONCODE_ActionStringLess    0x29
#define ACTIONCODE_ActionMBStringLength 0x31
#define ACTIONCODE_ActionMBStringExtract 0x35
#define ACTIONCODE_ActionToInteger     0x18
#define ACTIONCODE_ActionCharToAscii   0x32
#define ACTIONCODE_ActionAsciiToChar   0x33
#define ACTIONCODE_ActionMBCharToAscii 0x36
#define ACTIONCODE_ActionMBAsciiToChar 0x37
#define ACTIONCODE_ActionJump          0x99
#define ACTIONCODE_ActionIf            0x9D
#define ACTIONCODE_ActionCall          0x9E
#define ACTIONCODE_ActionGetVariable   0x1C
#define ACTIONCODE_ActionSetVariable   0x1D
#define ACTIONCODE_ActionGetURL2       0x9A
#define ACTIONCODE_ActionGotoFrame2    0x9F
#define ACTIONCODE_ActionSetTarget2    0x20
#define ACTIONCODE_ActionGetProperty   0x22
#define ACTIONCODE_ActionSetProperty   0x23
#define ACTIONCODE_ActionCloneSprite   0x24
#define ACTIONCODE_ActionRemoveSprite  0x25
#define ACTIONCODE_ActionStartDrag     0x27
#define ACTIONCODE_ActionEndDrag       0x28
#define ACTIONCODE_ActionWaitForFrame2 0x8D
#define ACTIONCODE_ActionTrace         0x26
#define ACTIONCODE_ActionGetTime       0x34
#define ACTIONCODE_ActionRandomNumber  0x30

/* SWF 5 actions */
#define ACTIONCODE_ActionCallFunction  0x3D
#define ACTIONCODE_ActionCallMethod    0x52
#define ACTIONCODE_ActionConstantPool  0x88
#define ACTIONCODE_ActionDefineFunction 0x9B
#define ACTIONCODE_ActionDefineLocal   0x3C
#define ACTIONCODE_ActionDefineLocal2  0x41
#define ACTIONCODE_ActionDelete        0x3A
#define ACTIONCODE_ActionDelete2       0x3B
#define ACTIONCODE_ActionEnumerate     0x46
#define ACTIONCODE_ActionEquals2       0x49
#define ACTIONCODE_ActionGetMember     0x4E
#define ACTIONCODE_ActionInitArray     0x42
#define ACTIONCODE_ActionInitObject    0x43
#define ACTIONCODE_ActionNewMethod     0x53
#define ACTIONCODE_ActionNewObject     0x40
#define ACTIONCODE_ActionSetMember     0x4F
#define ACTIONCODE_ActionTargetPath    0x45
#define ACTIONCODE_ActionWith          0x94
#define ACTIONCODE_ActionToNumber      0x4A
#define ACTIONCODE_ActionToString      0x4B
#define ACTIONCODE_ActionTypeOf        0x44
#define ACTIONCODE_ActionAdd2          0x47
#define ACTIONCODE_ActionLess2         0x48
#define ACTIONCODE_ActionModulo        0x3F
#define ACTIONCODE_ActionBitAnd        0x60
#define ACTIONCODE_ActionBitLShift     0x63
#define ACTIONCODE_ActionBitOr         0x61
#define ACTIONCODE_ActionBitRShift     0x64
#define ACTIONCODE_ActionBitURShift    0x65
#define ACTIONCODE_ActionBitXor        0x62
#define ACTIONCODE_ActionDecrement     0x51
#define ACTIONCODE_ActionIncrement     0x50
#define ACTIONCODE_ActionPushDuplicate 0x4C
#define ACTIONCODE_ActionReturn        0x3E
#define ACTIONCODE_ActionStackSwap     0x4D
#define ACTIONCODE_ActionStoreRegister 0x87

/* SWF 6 actions */
#define ACTIONCODE_ActionInstanceOf    0x54
#define ACTIONCODE_ActionEnumerate2    0x55
#define ACTIONCODE_ActionStrictEquals  0x66
#define ACTIONCODE_ActionGreater       0x67
#define ACTIONCODE_ActionStringGreater 0x68

/* SWF 7 actions */
#define ACTIONCODE_ActionDefineFunction2 0x8E
#define ACTIONCODE_ActionExtends       0x69
#define ACTIONCODE_ActionCastOp        0x2B
#define ACTIONCODE_ActionImplementsOp  0x2C
#define ACTIONCODE_ActionTry           0x8F
#define ACTIONCODE_ActionThrow         0x2A

/* Error codes */
typedef enum
{
    SWF_OK                        = 0,
    SWF_ERROR_INVALID_SIG         = -1,
    SWF_ERROR_INVALID_FILELENGTH  = -2,
    SWF_ERROR_NODATA              = -3,
    SWF_ERROR_INVALID_TAGLENGTH   = -4,
    SWF_ERROR_NOTAGS              = -5,

    SWF_ERROR_MEMORY              = -10,
    SWF_ERROR_DECOMPRESSION       = -11

} ErrorCode;

#endif /* _SWF_SCANNER_H */

