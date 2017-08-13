#include "foxtoken.h"
#include "foxjsutil.h"
#include "foxreport.h"
#include "foxdetect.h"
#include <string.h>
#include <math.h>

#define FOX_DETECT_NUM_CVES 27

typedef enum { NONE, 
               CVE_2007_0104, 
               CVE_2007_3896, 
               CVE_2007_5659, 
               CVE_2008_2992, 
               CVE_2008_4813, 
               CVE_2009_0658, 
               CVE_2009_0836, 
               CVE_2009_0927, 
               CVE_2009_1492, 
               CVE_2009_1493, 
               CVE_2009_1855, 
               CVE_2009_1856, 
               CVE_2009_2990, 
               CVE_2009_2994, 
               CVE_2009_2997,
               CVE_2009_3459,
               CVE_2009_3955,
               CVE_2009_4324,
               CVE_2010_0188,
               CVE_2010_0196,
               CVE_2010_2862,
               CVE_2010_2883,
               CVE_2010_3622,
               CVE_2010_4091,
               CVE_2011_2106,
			   CVE_2011_2462
             } CVELIST;

static void (*DetectionByCVE [FOX_DETECT_NUM_CVES] )(uint8_t *content, uint32_t length, PDFStreamType type);


void Dig (uint8_t *content, uint32_t length, PDFStreamType type) {
	int i;
	for (i = 1; i < FOX_DETECT_NUM_CVES; i++)
		if (DetectionByCVE[i] != NULL)
		    (*DetectionByCVE [i] )(content, length, type);
}

static void cve_2007_0104 (uint8_t *content, uint32_t length, PDFStreamType type) {
}

static void cve_2007_3896 (uint8_t *content, uint32_t length, PDFStreamType type) {

	uint32_t i, found = 0;
    char *extension, *end;

	if (type != URI)
		return;

	for (i = 0; i < length; i++) {
		if ((char)content[i] == '%') {
			found = 1;
			break;
		}
	}

	if (!found)
		return;

    end = (char *)(content + length);

	while ((extension = strstr((char *)content, ".cmd")) != NULL) {
		if (extension + 4 >= end) {
			goto FOUND;
		}
		else if (isWhitespace((char)content[4])) {
			goto FOUND;
		}
	}

	while ((extension = strstr((char *)content, ".bat")) != NULL) {
        if (extension + 4 >= end) {
            goto FOUND;
        }   
        else if (isWhitespace((char)content[4])) {
            goto FOUND;
        } 
	}

	while ((extension = strstr((char *)content, ".msi")) != NULL) {
        if (extension + 4 >= end) {
            goto FOUND;
        }   
        else if (isWhitespace((char)content[4])) {
            goto FOUND;
        } 
	}     

    return;

FOUND:
	foxReport("Microsoft Windows ShellExecute and IE7 URL Handling Code Execution", "CVE-2007-3896", 2, 2, 0, 1);

}

static void cve_2007_5659 (uint8_t *content, uint32_t length, PDFStreamType type) {
/*
 *
 * Will need to add new functionality to the JSArg parser.
 *
 * Collab.collectEmailInfo ({
 * to:"to_addr",
 * cc:"cc",
 * subj:"subject",
 * msg:"msg body",...});
 *
 * Where message body >= 4096
 */

    JSArgList *list;
    //uint32_t arglen;
    char *msg, *end;
	int i, j, ignore = 0;

	if (type != JAVASCRIPT)
		return;

    if ((list = JSFindFunction((char *)content, length, "Collab.collectEmailInfo")) == NULL) {
		//Function not found
		return;
	}

    if (list->count < 1)
		goto END;

	//arglen = strlen(list->vector[0]);
	//strstr for msg:
	if ((msg = strstr(list->vector[0], "msg:")) == NULL) {
		//msg not found
		goto END;
	}

    end = (char *)(content + length);

    if (end - msg < 4096)
		goto END;

	for (i = 0; i < end - msg; i++) {
		if (*(msg + i) == '"')
			break;
	}

	i++;

	for (j = i; j < end - msg; j++) {
		if (ignore == 0) {
			if (*(msg + j) == '"') {
				if (j-i >= 4096) {
				    foxReport("Adobe Multiple Products PDF JavaScript Method Buffer Overflow", "CVE-2007-5659", 3, 2, 0, 1);
					goto END;
				}
				else {
					//msg len okay!
					goto END;
				}
			}
			else if (*(msg + j) == '\\') {
				ignore = 1;
			}
		}
		else {
			ignore = 1;
		}
	}
	
	//Msg never ended.
END:
	destroyJSArgList(list);
	return;
}


static void cve_2008_2992 (uint8_t *content, uint32_t length, PDFStreamType type) {
	
    JSArgList *list;
    uint32_t arglen;
    uint32_t i;

	if (type != JAVASCRIPT)
		return;

	if ((list = JSFindFunction((char *)content, length, "util.printf")) == NULL) {
		//Function not found
		return;
	}

	if (list->count < 1)
		goto END;

	arglen = strlen(list->vector[0]);

    for (i = 0; i < arglen; i++) {
		if (list->vector[0][i] == '%') 
			break;
	}

	if (i == arglen)
		goto END;

    i++;

    arglen = strtoul((const char *)(list->vector[0] + i), NULL, 10);

	if (arglen > 264)
		foxReport("Adobe Reader and Acrobat util.printf Stack Buffer Overflow", "CVE-2008-2992", 4, 2, 0, 1);

    //Search for util.printf and
	//return arg list in a loop
	//
	//Alert when conditions satisfied.
	/*
	 * util.printf(%#f, ...)
	 *
	 * where the # part > 264
	 */

END:
	destroyJSArgList(list);
	return;
}

static void cve_2008_4813 (uint8_t *content, uint32_t length, PDFStreamType type) {
}

static void cve_2009_0658 (uint8_t *content, uint32_t length, PDFStreamType type) {

    uint32_t segPageAssoc, flags, jumpDist;

	if (type != JBIG2)
		return;

	if (length < 6)
		return;

	if ((content[4] & 0x40) == 0)
		return;

	if ((content[5] & 0xe0) >> 5 > 4) {
		//Long form
		flags = content[5] & 0x1fffffff;
		jumpDist = 4 + (uint32_t)ceil((((double)flags + 1)/8));
		
		if (6 + jumpDist + 4 > length)
			return;

        segPageAssoc = *(content+6) << 24;
        segPageAssoc |= *(content+7) << 16;
        segPageAssoc |= *(content+8) << 8;
        segPageAssoc |= *(content+9);

        if (segPageAssoc > 0x1000)
            foxReport("Adobe Multiple Products Embedded JBIG2 Stream Buffer Overflow", "CVE-2009-0658", 6, 2, 0, 1);

	}
	else {
		if (length < 10)
			return;

		segPageAssoc = *(content+6) << 24;
		segPageAssoc |= *(content+7) << 16;
		segPageAssoc |= *(content+8) << 8;
		segPageAssoc |= *(content+9);

		if (segPageAssoc > 0x1000)
            foxReport("Adobe Multiple Products Embedded JBIG2 Stream Buffer Overflow", "CVE-2009-0658", 6, 2, 0, 1);
	}

}

static void cve_2009_0836 (uint8_t *content, uint32_t length, PDFStreamType type) {
}

static void cve_2009_0927 (uint8_t *content, uint32_t length, PDFStreamType type) {
    JSArgList *list;

	if (type != JAVASCRIPT)
		return;

	if ((list = JSFindFunction((char *)content, length, "Collab.getIcon")) == NULL) {
		//Function not found
		return;
	}

	if (list->count < 1)
		goto END;

	if (strlen(list->vector[0]) > 256)
		foxReport("Adobe Acrobat JavaScript getIcon Method Buffer Overflow", "CVE-2009-0927", 8, 2, 0, 1);

END:
	destroyJSArgList(list);
	return;

}

static void cve_2009_1492 (uint8_t *content, uint32_t length, PDFStreamType type) {
	JSArgList *list;
    uint32_t i;

	if (type != JAVASCRIPT)
		return;

	if ((list = JSFindFunction((char *)content, length, "getAnnots")) == NULL) {
		//Function not found
		return;
	}
	
    if (list->count < 4)
		goto END;

	for (i = 0; i < 4; i++) {
		if (list->vector[i][0] != '-')
		    goto END;
	}

	foxReport("Adobe Reader JavaScript getAnnots Method Memory Corruption", "CVE-2009-1492", 9, 2, 0, 1);

	/*
	 * getAnnots(-#,-#,-#,-#)
	 *
	 * All negative.
	 */
END:
	destroyJSArgList(list);
	return;
}

static void cve_2009_1493 (uint8_t *content, uint32_t length, PDFStreamType type) {
	JSArgList *list;

	if (type != JAVASCRIPT)
		return;

	if ((list = JSFindFunction((char *)content, length, "customDictionaryOpen")) == NULL) {
		//Function not found
		return;
	}

	if (list->count < 2)
		goto END;

	if (strlen(list->vector[1]) > 256)
	    foxReport("Adobe Reader JavaScript spell.customDictionaryOpen Method Memory Corruption", "CVE-2009-1493", 10, 2, 0, 1);

END:
	destroyJSArgList(list);
	return;
}

static void cve_2009_1855 (uint8_t *content, uint32_t length, PDFStreamType type) {
	//U3D extension
	//\x00\x00\x00\x00RHAdobeMeta key for metadata
	//value is xml
	//
	//if any of the names of attribites > 256, alert
}

static void cve_2009_1856 (uint8_t *content, uint32_t length, PDFStreamType type) {
    DecodeParams *DParams;
    uint64_t result;

    if (type != DECODEPARAMS)
        return;

    if (length != sizeof(DecodeParams))
        return;

    DParams = (DecodeParams *)content;

    if (DParams->predictor <= 1) 
		return;
	
	result = (uint64_t)DParams->columns * DParams->bitspercomp;

	if (result > 0xffffffff) {
		foxReport("Adobe Acrobat and Adobe Reader FlateDecode Integer Overflow", "CVE-2009-1856", 12, 2, 0, 2);
		return;
	}

	result = (result * DParams->colors);

	if (result > 0xffffffff) {
		foxReport("Adobe Acrobat and Adobe Reader FlateDecode Integer Overflow", "CVE-2009-1856", 12, 2, 0, 2);
		return;
	}

}

static void cve_2009_2990 (uint8_t *content, uint32_t length, PDFStreamType type) {
    
	if (type != U3D)
		return;

	uint8_t *cursor, *end;
    uint16_t namesize;
	uint32_t positionCount, resolutionUpdate;

    end = content + length;

	if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"\x31\xff\xff\xff", 4)) == NULL) {
		//Not Found
		return;
	}

    if (cursor + 14 >= end)
		return;

    cursor += 12;

    namesize = *(cursor);
    namesize |= *(cursor+1);

    if (cursor + 2 + namesize + 16 > end)
        return;

    cursor += namesize + 2;

	positionCount = *(cursor + 12);
	positionCount |= *(cursor + 13) << 8;
	positionCount |= *(cursor + 14) << 16;
	positionCount |= *(cursor + 15) << 24;


	if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"\x3c\xff\xff\xff", 4)) == NULL) {
		//Not Found
		return;
	}

	if (cursor + 14 >= end)
		return;

	cursor += 14;

    while (*(char *)cursor++ != '\0') {
        if (cursor >= end)
            return;
    }
    
    if (cursor + 16 > end)
        return;

    resolutionUpdate = *(cursor + 12);
    resolutionUpdate |= *(cursor + 13) << 8;
    resolutionUpdate |= *(cursor + 14) << 16;
    resolutionUpdate |= *(cursor + 15) << 24;

    if (resolutionUpdate > positionCount)
        foxReport("Adobe Acrobat Reader U3D CLODMeshContinuation Code Execution", "CVE-2009-2990", 13, 2, 0, 2);

}

static void cve_2009_2994 (uint8_t *content, uint32_t length, PDFStreamType type) {

    if (type != U3D)
        return;

    uint8_t *cursor, *end;
    uint16_t namesize;
	uint32_t positionCount, minimumResolution;

    end = content + length;

    if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"\x31\xff\xff\xff", 4)) == NULL) {
        //Not Found
        return;
    }

    if (cursor + 14 >= end)
        return;

    cursor += 12;

    namesize = *(cursor);
    namesize |= *(cursor+1);

    if (cursor + 2 + namesize + 56 > end)
        return;

    cursor += namesize + 2;

    positionCount = *(cursor + 12);
    positionCount |= *(cursor + 13) << 8;
    positionCount |= *(cursor + 14) << 16;
    positionCount |= *(cursor + 15) << 24;

    minimumResolution = *(cursor + 52);
    minimumResolution = *(cursor + 53) << 8;
    minimumResolution = *(cursor + 54) << 16;
    minimumResolution = *(cursor + 55) << 24;

	if (minimumResolution > positionCount)
		foxReport("Adobe Acrobat Reader U3D CLODMeshDeclaration Memory Corruption", "CVE-2009-2994", 14, 2, 0, 2);
}

static void cve_2009_2997 (uint8_t *content, uint32_t length, PDFStreamType type) {
}

static void cve_2009_3459 (uint8_t *content, uint32_t length, PDFStreamType type) {

    DecodeParams *DParams;

	if (type != DECODEPARAMS)
		return;

	if (length != sizeof(DecodeParams))
		return;

	DParams = (DecodeParams *)content;

	if (DParams->predictor == 2 && DParams->colors >= 0x3fffffee)
		foxReport("Adobe Acrobat and Adobe Reader Deflate Parameter Integer Overflow", "CVE-2009-3459", 16, 2, 0, 1);

}

static void cve_2009_3955 (uint8_t *content, uint32_t length, PDFStreamType type) {

	uint16_t isshort;
	uint16_t crgn = 0;
	
	if (type != JPX)
		return;

	if (length < 5)
		return;

	isshort = *(content+2) << 8;
	isshort |= *(content+3);

    if (isshort == 5) {
		crgn = *(content+4);
	}
	else if (isshort == 6) {
		if (length < 6)
			return;

		crgn = *(content+4) << 8;
		crgn |= *(content+5);
	}

	if (crgn > 0x80)
		foxReport("Adobe Acrobat and Reader JpxDecode Memory Corruption", "CVE-2009-3955", 17, 2, 0, 1);
	
}

static void cve_2009_4324 (uint8_t *content, uint32_t length, PDFStreamType type) {
	JSArgList *list;

	if (type != JAVASCRIPT)
		return;

	if ((list = JSFindFunction((char *)content, length, "media.newPlayer")) == NULL) {
		//Function not found
		return;
	}

	if (list->count < 1)
		goto END;

	if (strlen(list->vector[0]) == 4)
		if (!memcmp(list->vector[0], "null", 4)) {
	        foxReport("Adobe Reader and Acrobat media.newPlayer Code Execution", "CVE-2009-4324", 18, 2, 0, 1);
		}

END:
	destroyJSArgList(list);
	return;
}

static void cve_2010_0188 (uint8_t *content, uint32_t length, PDFStreamType type) {

    uint8_t endian, *cursor, *end;
	uint16_t numifds;
	uint16_t tagid;
    uint32_t ifdtable;
	uint32_t datacount;

    cursor = 0;

	if (type != UNKNOWN)
		return;

	if (length < 8)
		return;

	if (!memcmp(content, "\x4d\x4d\x00\x2a", 4)) {
		//Big endian
		endian = 1;
	}
	else if (!memcmp(content, "\x49\x49\x2a\x00", 4)) {
		//Little endian
		endian = 0;
	}
	else
		return;

    if (endian == 1) {
		ifdtable = *(content+4) << 24;
		ifdtable |= *(content+5) << 16;
		ifdtable |= *(content+6) << 8;
		ifdtable |= *(content+7);
	}
	else {
		ifdtable = *(content+4);
		ifdtable |= *(content+5) << 8;
		ifdtable |= *(content+6) << 16;
		ifdtable |= *(content+7) << 24;
	}

    if (ifdtable + 2 > length)
		return;

    cursor += ifdtable;

	if (endian == 1) {
		numifds = *(cursor) << 8;
		numifds |= *(cursor+1);
	}
	else {
		numifds = *(cursor);
		numifds |= *(cursor+1) << 8;
	}

	cursor += 2;

    end = content + length;

	while (numifds > 0 && cursor + 8 < end ) {
        
		if (endian == 1) {
			tagid = *(cursor) << 8;
			tagid |= *(cursor+1);
		}
		else {
			tagid = *(cursor);
			tagid |= *(cursor+1) << 8;
		}

		if (tagid == 0x129 || tagid == 0x141 || tagid == 0x212 || tagid == 0x150) {
			if (endian == 1) {
				datacount = *(cursor+4) << 24;
				datacount |= *(cursor+5) << 16;
				datacount |= *(cursor+6) << 8;
				datacount |= *(cursor+7);
			}
			else {
				datacount = *(cursor+4);
				datacount = *(cursor+5) << 8;
				datacount = *(cursor+6) << 16;
				datacount = *(cursor+7) << 24;
			}

			if (datacount > 2)
				foxReport("Adobe Reader and Acrobat Libtiff TIFFFetchShortPair Stack Buffer Overflow", "CVE-2010-0188", 19, 2, 0, 1);
		}

		numifds--;
		cursor += 12;
	}

}

static void cve_2010_0196 (uint8_t *content, uint32_t length, PDFStreamType type) {

	if (type != U3D)
        return;

    uint8_t *cursor, *end;
	uint16_t namesize;
    uint32_t shadingCount;

    end = content + length;

    if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"\x31\xff\xff\xff", 4)) == NULL) {
        //Not Found
        return;
    }

    if (cursor + 14 >= end)
        return;

    cursor += 12;

	namesize = *(cursor);
	namesize |= *(cursor+1);

    if (cursor + 2 + namesize + 36 > end)
        return;

	cursor += namesize + 2;

	shadingCount = *(cursor + 32);
	shadingCount |= *(cursor + 33) << 8;
	shadingCount |= *(cursor + 34) << 16;
	shadingCount |= *(cursor + 35) << 24;

	if (shadingCount >= 0x05d1745e)
		foxReport("Adobe Reader U3D CLODMeshDeclaration Shading Count Buffer Overflow", "CVE-2010-0196", 20, 2, 0, 2);

}

static void cve_2010_2862 (uint8_t *content, uint32_t length, PDFStreamType type) {

    uint8_t *cursor, *end;
    uint16_t maxCompPoints;
	uint32_t maxp_offset;

	if (type != TRUEOPENTYPE)
		return;

    if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"maxp", 4)) == NULL) {
        //Not Found
        return;
    }

    end = content + length;

    if (cursor + 12 > end)
		return;

    maxp_offset = *(cursor+8) << 24;
    maxp_offset |= *(cursor+9) << 16;
    maxp_offset |= *(cursor+10) << 8;
    maxp_offset |= *(cursor+11);

	if (content + maxp_offset + 8 > end)
		return;

	cursor = content + maxp_offset + 6;

    maxCompPoints = *(cursor) << 8;
	maxCompPoints |= *(cursor+1);

	if (maxCompPoints > 0xfff8)
		foxReport("Adobe Acrobat and Reader Font Parsing Integer Overflow", "CVE-2010-2862", 21, 2, 0, 2);

}

static void cve_2010_2883 (uint8_t *content, uint32_t length, PDFStreamType type) {

    uint8_t *cursor, *end;
    uint32_t name_length;
	uint32_t sing_length;
    uint32_t sing_offset;
    uint32_t string_count =0;

	if (type != TRUEOPENTYPE)
		return;

    end = content + length;

    if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"name", 4)) == NULL) {
        //Not Found
        return;
    }

	if (cursor + 16 > end)
		return;

	name_length = *(cursor+12) << 24;
	name_length |= *(cursor+13) << 16;
	name_length |= *(cursor+14) << 8;
	name_length |= *(cursor+15);

    if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"SING", 4)) == NULL) {
        //Not Found
        return;
    }

    if (cursor + 16 > end)
		return;

    sing_length = *(cursor+12) << 24;
    sing_length |= *(cursor+13) << 16;
    sing_length |= *(cursor+14) << 8;
    sing_length |= *(cursor+15);

    if (name_length != 0 || sing_length == 0)
		return;

	sing_offset = *(cursor+8) << 24;
	sing_offset |= *(cursor+9) << 16;
	sing_offset |= *(cursor+10) << 8;
	sing_offset |= *(cursor+11);

	if (content + sing_offset + 16 > end)
		return;

	cursor = content + sing_offset + 16;

	while (cursor < end && *(cursor) != 0) {
		string_count++;
		if (string_count > 28)
			foxReport("Adobe Acrobat and Reader CoolType.dll Stack Buffer Overflow", "CVE-2010-2883", 22, 2, 0, 1);
	}
}

static void cve_2010_3622 (uint8_t *content, uint32_t length, PDFStreamType type) {
    uint8_t *cursor, *end;
	uint32_t numnames;

	if (type != UNKNOWN)
		return;

    if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"mluc\x00\x00\x00\x00", 8)) == NULL) {
        //Not Found
        return;
    }

	end = content + length;

	if (cursor + 12 > end)
		return;

    numnames = *(cursor+8) << 24;
    numnames = *(cursor+9) << 16;
    numnames = *(cursor+10) << 8;
    numnames = *(cursor+11);

	if (numnames > 0x15555555)
		foxReport ("Adobe Acrobat Reader ACE.dll ICC mluc Integer Overflow", "CVE-2010-3622", 23, 2, 0, 2);

}

static void cve_2010_4091 (uint8_t *content, uint32_t length, PDFStreamType type) {
	JSArgList *list;

	if (type != JAVASCRIPT)
		return;

	if ((list = JSFindFunction((char *)content, length, "printSeps")) == NULL) {
		//Function not found
	    return;
	}

	foxReport("Adobe Reader printSeps Memory Corruption", "CVE-2010-4091", 24, 2, 0, 1);

	destroyJSArgList(list);
	return;
}

static void cve_2011_2106 (uint8_t *content, uint32_t length, PDFStreamType type) {
}

static void cve_2011_2462 (uint8_t *content, uint32_t length, PDFStreamType type) {

    if (type != U3D)
        return;

    uint8_t *cursor, *end;
    uint16_t namesize;
    uint32_t shaderListCount, shaderCount;

    end = content + length;

    if ((cursor = (uint8_t *)memmem((const void *)content, length, (const void *)"\x45\xff\xff\xff", 4)) == NULL) {
        //Not Found
        return;
    }

	if (cursor + 6 >= end)
		return;

	namesize = *(cursor + 4);
	namesize |= *(cursor + 5) << 8;

	if (cursor + 6 + namesize + 12 >= end)
		return;

	cursor += namesize + 6 + 8;

	shaderListCount = *(cursor);
	shaderListCount |= *(cursor+1) << 8;
	shaderListCount |= *(cursor+2) << 16;
	shaderListCount |= *(cursor+3) << 24;

    cursor += 4;

	while (shaderListCount != 0) {
		if (cursor + 4 > end)
			return;

		shaderCount = *cursor;
		shaderCount |= *(cursor+1) << 8;
		shaderCount |= *(cursor+2) << 16;
		shaderCount |= *(cursor+3) << 24;

        if (shaderCount == 0)
			foxReport("Adobe Reader and Acrobat U3D Memory Corruption", "CVE-2011-2462", 26, 2, 0, 2);

		cursor += 4;

		if (cursor + 2 > end)
			return;

        namesize = *(cursor);
		namesize |= *(cursor+1) << 8;

		if (cursor + 2 + namesize > end)
			return;

		cursor += namesize + 2;

		shaderListCount--;
	}
}

void __attribute__((constructor)) initDetectionList () {
    
    DetectionByCVE [CVE_2007_0104] = cve_2007_0104;
    DetectionByCVE [CVE_2007_3896] = cve_2007_3896;
    DetectionByCVE [CVE_2007_5659] = cve_2007_5659;
    DetectionByCVE [CVE_2008_2992] = cve_2008_2992;
    DetectionByCVE [CVE_2008_4813] = cve_2008_4813;
    DetectionByCVE [CVE_2009_0658] = cve_2009_0658;
    DetectionByCVE [CVE_2009_0836] = cve_2009_0836;
    DetectionByCVE [CVE_2009_0927] = cve_2009_0927;
    DetectionByCVE [CVE_2009_1492] = cve_2009_1492;
    DetectionByCVE [CVE_2009_1493] = cve_2009_1493;
    DetectionByCVE [CVE_2009_1855] = cve_2009_1855;
    DetectionByCVE [CVE_2009_1856] = cve_2009_1856;
    DetectionByCVE [CVE_2009_2990] = cve_2009_2990;
    DetectionByCVE [CVE_2009_2994] = cve_2009_2994;
    DetectionByCVE [CVE_2009_2997] = cve_2009_2997;
	DetectionByCVE [CVE_2009_3459] = cve_2009_3459;
	DetectionByCVE [CVE_2009_3955] = cve_2009_3955;
	DetectionByCVE [CVE_2009_4324] = cve_2009_4324;
	DetectionByCVE [CVE_2010_0188] = cve_2010_0188;
	DetectionByCVE [CVE_2010_0196] = cve_2010_0196;
	DetectionByCVE [CVE_2010_2862] = cve_2010_2862;
	DetectionByCVE [CVE_2010_2883] = cve_2010_2883;
	DetectionByCVE [CVE_2010_3622] = cve_2010_3622;
    DetectionByCVE [CVE_2010_4091] = cve_2010_4091;
    DetectionByCVE [CVE_2011_2106] = cve_2011_2106;
    DetectionByCVE [CVE_2011_2462] = cve_2011_2462;
}
