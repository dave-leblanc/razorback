/*

Copyright(c) 2011, Google, Inc. [mjwiacek@google.com].

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "regex.h"
#include <string.h>
#include <re2/re2.h>
#include <re2/stringpiece.h>
#include "yara.h"

int regex_exec(REGEXP* regex, const char *buffer, size_t buffer_size) 
{
    if (!regex || buffer_size == 0)
        return 0;

    re2::StringPiece data(buffer, buffer_size);
    re2::StringPiece substring;
    re2::RE2::Anchor anchor = re2::RE2::UNANCHORED;

    if (regex->re2_anchored)
        anchor = re2::RE2::ANCHOR_START;

    re2::RE2* re_ptr = (re2::RE2*) regex->regexp;

    if (re_ptr->Match(data, 0, data.size()-1, anchor, &substring, 1)) 
    {
        return substring.size();
    }
    
    return -1;
}


void regex_free(REGEXP* regex) 
{
    if (!regex)
        return;

    if (regex->regexp) 
    {
        delete (re2::RE2*) regex->regexp;
        regex->regexp = NULL;
    }
}


int regex_compile(REGEXP* output,
                  const char* pattern,
                  int anchored,
                  int case_insensitive,
                  char* error_message,
                  size_t error_message_size,
                  int* error_offset) 
{
    if (!output || !pattern)
        return 0;

    memset(output, '\0', sizeof(REGEXP));

    RE2::Options options;
    options.set_log_errors(false);
    options.set_encoding(RE2::Options::EncodingLatin1);

    if (case_insensitive)
        options.set_case_sensitive(false);
    
    if (anchored)
        output->re2_anchored = anchored;

    re2::StringPiece string_piece_pattern(pattern);
    output->regexp = (void *)new RE2(string_piece_pattern, options);
    
    if (output->regexp == NULL) 
    {
        // TODO: Handle fatal error here, consistently with how yara would.
        return 0;
    }

    re2::RE2* re_ptr = (re2::RE2*)output->regexp;
    
    if (!re_ptr->ok()) 
    {
        if (error_message && error_message_size)
        {
            strncpy(error_message, re_ptr->error().c_str(), error_message_size - 1);
            error_message[error_message_size - 1] = '\0';
        }
        
        *error_offset = re_ptr->error().find(pattern);
        delete re_ptr;
        output->regexp = NULL;
        return 0;
    }
    
    return 1;
}
