#ifndef SWF_DETECTION_H
#define SWF_DETECTION_H

typedef enum
{
    SWFDetectionID_NONE = -1,
    SWFDetectionID_Warnings = 0,
    SWFDetectionID_CVE_2007_0071,
    SWFDetectionID_CVE_2007_5400,
    SWFDetectionID_CVE_2005_2628,
    SWFDetectionID_END
} SWFDetectionID;

struct swf_vul_desc
{
    const char *description;
    const char *cve[14];
    const int cveCount;
    const char *bid[7];
    const int bidCount;
};

/* SWF_DetectionID_CVE_2007_0071 */
const struct swf_vul_desc DetectionResult00 = {
    "Parsing warnings",
    { NULL } ,
    0,
    { NULL },
    0
};

const struct swf_vul_desc DetectionResult01 = {
    "Adobe Flash Player Multimedia File DefineSceneAndFrameLabelData Code Execution",
    { "CVE-2007-0071" },
    1,
    { "BID-28695" },
    1
};

/* SWF_DetectionID_CVE_2007_5400 */
const struct swf_vul_desc DetectionResult02 = {
    "RealNetworks RealPlayer SWF Frame Handling Buffer Overflow",
    { "CVE-2007-5400", "CVE-2006-0323" },
    2,
    { NULL },
    0
};

/* SWF_DetectionID_CVE_2005_2628 */
const struct swf_vul_desc DetectionResult03 = {
    "Macromedia Flash ActionDefineFunction Memory Access Vulnerability",
    { "CVE-2005-2628" },
    1,
    { "BID-15334" },
    1
};

const struct swf_vul_desc *DetectionResults[SWFDetectionID_END] = {
    &DetectionResult00,
    &DetectionResult01,
    &DetectionResult02,
    &DetectionResult03,
};

#endif

