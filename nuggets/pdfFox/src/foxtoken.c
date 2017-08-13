#include "foxtoken.h"
#include "foxreport.h"

#include <string.h>
/**Backs up by a single character in a file stream.
 * Used when a lookahead character is consumed and is determined not
 * to be a part of the current token. We back up in the file stream
 * so that the character will be consumed on the next call to
 * the tokenizer rather than skipped entirely. 
 */
bool backupChar(FILE *file, uint32_t length) {
    int distance = 0;

	if (length < 0x80000000) {
		distance = -length;
	}
	else {
		foxLog(FATAL, "%s: Amount to back up exceeds 2 gigs.\n", __func__);
		return false;
	}
	
	if (fseek(file, distance, SEEK_CUR) != 0) {
		foxLog(FATAL, "%s: Could not back up.\n", __func__);
		return false;
	}

	return true;
}

bool consumeToken(FILE *file, uint32_t length, PDFToken *token) {

	//+1 for terminating null
    token->content = calloc(1, length + 1);
    if (token->content == NULL) {
        foxLog(FATAL, "%s: Could not malloc buffer for content.\n", __func__);
		return false;
	}

    //TODO, check against how much file is remaining.
    if(fread(token->content, 1, length, file) != length) {
        foxLog(FATAL, "%s: Could not tokenize stream.\n", __func__);
		return false;
    }

    token->length = length;

	return true;
}

/**Returns true if the character passed to it is one of the
 * PDF whitespace characters.
 */
bool isWhitespace(char c) {
	return (c == '\x00' || c == '\x09' || c == '\x0a' ||
			c == '\x0c' || c == '\x0d' || c == '\x20') ? true : false;
}

/**Returns true if the argument is a delimiter character.
 */
bool isDelimiter(char c) {
	return (c == '/' || c == '[' || c == ']' || c == '(' || c == ')'
			|| c == '<' || c == '>') ? true : false;
}

/**Differentiates between comments and special keywords that begin 
 * with the comment character '%'. On match, changes to the 
 * appropriate token type.
 */
void checkComment(PDFToken *token) {

	if (token->length >= 5) {
			if (!memcmp(token->content, "%%EOF", 5)) {
				token->type = END;
                return;
			}
			else if (*(unsigned char *)(token->content+1) >= 0x80 &&
			    *(unsigned char *)(token->content+2) >= 0x80 &&
			    *(unsigned char *)(token->content+3) >= 0x80 &&
			    *(unsigned char *)(token->content+4) >= 0x80) {
			    token->type = BINHEAD;
				return;
			}
	}

	if (token->length >= 8) {
			if (!memcmp(token->content, "%PDF-", 5) && *(char *)(token->content+6) == '.' &&
                 isdigit(*(char *)(token->content+5)) && isdigit(*(char *)(token->content+7))) {
				token->type = PDFVERS;
                return;
			}
	}

	//Otherwise, it's just a comment
}

/**Differentiates between regular NAME types and specific name keywords
 * that PDF Fox either tracks or requires for parsing.
 */
void checkNameKeyword(PDFToken *token) {
	switch(token->length) {
		case 2:
            if (!memcmp((const char *)token->content, "/S", 2)) {
                token->type = NAME_S;
            }
			break;

        case 3:
            if (!memcmp((const char *)token->content, "/JS", 3)) {
                token->type = NAME_JS;
            }
            break;

		case 4:
			if (!memcmp((const char *)token->content, "/URI", 4)) {
				token->type = NAME_URI;
			}
			break;

		case 6:
			if (!memcmp((const char *)token->content, "/Crypt", 6)) {
				token->type = FILTER_CRYPTDECODE;
			}
			break;

		case 7:
			if (!memcmp((const char *)token->content, "/Length", 7)) {
				token->type = NAME_STRMLEN;
			}
			else if (!memcmp((const char *)token->content, "/Filter", 7)) {
				token->type = FILTER;
			}
			else if (!memcmp((const char *)token->content, "/Colors", 7)) {
				token->type = NAME_DCPRMS_COLORS;
			}
			break;

		case 8:
			if (!memcmp((const char *)token->content, "/Columns", 8)) {
				token->type = NAME_DCPRMS_COLUMNS;
			}
			else if (!memcmp((const char *)token->content, "/Subtype", 8)) {
				token->type = NAME_SUBTYPE;
			}
			break;

		case 9:
			if (!memcmp((const char *)token->content, "/TrueType", 9)) {
				token->type = NAME_TRUETYPE;
			}
			else if (!memcmp((const char *)token->content, "/OpenType", 9)) {
				token->type = NAME_OPENTYPE;
			}
			break;

		case 10:
            if (!memcmp((const char *)token->content, "/DCTDecode", 10)) {
                token->type = FILTER_DCTDECODE;
            }
			else if (!memcmp((const char *)token->content, "/LZWDecode", 10)) {
				token->type = FILTER_LZWDECODE;
			}
			else if (!memcmp((const char *)token->content, "/JPXDecode", 10)) {
				token->type = FILTER_JPXDECODE;
			}
			else if (!memcmp((const char *)token->content, "/Predictor", 10)) {
				token->type = NAME_DCPRMS_PREDICTOR;
			}
			break;

        case 11:
            if (!memcmp((const char *)token->content, "/Javascript", 11)) {
                token->type = NAME_JAVASCRIPT;
            }
            break;

		case 12:
			if (!memcmp((const char *)token->content, "/FlateDecode", 12)) {
			    token->type = FILTER_FLATEDECODE;
			}
			else if (!memcmp((const char *)token->content, "/JBIG2Decode", 12)) {
				token->type = FILTER_JBIG2DECODE;
			}
			break;

		case 13:
			if (!memcmp((const char *)token->content, "/DecodeParams", 13)) {
				token->type = NAME_DECODEPARAMS;
			}
			break;

        case 14:
			if (!memcmp((const char *)token->content, "/ASCII85Decode", 14)) {
				token->type = FILTER_85DECODE;
			}
			break;

		case 15:
			if (!memcmp((const char *)token->content, "/ASCIIHexDecode", 15)) {
				token->type = FILTER_HEXDECODE;
			}
			else if (!memcmp((const char *)token->content, "/CCITTFaxDecode", 15)) {
				token->type = FILTER_CCITTDECODE;
			}
			break;

		case 16:
			if (!memcmp((const char *)token->content, "/RunLengthDecode", 16)) {
				token->type = FILTER_RLEDECODE;
			}
			break;

		case 17:
			if (!memcmp((const char *)token->content, "/BitsPerComponent", 17)) {
				token->type = NAME_DCPRMS_BPC;
			}
			break;

        default:
            //Leave token type as "NAME"
            break;
	}
}

/**Differentiates between syntax-specific keywords and identifiers.
 * 
 */
void checkIdentKeyword(PDFToken *token) {
 
	switch(token->length) {
		case 1:
			if (*(const char *)token->content == 'R')
				token->type = REF;
			break;

        case 3:
            if (!memcmp((const char *)token->content, "obj", 3)) {
                token->type = OBJ;
            }
            break;

        case 4:
            if (!memcmp((const char *)token->content, "null", 4)) {
                token->type = NULLOBJ;
            }
            else if (!memcmp((const char *)token->content, "true", 4)) {
                token->type = TOK_TRUE;
            }
            else if (!memcmp((const char *)token->content, "xref", 4)) {
                token->type = XREF;
            }
            break;

		case 5:
			if (!memcmp((const char *)token->content, "false", 5)) {
				token->type = TOK_FALSE;
			}
			break;

        case 6:
            if (!memcmp((const char *)token->content, "endobj", 6)) {
                token->type = ENDOBJ;
            }
            else if (!memcmp((const char *)token->content, "stream", 6)) {
                token->type = STREAM;
            }
            break;

        case 7:
            if (!memcmp((const char *)token->content, "trailer", 7)) {
                token->type = TRAILER;
            }
            break;

        case 9:
            if (!memcmp((const char *)token->content, "endstream", 9)) {
                token->type = ENDSTREAM;
            }
            else if (!memcmp((const char *)token->content, "startxref", 9)) {
                token->type = STARTXREF;
            }
            break;

        default:
            //Keep ident as token type
            break;
    }
}
                                                                                                           
/**Instantiates and initializes a new container of 
 * type PDFToken to be used and filled out by the 
 * lexical analyzer.
 */
PDFToken *newPDFToken() {
	PDFToken *token = (PDFToken *)calloc(sizeof(PDFToken), 1);
	if (token == NULL) {
		foxLog(FATAL, "%s: Could not allocate space for new token.\n", __func__);
	}
	
	return token;
}

void destroyPDFToken(PDFToken *token) {
	if (token == NULL)
		return;
	if (token->content)
		free(token->content);
	free(token);
}

/**Generalized parsing function.
 * Processes the file stream character-by-character and
 * returns a pointer to a PDFToken.
 * Used by the parser for the majority of situations.
 */
PDFToken *getNextToken(FILE *file) {
	
	int retchar;
	char c;
	uint32_t charCount = 0;
    PDFToken *token;
	PDFTokenizeState state;
	
	if ((token = newPDFToken()) == NULL) {
		foxLog(FATAL, "%s: Could not allocate space for new token.\n", __func__);
		return NULL;
	}
	
	state = START;

#ifdef PDF_FOX_SHOW_DEBUG
    foxLog(PRINT, "\n\t\tFile bytes: ");
#endif


	while ((retchar = fgetc(file)) != EOF) {

        c = (char)retchar;

#ifdef PDF_FOX_SHOW_DEBUG
        foxLog(PRINT, "%c", c);
#endif

		switch (state) {
			case START:

				switch (c) {
					case '/':
						state = INNAME;
						charCount++;
						break;

				    case '<':
						state = INANGLE;
						break;

					case '>':
						state = INCLOSEANGLE;
						break;

					case '%':
						state = INCOMMENT;
					    charCount++;	
						break;

					case '+':
					case '-':
						state = INREAL;
						charCount++;
						break;

					case '[':
						token->type = OPENARRAY;
						goto ACCEPT;

					case ']':
						token->type = CLOSEARRAY;
						goto ACCEPT;

					case '(':
						token->type = OPENPAREN;
						goto ACCEPT;

					case ')':
						token->type = CLOSEPAREN;
						goto ACCEPT;

                    case EOF:
						token->type = ENDOFFILE;
						goto ACCEPT;

					default:
						if (!isWhitespace(c)) {
			    			if (isalpha(c)) {
				    			state = INIDENT;
					    		charCount++;
				    		}
						    else if (isdigit(c)) {
							    state = INNUMBER;	
    							charCount++;
	    					}
		    				else {
			    				foxLog(NONFATAL, "%s: Unrecognized starting character.\n", __func__);
                                goto GNT_ERROR;
					    	}
						}
						break;

				}
				break;

			case INCOMMENT:
				if (c == '\r' || c == '\n') {
					if (!backupChar(file, charCount+1)) {
						goto GNT_FATAL;
					}
                    if (!consumeToken(file, charCount, token)) {
						goto GNT_FATAL;
					}
				    checkComment(token);
					if (token->type == TOK_ERROR) {
						token->type = COMMENT;
					}	
				    
				    retchar = fgetc(file);
					if (retchar == EOF) {
						foxLog(NONFATAL, "%s: Found End-of-File mid-token.\n", __func__);
						goto GNT_ERROR;
					}

                    c = (char)retchar;
                    if (c != '\n') {
						if (!backupChar(file, 1)) {
							goto GNT_FATAL;
						}
					}
					goto ACCEPT;
				}

				charCount++;
				break;

			case INANGLE:
				if (c == '<') {
					token->type = OPENDICT;
				}
				else {
					token->type = OPENANGLE;
                    if (!backupChar(file, 1)) {
						goto GNT_FATAL;
					}
				}
				goto ACCEPT;

            case INCLOSEANGLE:
				if (c == '>') {
					token->type = CLOSEDICT;
				}
				else {
					token->type = CLOSEANGLE;
	                if (!backupChar(file, 1)) {
						goto GNT_FATAL;
					}
				}
				goto ACCEPT;

            case INNAME:
                if (!isWhitespace(c) && !isDelimiter(c)) {
                    charCount++; 
                }
                else {
					if (!backupChar(file, charCount+1)) {
						goto GNT_FATAL;
					}
					if (!consumeToken(file, charCount, token)) {
						goto GNT_FATAL;
					}
                    //checkNameKeyword(token);
                    //if (token->type == ERROR) {
                        token->type = NAME;
                    //}

                    goto ACCEPT;
                }
                break;

            case INIDENT:
				if (!isWhitespace(c) && !isDelimiter(c)) {
				    charCount++;
				}
				else {
					if (!backupChar(file, charCount+1)) {
						goto GNT_FATAL;
					}
					if (!consumeToken(file, charCount, token)) {
						goto GNT_FATAL;
					}
                    checkIdentKeyword(token);
					if (token->type == STREAM) {
						do {
							retchar = fgetc(file);
							if (retchar == EOF) {
								foxLog(NONFATAL, "%s: Found End-of-File after stream token.\n", __func__);
								goto GNT_ERROR;
							}
							c = (char)retchar;
						} while (c != '\n');
					}
					else {
    					if (token->type == TOK_ERROR) {
                            token->type = IDENT;
                        }
					}
                    goto ACCEPT;
				}
				break;

			case INNUMBER:
				if (isdigit(c)) {
				    charCount++;
				}
				else if (c == '.') {
					charCount++;
					state = INREAL;
				}
				else {
                    if (!backupChar(file, charCount+1)) {
						goto GNT_FATAL;
					}
					if (!consumeToken(file, charCount, token)) {
						goto GNT_FATAL;
					}

                    token->type = INTEGER;

                    goto ACCEPT;
				}
				break;

			case INREAL:
                if (isdigit(c) || c == '.') {
                    charCount++;
                }
                else {
                    if (!backupChar(file, charCount+1)) {
						goto GNT_FATAL;	
					}
                    if (!consumeToken(file, charCount, token)) {
						goto GNT_FATAL;
					}

                    token->type = REAL;

                    goto ACCEPT;
                }
				break;

			default:
				foxLog(FATAL, "%s: Could not identify token.\n", __func__);
				goto GNT_FATAL;
				break;
		}
	}

if (retchar == EOF) {
	foxLog(PDF_DEBUG, "%s: End of file.\n", __func__);
    token->type = ENDOFFILE;
    return token;
}

GNT_FATAL:
    foxLog(FATAL, "%s: GNT_FATAL\n", __func__);
    destroyPDFToken(token);
	return NULL;

GNT_ERROR:
	foxLog(NONFATAL, "%s: GNT_ERROR\n", __func__);
    token->type = TOK_ERROR;

ACCEPT:
#ifdef PDF_FOX_SHOW_DEBUG
    foxLog(PRINT, "\n");
#endif
	return token;

}

/**Specific stream parsing function.
 * Processes a file stream measured in character by "length"
 * and stores that content. The content is returned in a PDFToken.
 * Used by the parser for tokenizing stream (arbitrary sequences of
 * bytes).
 */
PDFToken *tokenizeStream(FILE *file, uint32_t length) {
	PDFToken *token;

    //printf("Tokenizing stream with length %d\n", length);

	if ((token = newPDFToken()) == NULL) {
	    foxLog(FATAL, "%s: Could not allocate space for new token.\n", __func__);
		return NULL;
	}

	token->content = calloc(1, length);
	if (token->content == NULL) {
	    foxLog(FATAL, "%s: Could not malloc buffer for comment.\n", __func__);
		destroyPDFToken(token);
		return NULL;
	}

	//TODO, check against how much file is remaining.
    if(fread(token->content, 1, length, file) != length) {
		foxLog(FATAL, "%s: Could not tokenize stream.\n", __func__);
		destroyPDFToken(token);
		return NULL;
	}

    token->length = length;

	return token;
}

/**Specific literal string parsing function.
 * Processes a file stream as a PDF string and stores the 
 * content in a PDFToken. Handles comment parenthesis nesting.
 */
PDFToken *tokenizeLitString(FILE *file) {
	PDFToken *token;
    void *temp;
    int retchar;
	char c;
	bool escape = false;
    uint32_t nestcount = 0;

	if ((token = newPDFToken()) == NULL) {
	    foxLog(FATAL, "%s: Could not allocate space for new token.\n", __func__);
		return NULL;
	}

    token->content = calloc(1, BUF_IDENT_SIZE);
    if (token->content == NULL) {
        foxLog(FATAL, "%s: Could not malloc buffer for comment.\n", __func__);
		destroyPDFToken(token);
		return NULL;
    }

	//TODO monitor end of file
    retchar = fgetc(file);
	for (;;retchar = fgetc(file)) {
		
		if (retchar == EOF) {
			foxLog(NONFATAL, "%s:Found End-of-File mid-lit-string.\n", __func__);
            break;
		}
		
		c = (char)retchar;

		if (escape == false) {
		    if (c == '(')
		        nestcount++;
		    else if (c == ')') {
			    if (nestcount == 0) {
				    if (!backupChar(file, 1)) {
						destroyPDFToken(token);
						return NULL;
					}
				    goto ACCEPT;
			    }
			    else 
				    nestcount--;
		    }
			else if (c == '\\')
				escape = true;
		}
		else {
			escape = false;
		}

		if (token->length % 100 == 0) {
		    if ((temp = realloc(token->content, token->length + BUF_IDENT_SIZE)) == NULL) {
		        foxLog(FATAL, "%s: Could not realloc.\n", __func__);
				destroyPDFToken(token);
				return NULL;
		    }
		    token->content = temp;
		}

		if (escape != true)
		    *(char *)(token->content + token->length++) = c;
	}

	destroyPDFToken(token);

	foxLog(FATAL, "%s: ERROR\n", __func__);

    return NULL;

ACCEPT:
	token->type = LITSTRING;
	return token;
}

/**Specific Hexadecimal string parsing function.
 * Processes a file stream as a hexadecimal string and
 * stores the content in a PDFToken.
 */
PDFToken *tokenizeHexString(FILE *file) {
    PDFToken *token;
	int retchar;
    char c;
	void *temp;

	if ((token = newPDFToken()) == NULL) {
	    foxLog(FATAL, "%s: Could not allocate space for new token.\n", __func__);
		return NULL;
	}

	token->content = calloc(1, BUF_IDENT_SIZE);
	if (token->content == NULL) {
	    foxLog(FATAL, "%s: Could not malloc buffer for comment.\n", __func__);
	    goto ERROR;
	}

	while ((retchar = fgetc(file)) != EOF) {
		c = (char)retchar;
		if (isxdigit(c)) {
            if (token->length % 100 == 0) {
                if ((temp = realloc(token->content, token->length + BUF_IDENT_SIZE)) == NULL) {
                    foxLog(FATAL, "%s: Could not realloc.\n", __func__);
                    goto ERROR;
                }
                token->content = temp;
            }

			*(char *)(token->content + token->length++) = c;
		}
		else if (isWhitespace(c)) {
		}
		else if (c == '>') {
            //backupChar(file);
            goto ACCEPT;
		}
		else {
            foxLog(FATAL, "%s: Invalid character in hex string.\n", __func__);
			goto ERROR;
		}
	}

ERROR:
	if (token->content)
	    free(token->content);
	free(token);

	foxLog(FATAL, "%s: ERROR\n", __func__);
	return NULL;

ACCEPT:
	token->type = HEXSTRING;
    return token;
}

/**Specific Indirect Object Reference parsing function.
 * Processes a file stream until end of ref.
 */
bool tokenizeRef(FILE *file) {
	int retchar;
	char c;

	while ((retchar = fgetc(file)) != EOF) {
		c = (char)retchar;
		if (c == 'R')
			return true;
		else if (!isWhitespace(c)) {
			foxLog(FATAL, "%s: Expected Ref.\n", __func__);
			return false;
		}
	}
 
	foxLog(FATAL, "%s: Unexpected end of file.", __func__);
	return false;
}
