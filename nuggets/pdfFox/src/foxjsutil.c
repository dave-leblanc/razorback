#include <stdlib.h>
#include <string.h>
#include "foxreport.h"
#include "foxtoken.h"

#define JSARG_VECTOR_INIT_SIZE 10


/*
 * This is our JSArgList structure
 *
 * Contains an argc and argv
 */
typedef struct _JSArgList {
	int count;
	char **vector;
} JSArgList;

/*
 * Basic JSArgList Init function
 *
 * Can initially store up to 10 arguments,
 * but will dynamically realloc if more space
 * is needed.
 */
JSArgList *initJSArgList () {
	JSArgList *list = (JSArgList *)malloc(sizeof(JSArgList));
    if (list == NULL) {
		foxLog(FATAL, "%s: Malloc failure.\n", __func__);
		return NULL;
	}

    list->count = 0;
    
	if ((list->vector = (char **)calloc(JSARG_VECTOR_INIT_SIZE, sizeof(char *))) == NULL) {
	    foxLog(FATAL, "%s: Calloc failure.\n", __func__);
		free(list);
		return NULL;
    }

	return list;
}

/*
 * Basic JSArgList Destroy function
 *
 */
void destroyJSArgList (JSArgList *list) {
	int i;

    if (list == NULL)
		return;

	for (i = 0; i < list->count; i++) {
		if (list->vector[i] != NULL)
			free(list->vector[i]);
	}
	free(list->vector);
	free(list);
}

/*
 * Add an argument to the JSArgList arg vector
 * and increment arg count.
 *
 */
bool addJSArgList(JSArgList *list, char *arg) {
	
    char **temp;

	if (list->count > 0) {
		if (list->count % JSARG_VECTOR_INIT_SIZE == 0) {
			if ((temp = (char **)realloc(list->vector, list->count + JSARG_VECTOR_INIT_SIZE)) == NULL) {
				foxLog(FATAL, "%s: Realloc failure.\n", __func__);
	            free(temp);
				return false;
			}
			list->vector = temp;
		}
	}

	list->vector[list->count] = arg;
	list->count++;

	return true;
}

/*
 * Main JS argument parsing function
 *
 * Takes a pointer to the start of the search
 * buffer (ie: the point just after the location
 * where the JS method is found) and the buffer length.
 *
 * Will walk the method parameters and add them to
 * JSArgList for use by the detection functions.
 *
 */
JSArgList *populateJSArgList (char *start, uint32_t length) {

	JSArgList *list = NULL;
	char *arg = NULL;
    uint32_t i, state, initial, ignore;
    i = 0; state = 0; initial = 0, ignore = 0;

	while (i < length) {
		switch(state) {
			case 0:
				if (start[i] == '\x28') {
					state = 1;
					list = initJSArgList();
				}
				else if (!isWhitespace(start[i])) {
				    foxLog(FATAL, "%s: Unexpected character in JS function argument.\n", __func__);
					destroyJSArgList(list);
					return NULL;
				}
				break;

			case 1:
				//Inside the parameter list
				if (start[i] == '"') {
				    initial = i;
					state = 4;
				}
				else if (start[i] == '\'') {
					initial = i;
					state = 3;
				}
				else if (start[i] == '{') {
					initial = i;
					state = 5;
				}
				else if (start[i] == ')') {
					goto ACCEPT;
				}
				else if (!isWhitespace(start[i]) && start[i] != ',') {
					initial = i;
					state = 2;
				}
				break;

			case 2:
				//Inside an argument
				if (isWhitespace(start[i]) || start[i] == ',' || start[i] == '\x29') {
					//Save argument into variable
					arg = (char *)malloc(i - initial + 1);
					memcpy(arg, start + initial, i - initial);
					arg[i - initial] = '\0';
					//increment arg counter
					if (addJSArgList(list, arg) == false) {
						free(arg);
						destroyJSArgList(list);
						return NULL;
					}
					state = 1;
					if (start[i] == '\x29')
						goto ACCEPT;
				}
				break;

			case 3:
				if (ignore == 0) {
					if (start[i] == '\\') {
						ignore = 1;
					}
					else if (start[i] == '\'') {
						arg = (char *)malloc(i - initial + 2);
						memcpy(arg, start + initial, i + 1 - initial);
						arg[i + 1 - initial] = '\0';
						if (addJSArgList(list, arg) == false) {
							free(arg);
							destroyJSArgList(list);
							return NULL;
						}
						state = 1;
						if (start[i] == '\x29')
						    goto ACCEPT;
					}
				}
				else {
					ignore = 0;
				}
				break;

			case 4:
                if (ignore == 0) {
                    if (start[i] == '\\') {
                        ignore = 1;
                    }
                    else if (start[i] == '\"') {
                        arg = (char *)malloc(i - initial + 2);
                        memcpy(arg, start + initial, i + 1 - initial);
                        arg[i + 1 - initial] = '\0';
                        if (addJSArgList(list, arg) == false) {
							free(arg);
							destroyJSArgList(list);
							return NULL;
						}
                        state = 1;
                        if (start[i] == '\x29')
                            goto ACCEPT;
                    }
                }
                else {
                    ignore = 0;
                }	
				break;

			case 5:
				if (ignore == 0) {
					if (start[i] == '\\') {
						ignore = 1;
					}
					else if (start[i] == '}') {
                        arg = (char *)malloc(i - initial + 2);
                        memcpy(arg, start + initial, i + 1 - initial);
                        arg[i + 1 - initial] = '\0';
                        if (addJSArgList(list, arg) == false) {
							free(arg);
							destroyJSArgList(list);
							return NULL;
						}
                        state = 1;
                        if (start[i] == '\x29')
                            goto ACCEPT;
					}
				}
				else {
					ignore = 0;
				}
				break;

			default:
				foxLog(FATAL, "%s: Reached unknown state in Javascript arglist parsing.\n", __func__);
			    free(arg);
				destroyJSArgList(list);
				return NULL;
				break;
		}

		i++;
	}

ACCEPT:
	return list;
}

/*
 * Main JS utility function
 *
 * Locates the named function in the content buffer
 * and makes all of the arguments available to the 
 * caller by means of the JSArgList structure.
 */
JSArgList *JSFindFunction (char *content, uint32_t length, const char *function) {

    static char *prev_content = NULL;
	static const char *prev_function = NULL;
	static char *prev_cursor = NULL;

	JSArgList *arglist;
	char *cursor, *end_of_content;
	int namelen;

    if (prev_content == content && prev_function == function) {
		length = length - (prev_cursor - content) - 1;
		content = prev_cursor + 1;
	}

	if ((cursor = strstr(content, function)) == NULL) {
		foxLog(PDF_DEBUG, "%s: Function %s not found in Javascript stream.\n", __func__, function);
		prev_content = NULL;
		prev_function = NULL;
		return NULL;
	}

	//When found, skip ahead to end of name
	end_of_content = content + length; //XXX: Maybe sanity check??
	namelen = strlen(function);
	if (cursor + namelen >= end_of_content) {
		//REPORT: We shouldn't get here if strstr succeeded... Fail?
		foxLog(NONFATAL, "%s: JS method search exceeded bounds of buffer.\n", __func__);
		return NULL;
	}
	cursor += namelen;

	//Create ArgList and populate with arg strings
	arglist = populateJSArgList(cursor, end_of_content - cursor);
    if (arglist == NULL) {
		foxLog(NONFATAL, "%s: Error obtaining arglist for Javascript function %s().", __func__, function);
		return NULL;
	}

    prev_content = content;
	prev_function = function;
	prev_cursor = cursor;

	return arglist;
}
