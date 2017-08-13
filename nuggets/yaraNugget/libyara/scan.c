/*

Copyright(c) 2007. Victor M. Alvarez [plusvic@gmail.com].

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include <string.h>
#include <ctype.h>

#include "filemap.h"
#include "yara.h"
#include "eval.h"
#include "ast.h"
#include "exe.h"
#include "mem.h"
#include "eval.h"
#include "regex.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef WIN32
#define inline __inline
#endif


static char lowercase[256];
static char isalphanum[256];

/* Function implementations */

#ifdef SSE42

#include <nmmintrin.h>

static char sdeltas[16] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
static char szeroes[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char sranges[2]  = {0x61, 0x7a};

int inline compare(char* str1, char* str2, int len)
{
	__m128i s1, s2;

	int c, result = 0;

	do {

    	s1 = _mm_lddqu_si128((const __m128i*) str1);
		s2 = _mm_lddqu_si128((const __m128i*) str2);

		c = _mm_cmpestri(s1, len - result, s2, 16, _SIDD_CMP_EQUAL_EACH | _SIDD_MASKED_NEGATIVE_POLARITY);

		str1 += 16;
		str2 += 16;

		result += c;

	} while(c == 16);
	
	return ((result==len) ? result : 0);
}

int inline icompare(char* str1, char* str2, int len)
{ 
	__m128i s1, s2, ranges, zeroes, deltas, mask;

	int c, result = 0;

	ranges = _mm_loadu_si128((const __m128i*) sranges);  
	deltas = _mm_loadu_si128((const __m128i*) sdeltas);
	zeroes = _mm_loadu_si128((const __m128i*) szeroes);

	do {

	    s1 = _mm_lddqu_si128((const __m128i*) str1);
		s2 = _mm_lddqu_si128((const __m128i*) str2);

 		// producing mask, 0xFF for lowercases, 0x00 for the rest
		mask = _mm_cmpestrm(ranges, 2, s1, len - result, _SIDD_CMP_RANGES | _SIDD_UNIT_MASK);       
        
		// producing mask, 0x20 for lowercases, 0x00 for the rest
		mask = _mm_blendv_epi8(zeroes, deltas, mask); 

		s1 = _mm_sub_epi8(s1, mask);  

 		// producing mask, 0xFF for lowercases, 0x00 for the rest
		mask = _mm_cmpestrm(ranges, 2, s2, 16, _SIDD_CMP_RANGES | _SIDD_UNIT_MASK);       
        
		// producing mask, 0x20 for lowercases, 0x00 for the rest
		mask = _mm_blendv_epi8(zeroes, deltas, mask);

		s2 = _mm_sub_epi8(s2, mask); 

		c = _mm_cmpestri(s1, len - result, s2, 16, _SIDD_CMP_EQUAL_EACH | _SIDD_MASKED_NEGATIVE_POLARITY);

		str1 += 16;
		str2 += 16;

		result += c;

	} while(c == 16);
	
	return ((result==len) ? result : 0);
}


#else

inline int compare(char* str1, char* str2, int len)
{
	char* s1 = str1;
	char* s2 = str2;
	int i = 0;
	
	while (i < len && *s1++ == *s2++) 
	{
	    i++;
    }

	return ((i==len) ? i : 0);
}

inline int icompare(char* str1, char* str2, int len)
{
	char* s1 = str1;
	char* s2 = str2;
	int i = 0;
	
	while (i < len && lowercase[*s1++] == lowercase[*s2++]) 
	{
	    i++;
    }
	
	return ((i==len) ? i : 0);
}

#endif




inline int wcompare(char* str1, char* str2, int len)
{
	char* s1 = str1;
	char* s2 = str2;
	int i = 0;

	while (i < len && *s1 == *s2) 
	{
		s1++;
		s2+=2;
		i++;
	}
	
	return ((i==len) ? i * 2 : 0);
}

inline int wicompare(char* str1, char* str2, int len)
{
	char* s1 = str1;
	char* s2 = str2;
	int i = 0;

	while (i < len && lowercase[*s1] == lowercase[*s2]) 
	{
		s1++;
		s2+=2;
		i++;
	}
	
	return ((i==len) ? i * 2 : 0);
}

 
int hex_match(unsigned char* buffer, size_t buffer_size, unsigned char* pattern, int pattern_length, unsigned char* mask)
{
	size_t b,p,m;
	unsigned char i;
	unsigned char distance;
	unsigned char delta;
    int match;
    int match_length;
    int longest_match;
	int matches;
    int tmp, tmp_b;
	
	b = 0;
	p = 0;
	m = 0;
	
	matches = 0;	
	
	while (b < (size_t) buffer_size && p < (size_t) pattern_length)
	{
		if (mask[m] == MASK_EXACT_SKIP)
		{
			m++;
			distance = mask[m++];
			b += distance;
			matches += distance;
		}
		else if (mask[m] == MASK_RANGE_SKIP)
		{
			m++;
			distance = mask[m++];
			delta = mask[m++] - distance;
			b += distance;
			matches += distance;
			
            i = 0;
                        
            while (i <= delta && b + i < buffer_size)
            {
                if ((buffer[b + i] & mask[m]) == pattern[p])
                {
       			    tmp = hex_match(buffer + b + i, buffer_size - b - i,  pattern + p, pattern_length - p, mask + m);
       			}
       			else
       			{
                    tmp = 0;
       			}
				
			    if (tmp > 0) 
					return b + i + tmp;
				
                i++;      
            }
			
			break;	
		}
		else if (mask[m] == MASK_OR)
		{		    
            longest_match = 0;
            		    
		    while (mask[m] != MASK_OR_END)
		    {
                tmp_b = b;
                match = TRUE;
                match_length = 0;
                m++;
		        
		        while (mask[m] != MASK_OR && mask[m] != MASK_OR_END)
                {
                    if ((buffer[tmp_b] & mask[m]) != pattern[p])
                    {
                        match = FALSE;
                    }
                    
                    if (match)
                    {
                        match_length++;
                    }
                
                    tmp_b++;
                    m++;
                    p++;                    
                }
		        
		        if (match && match_length > longest_match)
		        {
                    longest_match = match_length;
		        }	     
		    }
		    
            m++;
		    
		    if (longest_match > 0)
		    {
                b += longest_match;
                matches += longest_match;
            }
            else
            {
                matches = 0;
                break;
            }
    
		}
		else if ((buffer[b] & mask[m]) == pattern[p])  
		{
			b++;
			m++;
			p++;
			matches++;
		}
		else  /* do not match */
		{
			matches = 0;
			break;
		}
	}
	
	if (p < (size_t) pattern_length)  /* did not reach the end of pattern because buffer was too small */
	{
		matches = 0;
	}
	
	return matches;
}

int regexp_match(unsigned char* buffer, size_t buffer_size, unsigned char* pattern, int pattern_length, REGEXP re, int file_beginning)
{
	int result = 0;
	
	// if we are not at the beginning of the file, and 
	// the pattern begins with ^, the string doesn't match
	if (file_beginning && pattern[0] == '^')
	{
		return 0;
	}

    result = regex_exec(&re, (char *)buffer, buffer_size);

    if (result >= 0)
        return result;
    else	
	    return 0;
}

int populate_hash_table(HASH_TABLE* hash_table, RULE_LIST* rule_list)
{
	RULE* rule;
	STRING* string;
	STRING_LIST_ENTRY* entry;
	unsigned char x,y;
    char hashable_2b;
    char hashable_1b;
    int i, next;
    
    for (i = 0; i < 256; i++)
    {
        lowercase[i] = tolower(i);
        isalphanum[i] = isalnum(i);
    }
		
	rule = rule_list->head;
	
	while (rule != NULL)
	{
		string = rule->string_list_head;

		while (string != NULL)
		{	        
			if (string->flags & STRING_FLAGS_REGEXP)
			{	
				/* take into account anchors (^) at beginning of regular expressions */
							
				if (string->string[0] == '^')
				{
				    if (string->length > 2)
				    {
					    x = string->string[1];
					    y = string->string[2];
					}
					else
					{
                        x = 0;
                        y = 0; 
					}
				}
				else
				{
					x = string->string[0];
					y = string->string[1];
				}
			
                hashable_2b = isalphanum[x] && isalphanum[y];
                hashable_1b = isalphanum[x];
			}
			else
			{
			    x = string->string[0];
				y = string->string[1];
				
				hashable_2b = TRUE;
				hashable_1b = TRUE;
				
			} /* if (string->flags & STRING_FLAGS_REGEXP) */
			
			if (string->flags & STRING_FLAGS_HEXADECIMAL)
			{
			    hashable_2b = (string->mask[0] == 0xFF) && (string->mask[1] == 0xFF);
			    hashable_1b = (string->mask[0] == 0xFF);
			}
			
			if (hashable_1b && string->flags & STRING_FLAGS_NO_CASE)
			{	
			    if (hashable_2b)
			    {	    
    			    /* 
    			       if string is case-insensitive add an entry in the hash table
    			       for each posible combination 
    			    */
			    
    				x = lowercase[x];
    				y = lowercase[y];
				
    				/* both lowercases */
				
    				entry = (STRING_LIST_ENTRY*) yr_malloc(sizeof(STRING_LIST_ENTRY));
				
    				if (entry == NULL)
        			    return ERROR_INSUFICIENT_MEMORY;
    			    
        			entry->next = hash_table->hashed_strings_2b[x][y];
        			entry->string = string;
        			hash_table->hashed_strings_2b[x][y] = entry;
    			
        			/* X uppercase Y lowercase */
    			
                    x = toupper(x);
				
    				entry = (STRING_LIST_ENTRY*) yr_malloc(sizeof(STRING_LIST_ENTRY));
				
    				if (entry == NULL)
                        return ERROR_INSUFICIENT_MEMORY;
    			    
            		entry->next = hash_table->hashed_strings_2b[x][y];  
            		entry->string = string;
            		hash_table->hashed_strings_2b[x][y] = entry; 
        		
            		/* both uppercases */			    
    			
        			y = toupper(y);  
    			    
        			entry = (STRING_LIST_ENTRY*) yr_malloc(sizeof(STRING_LIST_ENTRY));
				
    				if (entry == NULL)
                        return ERROR_INSUFICIENT_MEMORY;
    			    
            		entry->next = hash_table->hashed_strings_2b[x][y];
            		entry->string = string;
            		hash_table->hashed_strings_2b[x][y] = entry;
        		
            		/* X lowercase Y uppercase */
    			    
                    x = lowercase[x];
 
        			entry = (STRING_LIST_ENTRY*) yr_malloc(sizeof(STRING_LIST_ENTRY));
				
    				if (entry == NULL)
                        return ERROR_INSUFICIENT_MEMORY;
    			    
            		entry->next = hash_table->hashed_strings_2b[x][y]; 
            		entry->string = string; 
            		hash_table->hashed_strings_2b[x][y] = entry;
        		}
        		else
        		{
        		    /* lowercase */
    
        		    x = lowercase[x];
		
    				entry = (STRING_LIST_ENTRY*) yr_malloc(sizeof(STRING_LIST_ENTRY));
				
    				if (entry == NULL)
        			    return ERROR_INSUFICIENT_MEMORY;
    			    
        			entry->next = hash_table->hashed_strings_1b[x];
        			entry->string = string;
        			hash_table->hashed_strings_1b[x] = entry;
        			
        			/* uppercase */
    
        		    x = toupper(x);
		
    				entry = (STRING_LIST_ENTRY*) yr_malloc(sizeof(STRING_LIST_ENTRY));
				
    				if (entry == NULL)
        			    return ERROR_INSUFICIENT_MEMORY;
    			    
        			entry->next = hash_table->hashed_strings_1b[x];
        			entry->string = string;
        			hash_table->hashed_strings_1b[x] = entry;
        		}               
    							
			}
			else if (hashable_1b)
			{
			    entry = (STRING_LIST_ENTRY*) yr_malloc(sizeof(STRING_LIST_ENTRY));
			
				if (entry == NULL)
                    return ERROR_INSUFICIENT_MEMORY;
                
                entry->string = string; 
			    
			    if (hashable_2b)
			    {   
            		entry->next = hash_table->hashed_strings_2b[x][y]; 		
            		hash_table->hashed_strings_2b[x][y] = entry;
        		}
        		else
        		{    			    
        			entry->next = hash_table->hashed_strings_1b[x];
        			hash_table->hashed_strings_1b[x] = entry;
        		}  
			}
			else /* non hashable */
			{
			    entry = (STRING_LIST_ENTRY*) yr_malloc(sizeof(STRING_LIST_ENTRY));
				
				if (entry == NULL)
                    return ERROR_INSUFICIENT_MEMORY;
			    
			    entry->next = hash_table->non_hashed_strings;
			    entry->string = string; 
                hash_table->non_hashed_strings = entry;
			}
		
			string = string->next;
		}
		
		rule = rule->next;
	}
	
    hash_table->populated = TRUE;
	
	return ERROR_SUCCESS;
}


void clear_hash_table(HASH_TABLE* hash_table)
{
	int i,j;
	
	STRING_LIST_ENTRY* next_entry;
	STRING_LIST_ENTRY* entry;

	for (i = 0; i < 256; i++)
	{
	    entry = hash_table->hashed_strings_1b[i];
			
		while (entry != NULL)
		{
			next_entry = entry->next;
			yr_free(entry);
			entry = next_entry;
		}
		
		hash_table->hashed_strings_1b[i] = NULL;
	    
		for (j = 0; j < 256; j++)
		{
			entry = hash_table->hashed_strings_2b[i][j];
				
			while (entry != NULL)
			{
				next_entry = entry->next;
				yr_free(entry);
				entry = next_entry;
			}
			
			hash_table->hashed_strings_2b[i][j] = NULL;
		}
	}
	
    entry = hash_table->non_hashed_strings;
    
    while (entry != NULL)
	{
		next_entry = entry->next;
		yr_free(entry);
		entry = next_entry;
	}
	
    hash_table->non_hashed_strings = NULL;
}

void clear_marks(RULE_LIST* rule_list)
{
	RULE* rule;
	STRING* string;
	MATCH* match;
	MATCH* next_match;
	
	rule = rule_list->head;
	
	while (rule != NULL)
	{	 
	    rule->flags &= ~RULE_FLAGS_MATCH;
	    string = rule->string_list_head;
		
		while (string != NULL)
		{
			string->flags &= ~STRING_FLAGS_FOUND;  /* clear found mark */
			
			match = string->matches_head;
			
			while (match != NULL)
			{
				next_match = match->next;
				yr_free(match);
				match = next_match;
			}
			
			string->matches_head = NULL;
            string->matches_tail = NULL;
			string = string->next;
		}
		
		rule = rule->next;
	}
}

inline int string_match(unsigned char* buffer, size_t buffer_size, STRING* string, int flags, int negative_size)
{
	int match;
	int i, len;
	int is_wide_char;
	
	unsigned char tmp_buffer[512];
	unsigned char* tmp;
	
	if (IS_HEX(string))
	{
		return hex_match(buffer, buffer_size, string->string, string->length, string->mask);
	}
	else if (IS_REGEXP(string)) 
	{
		if (IS_WIDE(string))
		{
			i = 0;
			
			while(  i < buffer_size - 1 && 
			        buffer[i] >= 32 &&        // buffer[i] is a ... 
			        buffer[i] <= 126 &&       // ... printable character
			        buffer[i + 1] == 0)
			{
				i += 2;
			}
						
			len = i/2;
						
			if (len > sizeof(tmp_buffer))
			{
			    tmp = yr_malloc(len);
            }
            else 
            {
                tmp = tmp_buffer;
            }
            
            i = 0;
			
			if (tmp != NULL)
			{						
				while(i < len)
				{
					tmp[i] = buffer[i*2];
					i++;
				}
								
				match = regexp_match(tmp, len, string->string, string->length, string->re, (negative_size > 2));
			    
			    if (len > sizeof(tmp_buffer))
			    {
				    yr_free(tmp);			
				} 
				    
				return match * 2;
			}
			
		}
		else
		{
			return regexp_match(buffer, buffer_size, string->string, string->length, string->re, negative_size);
		}
	}
	
	if ((flags & STRING_FLAGS_WIDE) && IS_WIDE(string) && string->length * 2 <= buffer_size)
	{	
		if(IS_NO_CASE(string))
		{
			match = wicompare((char*) string->string, (char*) buffer, string->length);			
		}
		else
		{
			match = wcompare((char*) string->string, (char*) buffer, string->length);		
		}
		
		if (match > 0 && IS_FULL_WORD(string))
		{
			if (negative_size >= 2)
			{
				is_wide_char = (buffer[-1] == 0 && isalphanum[(char) (buffer[-2])]);
				
				if (is_wide_char)
				{
					match = 0;
				}
			}
			
			if (string->length * 2 < buffer_size - 1)
			{
				is_wide_char = (isalphanum[(char) (buffer[string->length * 2])] && buffer[string->length * 2 + 1] == 0);
				
				if (is_wide_char)
				{
					match = 0;
				}
			}
		}	
		
		if (match > 0)
            return match;
	}
	
	if ((flags & STRING_FLAGS_ASCII) && IS_ASCII(string) && string->length <= buffer_size)
	{		
		if(IS_NO_CASE(string))
		{
			match = icompare((char*) string->string, (char*) buffer, string->length);			
		}
		else
		{
			match = compare((char*) string->string, (char*) buffer, string->length);		
		}
				
		if (match > 0 && IS_FULL_WORD(string))
		{
			if (negative_size >= 1 && isalphanum[(char) (buffer[-1])])
			{
				match = 0;
			}
			else if (string->length < buffer_size && isalphanum[(char) (buffer[string->length])])
			{
				match = 0;
			}
		}
		
		return match;
	}
	
	return 0;
}


inline int find_matches_for_strings(   STRING_LIST_ENTRY* first_string, 
                                unsigned char* buffer, 
                                size_t buffer_size,
                                size_t current_offset,
                                int flags, 
                                int negative_size)
{
	int len;
	
	STRING* string;
	MATCH* match;
    STRING_LIST_ENTRY* entry = first_string;
    
   	while (entry != NULL)
	{	
		string = entry->string;
		entry = entry->next;
		
		if ((string->flags & STRING_FLAGS_FOUND) && (string->flags & STRING_FLAGS_FAST_MATCH))
		{
			continue;
		}
		
		if ( (string->flags & flags) && (len = string_match(buffer, buffer_size, string, flags, negative_size)))
		{
		    /*  
		        If this string already matched we must check that this match is not 
		        overlapping a previous one. This can occur for example if we search 
		        for the string 'aa' and the file contains 'aaaaaa'. 
		     */
		     
		    if ((string->matches_tail == NULL) ||
		        (string->matches_tail->offset + string->matches_tail->length <= current_offset))
		    {		    
    			string->flags |= STRING_FLAGS_FOUND;
    			match = (MATCH*) yr_malloc(sizeof(MATCH));
    			match->data = (unsigned char*) yr_malloc(len);

    			if (match != NULL && match->data != NULL)
    			{
    				match->offset = current_offset;
    				match->length = len;
    				match->next = NULL;
                    
                    memcpy(match->data, buffer, len);         
    				
    				if (string->matches_head == NULL)
    				{
                        string->matches_head = match;
    				}
    				
    				if (string->matches_tail != NULL)
    				{
                        string->matches_tail->next = match;
    				}
    				
    				string->matches_tail = match;
    				 				
    			}
    			else
    			{
                    if (match != NULL) 
                        yr_free(match);
    			    
    				return ERROR_INSUFICIENT_MEMORY;
    			}
		    }
		}		
	}
	
    return ERROR_SUCCESS;
}


int find_matches(	unsigned char first_char, 
					unsigned char second_char, 
					unsigned char* buffer, 
					size_t buffer_size, 
					size_t current_offset,
					int flags,
					int negative_size, 
					YARA_CONTEXT* context)
{
	
    int result = ERROR_SUCCESS;
    	
    if (context->hash_table.hashed_strings_2b[first_char][second_char] != NULL)
    {
        result =  find_matches_for_strings( context->hash_table.hashed_strings_2b[first_char][second_char], 
                                            buffer, 
                                            buffer_size, 
                                            current_offset, 
                                            flags, 
                                            negative_size);
    }
    
    
    if (result == ERROR_SUCCESS && context->hash_table.hashed_strings_1b[first_char] != NULL)
    {
        result =  find_matches_for_strings( context->hash_table.hashed_strings_1b[first_char], 
                                            buffer, 
                                            buffer_size, 
                                            current_offset, 
                                            flags, 
                                            negative_size);
    }
    
    if (result == ERROR_SUCCESS && context->hash_table.non_hashed_strings != NULL)
    {
         result = find_matches_for_strings(    context->hash_table.non_hashed_strings, 
                                               buffer, 
                                               buffer_size, 
                                               current_offset, 
                                               flags, 
                                               negative_size);
    }
            	
	return result;
}




