#include "foxreport.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 *
 * INPUT: starting position of stream
 * OUTPUT: New length of stream
 *
 * FUNCTION: Will look for the end of the stream by searching
 *           for an "endstream" token, and, if found, will
 *           return the estimated actual length of the 
 *           stream and restore the file pointer to
 *           the beginning of the stream.
 *
 */
uint32_t recoverStream (FILE *file, long int lastposition) {

    long int seekpos = lastposition;
    uint8_t buffer[1024];
    uint8_t *instance;
    uint32_t bytesRead;
	uint32_t actualStrmLen;

    while (!feof(file)) {
        if (fseek(file, seekpos, SEEK_SET) != 0) {
            foxLog(FATAL, "%s: Can't seek to pos in file.\n", __func__);
			return 0;
        }

        if ((bytesRead = fread(buffer, 1, 1024, file)) < 1024) {
            if (!feof(file)) {
                foxLog(FATAL, "%s: Can't read from file.\n", __func__);
				return 0;
			}
        }

        if ((instance = (uint8_t *)memmem(buffer, bytesRead, (const void *)"\nendstream\n", 11)) != NULL) {
            
			actualStrmLen = seekpos - lastposition + (instance - buffer);
            
			if (fseek(file, lastposition, SEEK_SET) != 0) {
                foxLog(FATAL, "%s: Can't seek to pos in file.\n", __func__);
				return 0;
            }
            
            return actualStrmLen;
        }

		//Seek to 1024 - 10 bytes
		//This is in case endstream was on the buffer border
        seekpos = seekpos + 1014;
    }

	//Failed to recover. Fail out.
    foxLog(FATAL, "%s: Was unable to recover. Exiting.\n", __func__);

    return 0;
}
