#include "foxparse.h"
#include "foxobjtab.h"
#include "foxdetect.h"
#include "foxreport.h"
#include "foxrecovery.h"
#include "foxsemantics.h"
#include "foxtoken.h"
#include "foxdecode.h"

//Necessaries
static PDFToken *currentToken = NULL;
static FILE *file = NULL;

//Needed for error recovery and token rewinds
static long int lastposition = 0;

//Leaving in for now, but needs fixin'
//Do not like this.
static PDFSyntaxNode * filterList = NULL;

/**
 *
 * Syntax Rules for PDF format
 *
 */
static PDFSyntaxNode *root();          //ROOT = HEADER CONTENT
static PDFSyntaxNode *header();        //HEADER = PDFVERS | PDFVERS BINHEAD
static PDFSyntaxNode *content();       //CONTENT = DATA | DATA CONTENT
static PDFSyntaxNode *data();          //DATA = BODY XREF TRAILER | BODY TRAILER
static PDFSyntaxNode *body();          //BODY = OBJECT | OBJECT BODY
static PDFSyntaxNode *object();        //OBJECT = INTEGER INTEGER obj TYPE endobj
static PDFSyntaxNode *arraycontent();  //ARRAYCONTENT = TYPE | TYPE ARRAYCONTENT
static PDFSyntaxNode *type();          //TYPE = BOOLEAN | NUMBER | LITSRING | HEXSTRING |
                                       //       NAME | ARRAY | NULLOBJ |
                                       //       DICTIONARY | DICTIONARY STREAM
static PDFSyntaxNode *boolean();       //BOOLEAN = true | false
static PDFSyntaxNode *litString();     //LITSTRING = ( litstring )
static PDFSyntaxNode *hexString();     //HEXSTRING = < hexstring >
static PDFSyntaxNode *name();          //NAME = item from list of names
static PDFSyntaxNode *array();         //ARRAY = [ ARRAYCONTENT ]
static PDFSyntaxNode *dictionary();    //DICTIONARY = << ENTRY >>
static PDFSyntaxNode *entry();         //ENTRY = NAME TYPE | NAME TYPE ENTRY
static PDFSyntaxNode *stream();        //STREAM = DICTIONARY stream rawbytes endstream
static PDFSyntaxNode *nullobj();       //NULLOBJ = null
static PDFSyntaxNode *xref();          //XREF = xref SUBSECTION
static PDFSyntaxNode *subsection();    //SUBSECTION = INTEGER INTEGER=numrows numrows*XREFTABLE |
                                       //             INTEGER INTEGER=numrows numrows*XREFTABLE SUBSECTION
static PDFSyntaxNode *trailer();       //TRAILER = trailer DICTIONARY startxref INTEGER END |
                                       //          startxref INTEGER END
static PDFSyntaxNode *number();        //NUMBER = real | integer
static PDFSyntaxNode *integer();       //INTEGER = integer
static PDFSyntaxNode *xreftable();     //XREFTABLE = INTEGER INTEGER IDENT (f|n)
static PDFSyntaxNode *reference();     //REFERENCE = ref

static void destroyNodeTree(PDFSyntaxNode *node);


/**This function attempts to resolve all hitherto unresolved indirect objects
 * once parsing has reached the end of the file.
 *
 * If all indirect references have a corresponding object, this phase
 * should take care of them.
 */
PDFSyntaxNode *resolveAllIndirect() {
    PDFSyntaxNode *node, *temp;
    ObjTableEntry *entry;

    node = getNewNode();
    if (node == NULL)
		return NULL;

    temp = node;
    while ((entry = getNextUnresolved())) {

        if (fseek(file, entry->offset, SEEK_SET) != 0) {
            foxLog(FATAL, "%s: Can't figure out starting pos in file.\n", __func__);
			destroyNodeTree(node);
			return NULL;
        }

        setUnresolved(0);

        currentToken = getNextToken(file);
        if (currentToken == NULL) {
			destroyNodeTree(node);
			return NULL;
		}
		
		temp->sibling = type();

        temp = temp->sibling;

        if (getUnresolved() == 0)
            entry->fullyResolved = 1;
		destroyPDFToken(currentToken);
    }

    return node;
}


/**Match function
 *
 * Compares current token with expected token
 * and then calls the lexical analyzer to supply
 * next token in the file.
 */
static int match(PDFTokenType expected, int free) {
	
	if (currentToken == NULL) {
		foxLog(FATAL, "%s: NULL token.\n", __func__);
		return 0;
	}

	if (currentToken->type == ENDOFFILE) {
        foxLog(FATAL, "%s: Premature end of file.\n", __func__);
        return 0;
    }

	if (currentToken->type == expected) {
        foxLog(PDF_DEBUG, "%s: %s\n", __func__, PDFTokenString[expected]);

		if ((lastposition = ftell(file)) == -1) {
            foxLog(FATAL, "%s: Can't figure out starting pos in file.\n", __func__);
			return 0;
        }

		if (free)
			destroyPDFToken(currentToken);

		currentToken = getNextToken(file);
	
		if (currentToken == NULL)
			return 0;

		if (currentToken->type == TOK_ERROR)
			return 0;

		while(currentToken->type == COMMENT) {
			destroyPDFToken(currentToken);
			currentToken = getNextToken(file);
			if (currentToken == NULL)
				return 0;
			if (currentToken->type == TOK_ERROR)
				return 0;
		}

	}
	else {
		//XXX add some recovery
		//Some error handling
		foxLog(FATAL, "%s: Expected token %s but got token %s.\n", __func__, PDFTokenString[expected], PDFTokenString[currentToken->type]);
		return 0;
	}

	return 1;
}

/**Syntax Node initialization function
 *
 */
PDFSyntaxNode *getNewNode() {
	PDFSyntaxNode *node = (PDFSyntaxNode *)calloc(1, sizeof(PDFSyntaxNode));

    if (node == NULL) {
        foxLog(FATAL, "%s: Out of memory. Could not allocate for new node.\n", __func__);
		return NULL;
	}

	return node;
}

/**Main parsing function.
 *
 * Turns a stream of tokens into a syntax tree
 * using recursive descent.
 */
bool createPDFTree(FILE *l_file) {

	PDFSyntaxNode *node;

	//Create PDF Syntax Tree from file
    file = l_file;
	currentToken = getNextToken(file);
	if (currentToken == NULL) 
		return false;

	node = root();
	destroyPDFToken(currentToken);
    if (node == NULL)
		return false;

	//Go back and resolve any indirect objects 
	//that were previously undefined
    node->child[1] = resolveAllIndirect();

    destroyNodeTree(node);

    return true;
}

/*
 * ROOT = HEADER CONTENT
 */
static PDFSyntaxNode *root() {
	PDFSyntaxNode *node;

    node = header();
	if (node == NULL) {
		return NULL;
	}
	
	node->child[0] = content();
    if (node->child[0] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

/*
 * HEADER = PDFVERS | PDFVERS BINHEAD
 */
static PDFSyntaxNode *header() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

    node->token = currentToken;
	
	if(!match(PDFVERS, 0)) {
		destroyNodeTree(node);
		return NULL;
	}
	
	if (currentToken->type == BINHEAD) {
		if (!match(BINHEAD, 1)) {
			destroyNodeTree(node);
			return NULL;
		}
	}

	return node;
}

/*
 * CONTENT = DATA | DATA CONTENT
 */
static PDFSyntaxNode *content() {
	PDFSyntaxNode *node;

    node = data();
	if (node == NULL) {
		return NULL;
	}

	if (currentToken->type == INTEGER) {
		node->sibling = content();
		if (node->sibling == NULL) {
			destroyNodeTree(node);
			return NULL;
		}
	}

	return node;
}

/*
 * DATA = BODY XREF TRAILER | BODY TRAILER
 */
static PDFSyntaxNode *data() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

    node->child[0] = body();
	if (node->child[0] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	if (currentToken->type == XREF) {
	    node->child[1] = xref();
		if (node->child[1] == NULL) {
			destroyNodeTree(node);
			return NULL;
		}
	}

	node->child[2] = trailer();
	if (node->child[2] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

/*
 * BODY = OBJECT | OBJECT BODY
 */
static PDFSyntaxNode *body() {
    PDFSyntaxNode *node;

    node = object();
	if (node == NULL) {
		return NULL;
	}

	if (currentToken->type == INTEGER) {
		node->sibling = body();
		if (node->sibling == NULL) {
			destroyNodeTree(node);
			return NULL;
		}
	}

	return node;
}

/*
 * OBJECT = INTEGER INTEGER obj TYPE endobj
 */
static PDFSyntaxNode *object() {
    long int pos;
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

	node->child[0] = integer();
	if (node->child[0] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	node->child[1] = integer();
    if (node->child[1] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	//Get current pos
	if ((pos = ftell(file)) == -1) {
		foxLog(FATAL, "%s: Can't figure out starting pos in file.\n", __func__);
		destroyNodeTree(node);
		return NULL;
	}
	
	if (!match(OBJ, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	node->child[2] = type();
	if (node->child[2] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

    //XXX If an object is a stream, then the data added should
	//be the content of that stream (no dictionary)
	//XXX Adds object to the object table so that
	//indirect references to it can be resolved
    if (!object_Sem(node, lastposition-pos, pos)) {
		destroyNodeTree(node);
		return NULL;
	}
	
	if (!match(ENDOBJ, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

int arraycontent_CheckReference(PDFSyntaxNode **node) {
    long int pos;
	uint32_t ret;
	PDFSyntaxNode *temp1, *temp2;

	temp1 = *node;

    if (temp1->token->type != INTEGER)
		return 1;

	if (temp1->sibling->token->type != INTEGER)
		return 1;

    if (!temp1->sibling->sibling)
		return 1;

    if (temp1->sibling->sibling->token->type != REF)
	    return 1;

    /* A -> B -> C -> D
     *        becomes
     *        
     *     C -> D
     *    / \
     *   A   B
     *
     */
     temp2 = temp1;
     temp1 = temp1->sibling->sibling;
     temp1->child[0] = temp2;
     temp1->child[1] = temp2->sibling;
     temp1->child[0]->sibling = NULL;
     temp1->child[1]->sibling = NULL;

     *node = temp1;

	 pos = lastposition;

     ret = resolveIndirect(file, temp1);
     if (ret == 0)
		 return 0;
	 else if (ret == 2)
		 return 1;

     destroyPDFToken(currentToken);
     currentToken = getNextToken(file);
     if (currentToken == NULL) {
		 return 0;
	 }

	 //node->child[2] = arraycontent();
	 temp1->child[2] = type();
     if (temp1->child[2] == NULL)
		 return 0;

     if (fseek(file, pos, SEEK_SET) != 0) {
         foxLog(FATAL, "%s: Can't figure out starting pos in file.\n", __func__);
		 return 0;
	 }
     
	 destroyPDFToken(currentToken);
	 lastposition = pos;
     currentToken = getNextToken(file);
	 if (currentToken == NULL) {
		 return 0;
	 }

     return 1;
}

/*
 * ARRAYCONTENT = TYPE | TYPE ARRAYCONTENT
 */
static PDFSyntaxNode *arraycontent() {
	PDFSyntaxNode *node;

	node = type();
	if (node == NULL) {
		return NULL;
	}
	
	if (currentToken->type != CLOSEARRAY && currentToken->type != ENDOFFILE) {
		node->sibling = arraycontent();
		if (node->sibling == NULL) {
			destroyNodeTree(node);
			return NULL;
		}

		if (arraycontent_CheckReference(&node) == 0) {
		    destroyNodeTree(node);
			return NULL;
		}
	}

	return node;
}

/*
 * TYPE = BOOL | NUMBER | REFERENCE | LITSTRING | HEXSTRING | NAME | ARRAY | NULLOBJ | DICTIONARY | DICTIONARY STREAM
 */
static PDFSyntaxNode *type() {
	PDFSyntaxNode *node = NULL;

    switch (currentToken->type) {
		case TOK_TRUE:
		case TOK_FALSE:
			node = boolean();
			break;

		case REAL:
		case INTEGER:
			node = number();
			break;

		case OPENPAREN:
			node = litString();
			break;

		case OPENANGLE:
			node = hexString();
			break;

		case NAME:
			node = name();
			break;

		case OPENARRAY:
			node = array();
			break;

		case OPENDICT:
			node = dictionary();
			if (currentToken->type == STREAM) {
				node->child[1] = stream();
				if (node->child[1] == NULL) {
					destroyNodeTree(node);
					return NULL;
				}
			}
			break;

		case NULLOBJ:
			node = nullobj();
			break;

        case REF:
			node = reference();
			break;

		default:
			foxLog(FATAL, "%s: Unknown token type %s.\n", __func__, PDFTokenString[currentToken->type]);
			break;

	}

	if (node == NULL)
		return NULL;

	return node;
}

/*
 * BOOL = true | false
 */
static PDFSyntaxNode *boolean() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

	node->token = currentToken;
	if (currentToken->type == TOK_TRUE) {
		if (!match(TOK_TRUE, 0)) {
			destroyNodeTree(node);
			return NULL;
		}
	}
	else {
		if (!match(TOK_FALSE, 0)) {
			destroyNodeTree(node);
			return NULL;
		}
	}

	return node;
}

/*
 * LITSTRING = ( litstring ) 
 */
static PDFSyntaxNode *litString() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

    node->token = tokenizeLitString(file);
	if (node->token == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	if (!match(OPENPAREN, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	if (!match(CLOSEPAREN, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

/*
 * HEXSTRING = < hexstring >
 */
static PDFSyntaxNode *hexString() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

	node->token = tokenizeHexString(file);
    if (node->token == NULL) {
		destroyNodeTree(node);
		return NULL;
	}
	
	if (!match(OPENANGLE, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	//match(CLOSEANGLE);

	return node;
}

/*
 * NAME = name
 *
 * If name is one of a list of recognizable name fields,
 * then we identify it for state tracking.
 */
static PDFSyntaxNode *name() {
    uint32_t streamLength;
	
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

    node->token = currentToken;

    if (!match(NAME, 0)) {
        destroyNodeTree(node);
		return NULL;
	}

	checkNameKeyword(node->token);
    if (node->token->type == TOK_ERROR) {
		node->token->type = NAME;
	}

    if(node->token->type == NAME_STRMLEN) {
		streamLength = (uint32_t)strtoul((char *)currentToken->content, NULL, 10);
		setStreamLength(streamLength);
	}

	return node;
}

/*
 * ARRAY = [ ARRAYCONTENT ]
 */
static PDFSyntaxNode *array() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;
	
	PDFToken *token = newPDFToken();
    if (token == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	token->type = ARRAY;
    node->token = token;

    if (!match(OPENARRAY, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	if (currentToken->type != CLOSEARRAY) {
		node->child[0] = arraycontent();
	
		if (node->child[0] == NULL) {
			destroyNodeTree(node);
			return NULL;
		}
	}

	if (!match(CLOSEARRAY, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

/*
 * DICTIONARY = << ENTRY >>
 */
static PDFSyntaxNode *dictionary() {
	PDFSyntaxNode *node = getNewNode();
	if (node == NULL)
		return NULL;
	
    PDFToken *token = newPDFToken();
	if (token == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	token->type = DICTIONARY;
	node->token = token;

    if (!match(OPENDICT, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	if (currentToken->type != CLOSEDICT) {
	    node->child[0] = entry();
		if (node->child[0] == NULL) {
			destroyNodeTree(node);
			return NULL;
		}
	}
    
	if (!match(CLOSEDICT, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}


int entry_CheckReference(PDFSyntaxNode **node) {

	uint32_t ret;
	long int pos;
    PDFSyntaxNode *temp, *temp2;

    temp2 = *node;

    if (temp2->child[1]->token->type != INTEGER)
		return 1;

    if (currentToken->type != INTEGER) 
		return 1;

    temp2->child[2] = number();
    if (temp2->child[2] == NULL) {
		return 0;
	}

    if (currentToken->type != REF) {
	    foxLog(FATAL, "%s: Invalid Reference.\n", __func__);
		return 0;
	}
               
	temp = temp2->child[1];
    temp2->child[1] = reference();
    if (temp2->child[1] == NULL) {
		return 0;
	}
	
	temp2->child[1]->child[0] = temp;
    temp2->child[1]->child[1] = temp2->child[2];
    temp2->child[2] = NULL;

    *node = temp2;

    pos = lastposition;

	ret = resolveIndirect(file, temp2);
	
	if (ret == 0)	
		return 0;
	else if (ret == 2)
		return 1;

	destroyPDFToken(currentToken);
    currentToken = getNextToken(file);
    temp2->child[2] = type();
	if (temp2->child[2] == NULL) {
		return 0;
	}

    if (fseek(file, pos, SEEK_SET) != 0) {
        foxLog(FATAL, "%s: Can't figure out starting pos in file.\n", __func__);
		return 0;
    }
    
	destroyPDFToken(currentToken);
	lastposition = pos;
	currentToken = getNextToken(file);
	if (currentToken == NULL)
		return 0;

	return 1;
}

/*
 * ENTRY = NAME TYPE | NAME TYPE ENTRY
 */
static PDFSyntaxNode *entry() {
	PDFToken *token;
	PDFStreamType strmtype = UNKNOWN;
    DecodeParams *DParams;

	PDFSyntaxNode *node = getNewNode();
	if (node == NULL)
		return NULL;

    token = currentToken;

	node->child[0] = name();
	if (node->child[0] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	switch (token->type) {
		case NAME_JS:
			strmtype = JAVASCRIPT;
			foxLog(PDF_DEBUG, "%s: Javascript found!\n", __func__);
			break;

		case NAME_URI:
			strmtype = URI;
			foxLog(PDF_DEBUG, "%s: URI found!\n", __func__);
			break;

		case NAME_DECODEPARAMS:
		    strmtype = DECODEPARAMS;
			foxLog(PDF_DEBUG, "%s: DecodeParams found!\n", __func__);
			break;

		case NAME_SUBTYPE:
			strmtype = SUBTYPE;
			break;

		default:
			break;
	}


	node->child[1] = type();
    if (node->child[1] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	if (node->child[0]->token->type == FILTER) {
		filterList = node->child[1];
	}

    if (entry_CheckReference(&node) == 0) {
		destroyNodeTree(node);
		return NULL;
	}

	/*
	 *
	 * XXX STATE TRACKING EPILOGUE
	 * This is terrible. Fix it up.
	 *
	 */
	if (strmtype == SUBTYPE) {
		if (node->child[1]->token) {
			if (node->child[1]->token->type == NAME_TRUETYPE || 
					node->child[1]->token->type == NAME_OPENTYPE) {
                Dig(node->child[1]->token->content, node->child[1]->token->length, TRUEOPENTYPE);
			}
		}
	}
	else if (strmtype == DECODEPARAMS) {
		DParams = prepDecodeParams(node->child[1]);
		if (DParams == NULL)
			foxLog(NONFATAL, "%s: Could not set up decode params.\n", __func__);
		else {
		    Dig((uint8_t *)DParams, sizeof(DecodeParams), DECODEPARAMS);
		    free(DParams);
		}
	}
	else if (node->child[1]->token) {
		//Feed string to Dig()
		Dig(node->child[1]->token->content, node->child[1]->token->length, strmtype);
	}

	/*
	 *
	 * END STATE TRACKING EPILOGUE
	 *
	 */
	
	if (currentToken->type != CLOSEDICT) {
		node->sibling = entry();
		if (node->sibling == NULL) {
		    destroyNodeTree(node);
			return NULL;
		}
	}

	return node;
}

/*
 * STREAM = stream rawbytes endstream
 */
static PDFSyntaxNode *stream() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

	long int streamstart;
	uint32_t streamLength;

    if ((streamstart = ftell(file)) == -1) {
        foxLog(FATAL, "%s: Can't figure out starting pos in file.\n", __func__);
		destroyNodeTree(node);
		return NULL;
    }
    
	streamLength = getStreamLength();

	node->token = tokenizeStream(file, streamLength);
    if (node->token == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

    if (!match(STREAM, 1) || currentToken->type != ENDSTREAM) {
	
        lastposition = streamstart;

		streamLength = recoverStream(file, lastposition);
		if (streamLength == 0) {
			destroyNodeTree(node);
			return NULL;
		}
		destroyPDFToken(node->token);
		node->token = tokenizeStream(file, streamLength);
		if (node->token == NULL) {
			destroyNodeTree(node);
			return NULL;
		}
		
		currentToken = getNextToken(file);
		if (currentToken == NULL) {
			destroyNodeTree(node);
			return NULL;
		}

	}

	setStreamLength(0);
	
    //XXX DECODE STREAMS
    if (filterList != NULL) {
        streamDecode(node->token, filterList);
        filterList = NULL;
    }
 
	if (!match(ENDSTREAM, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

/*
 * NULLOBJ = nullobj
 */
static PDFSyntaxNode *nullobj() {
    PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

	node->token = currentToken;
	if (!match(NULLOBJ, 0)) {
		destroyNodeTree(node);
        return NULL;
	}

	return node;
}

/*
 * XREF = xref SUBSECTION
 */
static PDFSyntaxNode *xref() {
	PDFSyntaxNode *node, *temp;

    if (!match(XREF, 1)) {
		return NULL;
	}

	node = subsection();
	if (node == NULL) {
		return NULL;
	}

	temp = node;
	while (currentToken->type == INTEGER) {
		temp->sibling = subsection();
		if (temp->sibling == NULL) {
		    destroyNodeTree(node);
			return NULL;
		}
		temp = temp->sibling;
	}

	return node;
}

/*
 * SUBSECTION = INTEGER INTEGER=numrows numrows*XREFTABLE |
 *              INTEGER INTEGER=numrows numrows*XREFTABLE SUBSECTION
 */
static PDFSyntaxNode *subsection() {
	PDFSyntaxNode *temp, *node;
	uint32_t numrows = 0;
    uint32_t i = 0;
	node = getNewNode();
    if (node == NULL)
		return NULL;

	node->child[0] = integer();
	if (node->child[0] == NULL) {
		destroyNodeTree(node);
        return NULL;
	}

	node->child[1] = integer();
    if (node->child[1] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}

	//extract value of node->child[1]->token->content
	numrows = (uint32_t)strtoul((char *)node->child[1]->token->content, NULL, 10);

	node->child[2] = getNewNode();
    if (node->child[2] == NULL) {
        destroyNodeTree(node);
        return NULL; 
    }

	//Call xref table with that value
	temp = node->child[2];
	for (i = 0; i < numrows; i++) {
	    temp->sibling = xreftable();
		if (temp->sibling == NULL) {
		    destroyNodeTree(node);
			return NULL;
		}
		temp = temp->sibling;
	}

	return node;
}

/*
 * TRAILER = trailer DICTIONARY startxref INTEGER END | startxref INTEGER END
 */
static PDFSyntaxNode *trailer() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

	if (currentToken->type == TRAILER) {
	    if(!match(TRAILER, 1)) {
		    destroyNodeTree(node);
            return NULL;
		}

	    node->child[0] = dictionary();
		if (node->child[0] == NULL) {
		    destroyNodeTree(node);
            return NULL;
		}
	}

	if (!match(STARTXREF, 1)) {
		destroyNodeTree(node);
        return NULL;
	}

	node->child[1] = integer();
	if (node->child[1] == NULL) {
		destroyNodeTree(node);
        return NULL;
	}
	
	if (!match(END, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

/*
 * NUMBER = real | integer
 */
static PDFSyntaxNode *number() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

    node->token = currentToken;

	if (currentToken->type == REAL) {
		if (!match(REAL, 0)) {
		    destroyNodeTree(node);
            return NULL;
		}
	}
	else {
		if (!match(INTEGER, 0)) {
		    destroyNodeTree(node);
			return NULL;
		}
	}

	return node;
}

/*
 * INTEGER = integer
 */
static PDFSyntaxNode *integer() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

	node->token = currentToken;

    if (!match(INTEGER, 0)) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

/*
 * XREFTABLE = INTEGER INTEGER IDENT (n|f)
 */
static PDFSyntaxNode *xreftable() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

    node->child[0] = integer();
	if (node->child[0] == NULL) {
		destroyNodeTree(node);
	    return NULL;
	}

	node->child[1] = integer();
	if (node->child[1] == NULL) {
		destroyNodeTree(node);
		return NULL;
	}
	
	if (!match(IDENT, 1)) {
		destroyNodeTree(node);
		return NULL;
	}

	return node;
}

/*
 * REFERENCE = ref
 */
static PDFSyntaxNode *reference() {
	PDFSyntaxNode *node = getNewNode();
    if (node == NULL)
		return NULL;

    node->token = currentToken;
	if (!match(REF, 0)) {
		destroyNodeTree(node);
        return NULL;
	}

	return node;
}

static void destroyNodeTree(PDFSyntaxNode *node) {
    if (node == NULL)
		return;
	if (node->child[0] != NULL)
		destroyNodeTree(node->child[0]);
	if (node->child[1] != NULL)
		destroyNodeTree(node->child[1]);
	if (node->child[2] != NULL)
		destroyNodeTree(node->child[2]);
	if (node->sibling != NULL)
		destroyNodeTree(node->sibling);
    if (node->token != NULL)
		destroyPDFToken(node->token);

	free(node);

}
