#include "foxdecode.h"
#include "foxdetect.h"
#include "foxreport.h"
#include <netinet/in.h>

static void PDF_hex_decode(PDFToken *token) {

    uint8_t *out;
	uint32_t sizeOut = 0;
	uint8_t hex[2] = {0, 0};
	uint32_t memsize = 0;
    uint32_t i;

	uint8_t *content = (uint8_t *)token->content;
    uint32_t size = token->length;

    for (i = 0; i < size; i++) {
		if (isxdigit(content[i]))
			memsize++;
	}

	if ((out = calloc(1, (memsize % 2 == 0) ? memsize/2 : (memsize+1)/2)) == NULL) {
		foxLog(FATAL, "%s: Malloctile Dysfunction\n", __func__);
		return;
	}
        
	for (i = 0; i < size; i++) {
	    if (isxdigit(content[i])) {
			if (hex[0] == '\0') 
				hex[0] = content[i];
			else {
				hex[1] = content[i];
				//decode and reset buffer
				if (hex[0] >= 0x61 && hex[0] <= 0x66)
					hex[0] -= 0x27;
				if (hex[1] >= 0x61 && hex[1] <= 0x66)
					hex[1] -= 0x27;
				if (hex[0] >= 0x41 && hex[0] <= 0x46)
					hex[0] -= 0x07;
				if (hex[1] >= 0x41 && hex[1] <= 0x46)
					hex[1] -= 0x07;
                out[sizeOut] = ((hex[0] - 0x30) << 4) + (hex[1] - 0x30);
				sizeOut += 1;
				hex[0] = '\0';
				hex[1] = '\0';
			}
		}
		else if (isWhitespace(content[i])) {
		}
		else if (content[i] == '>') {
			if ( i != size-1) {
				foxLog(NONFATAL, "%s: Unexpected end of hex stream\n", __func__);
			}
		}
		else {
			printf("PDF_hex_decode: %02x\n", (uint8_t)content[i]);
			foxLog(NONFATAL, "%s: Unrecognized character in hex decode\n", __func__);
		}
	}

	if (hex[0] != '\0') {
		hex[1] = '0';

        //decode and reset buffer
        if (hex[0] >= 0x61 && hex[0] <= 0x66)
            hex[0] -= 0x27;
        if (hex[1] >= 0x61 && hex[1] <= 0x66)
            hex[1] -= 0x27;
        if (hex[0] >= 0x41 && hex[0] <= 0x46)
            hex[0] -= 0x07;
        if (hex[1] >= 0x41 && hex[1] <= 0x46)
            hex[1] -= 0x07;
        out[sizeOut] = ((hex[0] - 0x30) << 4) + (hex[1] - 0x30);
		sizeOut += 1;
	}

    free(token->content);
	token->content = out;
	token->length = sizeOut; 

	foxLog(PDF_DEBUG, "HexDecode successful!\n");

}

static void PDF_zlib_decode (PDFToken *token, int level) {
    
	int ret;
	uint32_t totalOutSize = 0;
	z_stream strm;

    uint8_t *content = (uint8_t *)token->content;
	uint32_t size = token->length;


    //unsigned char in[ZLIB_CHUNK];
	//unsigned char out[ZLIB_CHUNK];

    uint8_t *out, *temp;

	if ((out = malloc(ZLIB_CHUNK)) == NULL) {
		foxLog(FATAL, "%s: Malloctile dysfunction\n", __func__);
		return;
	}

	totalOutSize = ZLIB_CHUNK;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	if (inflateInit(&strm) != Z_OK) {
		free(out);
		foxLog(NONFATAL, "%s: inflateInit error\n", __func__);
		return;
	}

	strm.avail_in = size;
    strm.next_in = content;
	    
	strm.avail_out = ZLIB_CHUNK;
	strm.next_out = out;

    do {
        foxLog(PDF_DEBUG, "%s: ZLIB inflate...\n", __func__);
		ret = inflate(&strm, Z_SYNC_FLUSH);
		switch (ret) {
			case Z_STREAM_ERROR:
				//fatal error
				foxLog(NONFATAL, "%s: Z_STREAM_ERROR\n", __func__);
				free(out);
				return;
				
			case Z_NEED_DICT:
				//ret = Z_DATA_ERROR;
                (void)inflateEnd(&strm);
                foxLog(NONFATAL, "%s: Z_NEED_DICT ERROR\n", __func__);
				return;
			case Z_DATA_ERROR:
                (void)inflateEnd(&strm);
				/*FILE *file;
				file = fopen("error.blah", "w");
				fwrite(token->content, 1, token->length, file);*/
                foxLog(NONFATAL, "%s: Z_DATA ERROR\n", __func__);
				free(out);
				return;
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				foxLog(NONFATAL, "%s: ZLIB ERROR\n", __func__);
				free(out);
				return;
		}

		if (ret != Z_STREAM_END) {
			if ((temp = realloc(out, totalOutSize + ZLIB_CHUNK)) == NULL) {
				free(out);
				foxLog(FATAL, "%s: Could not realloc.\n", __func__);
				return;
			}

			out = temp;
			strm.next_out = out + totalOutSize;
			strm.avail_out = ZLIB_CHUNK;
			totalOutSize += ZLIB_CHUNK;
		}

		if (ret != Z_STREAM_END && strm.avail_in == 0) {
			foxLog(NONFATAL, "%s: Reached the end of the stream buffer but not done inflating\n.", __func__);
			free(out);
			return;
		}

	} while (ret != Z_STREAM_END);

	(void)inflateEnd(&strm);
	
	free(token->content);
	token->content = out;
	token->length = totalOutSize;
	
	foxLog(PDF_DEBUG, "Inflate successful!\n");
}

static void PDF_ASCII85_decode(PDFToken *token);
void testdec(PDFToken *token) {
	PDF_ASCII85_decode(token);
}

static void PDF_ASCII85_decode(PDFToken *token) {
	
	uint8_t *decoded;
	uint8_t final[5] = {'u', 'u', 'u', 'u', 'u'};
	uint8_t remainder, i = 0;
	uint32_t srcindex = 0;
	uint32_t destindex = 0;
    uint64_t temp = 0;

    uint8_t *content = (uint8_t *)token->content;
	uint32_t length = token->length;

    if ((decoded = (uint8_t *)malloc((length/5+1)*4)) == NULL) {
		foxLog(FATAL, "%s: Malloctle Dysfunction\n", __func__);
		return;
	}

	if (length <= 2) {
		foxLog(NONFATAL, "%s: Length of stream smaller than minimum expected.\n", __func__);
		free(decoded);
		return;
	}

	length -= 2;

	//TODO: handle whitespace
	while (length - srcindex >= 5) {

		if (content[srcindex] == 'z') {
			*(uint32_t *)(decoded + destindex) = 0x00000000;
			srcindex++;
		}
		else {
			foxLog(PDF_DEBUG, "%02x%02x%02x%02x%02x\n", content[srcindex], content[srcindex+1], content[srcindex+2], content[srcindex+3], content[srcindex+4]);
		    temp = (content[srcindex]-33)*85*85*85*85 + 
				   (content[srcindex+1]-33)*85*85*85 +
			       (content[srcindex+2]-33)*85*85 + 
				   (content[srcindex+3]-33)*85 + 
				   (content[srcindex+4]-33);
			if (temp > 0xffffffff) {
				foxLog(FATAL, "%s: Resultant value exceeds 4-byte max value.\n", __func__);
			}

		    *(uint32_t *)(decoded + destindex) = ntohl((uint32_t)temp);
			
			srcindex += 5;
		}
		destindex += 4;
	}

	remainder = length - srcindex;

	if (remainder == 1 ) {
		free(decoded);
		foxLog(NONFATAL, "%s: Can't have a 1 remainder.\n", __func__);
		return;
	}

	if (remainder >= 2 ) {
	    while (srcindex < length) {
			final[i] = content[srcindex++];
			i++;
		}
        //Pad with u's
		temp = (final[0]-33)*85*85*85*85 + (final[1]-33)*85*85*85 +
		       (final[2]-33)*85*85 + (final[3]-33)*85
			   + (final[4]-33);
        memcpy(decoded, final, remainder-1);
		//Decode and remove as many bytes as u's added
	}

	free(token->content);
    token->content = decoded;
	token->length = destindex + remainder - 1;
	foxLog(PDF_DEBUG, "ASCII85 Decode successful!\n");
}

/*
 * Unimplemented
 */
/*
static uint8_t *PDF_DCT_decode (uint8_t length, uint8_t *content) {
}

static uint8_t *PDF_JPX_decode (uint8_t length, uint8_t *content) {
}
*/

static void PDF_RLE_decode (PDFToken *token) {

    uint8_t *out = NULL;
	uint32_t sizeOfBuf = token->length;
	uint32_t bytesWritten = 0;
	uint32_t offset = 0;

    uint8_t *content = (uint8_t *)token->content;
	uint32_t length = token->length;

	if ((out = (uint8_t *)malloc(sizeOfBuf)) == NULL) {
		foxLog(FATAL, "%s: Malloctile dysfunction.\n", __func__);
	    return;
	}

	//TODO: Check exceed source buffer
	while (offset < length) {

		if (content[offset] < 128) {
			if (content[offset] + 1 + bytesWritten > sizeOfBuf) {
                //Realloc
			}
			memcpy(out+bytesWritten, content+offset+1, content[offset]+1);
			offset += content[offset] + 2;
			bytesWritten += content[offset] + 1;
			//Write byte + 1 bytes
			//Skip over byte + 1 bytes
		}
		else if (content[offset] > 128) {
			if (257 - content[offset] + bytesWritten > sizeOfBuf) {
				//Realloc
			}
			memcpy(out+bytesWritten, content+offset+1, 257 - content[offset]);
			offset += 1 + 257 - content[offset];
			bytesWritten += 257 -content[offset];
			//Write 257 - byte copies of following byte
			//Skip over next byte
		}
		else if (content[offset] == 128) {
			//END OF DATA
			break;
		}
	}

    free(token->content);
	token->content = out;
	token->length = bytesWritten;

	foxLog(PDF_DEBUG, "RLE Decode successful!\n");
}

void streamDecode(PDFToken *token, PDFSyntaxNode *filter) {

    if (filter == NULL) {
		foxLog(NONFATAL, "%s: Filter list is NULL.\n", __func__);
		return;
	}

	PDFSyntaxNode *temp;
	
	if (filter->child[0] != NULL)
		temp = filter->child[0];
	else
		temp = filter;

	while (temp != NULL) {

		switch (temp->token->type) {
			case FILTER_85DECODE:
				//printf("Calling 85 decode...\n");
				PDF_ASCII85_decode(token);
				break;

			case FILTER_RLEDECODE:
				//printf("Calling RLE decode...\n");
				PDF_RLE_decode(token);
				break;

			case FILTER_FLATEDECODE:
				//printf("Calling Flate decode...\n");
			    foxLog(PDF_DEBUG, "Length: %d\n", token->length);
				foxLog(PDF_DEBUG, "%02x %02x\n", token->content[0], token->content[token->length-1]);
				PDF_zlib_decode(token, 0);
				break;

			case FILTER_HEXDECODE:
				//printf("Calling Hex decode...\n");
				PDF_hex_decode(token);
				break;

			case FILTER_JPXDECODE:
				Dig(token->content, token->length, JPX);
				break;

			case FILTER_DCTDECODE:
			case FILTER_LZWDECODE:
			case FILTER_CCITTDECODE:
			case FILTER_CRYPTDECODE:
				break;

			case FILTER_JBIG2DECODE:
				Dig(token->content, token->length, JBIG2);
				break;

			default:
				foxLog(NONFATAL, "%s: Unknown filter type.\n", __func__);
				break;
		}

		temp = temp->sibling;
	}

}
