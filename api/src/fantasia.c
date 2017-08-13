/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * mod_mime_magic: MIME type lookup via file magic numbers
 * Copyright (c) 1996-1997 Cisco Systems, Inc.
 *
 * This software was submitted by Cisco Systems to the Apache Software Foundation in July
 * 1997.  Future revisions and derivatives of this source code must
 * acknowledge Cisco Systems as the original contributor of this module.
 * All other licensing and usage conditions are those of the Apache Software Foundation.
 *
 * Some of this code is derived from the free version of the file command
 * originally posted to comp.sources.unix.  Copyright info for that program
 * is included below as required.
 * ---------------------------------------------------------------------------
 * - Copyright (c) Ian F. Darwin, 1987. Written by Ian F. Darwin.
 *
 * This software is not subject to any license of the American Telephone and
 * Telegraph Company or of the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on any
 * computer system, and to alter it and redistribute it freely, subject to
 * the following restrictions:
 *
 * 1. The author is not responsible for the consequences of use of this
 * software, no matter how awful, even if they arise from flaws in it.
 *
 * 2. The origin of this software must not be misrepresented, either by
 * explicit claim or by omission.  Since few users ever read sources, credits
 * must appear in the documentation.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.  Since few users ever read
 * sources, credits must appear in the documentation.
 *
 * 4. This notice may not be removed or altered.
 * -------------------------------------------------------------------------
 *
 * For compliance with Mr Darwin's terms: this has been very significantly
 * modified from the free "file" command.
 * - all-in-one file for compilation convenience when moving from one
 *   version of Apache to the next.
 * - Memory allocation is done through the Apache API's apr_pool_t structure.
 * - All functions have had necessary Apache API request or server
 *   structures passed to them where necessary to call other Apache API
 *   routines.  (i.e. usually for logging, files, or memory allocation in
 *   itself or a called function.)
 * - struct magic has been converted from an array to a single-ended linked
 *   list because it only grows one record at a time, it's only accessed
 *   sequentially, and the Apache API has no equivalent of realloc().
 * - Functions have been changed to get their parameters from the server
 *   configuration instead of globals.  (It should be reentrant now but has
 *   not been tested in a threaded environment.)
 * - Places where it used to print results to stdout now saves them in a
 *   list where they're used to set the MIME type in the Apache request
 *   record.
 * - Command-line flags have been removed since they will never be used here.
 *
 * Ian Kluft <ikluft@cisco.com>
 * Engineering Information Framework
 * Central Engineering
 * Cisco Systems, Inc.
 * San Jose, CA, USA
 *
 * Initial installation          July/August 1996
 * Misc bug fixes                May 1997
 * Submission to Apache Software Foundation    July 1997
 *
 *
 */
/*
 * Razorback(TM) Block Data Type Magic (fantaisa.c)
 * Copyright (c) 2011 Sourcefire, Inc.
 *
 * For compliance with Mr Darwin's terms: this has been very significantly
 * modified from the free "file" command.
 *
 * This code has been further modified from the mod_mime_magic code by the 
 * Razorback(TM) team:
 * - Compressed lookup of internal data type support has been removed
 * - File system identification code has been removed.
 * - Ascii-ness code is currently not included.
 * - All RSL code has been removed.
 * - Code integrates directly with the razorback API to set data type UUID's 
 *   on items in the block pool.
 *
 * October 2011
 * Tom Judge <tjudge@sourcefire.com>
 * Sourcefire Inc,
 * 9770 Patuxent Woods Drive
 * Columbia, MD 21046, 
 * United State
 */

#include "config.h"
#include "fantasia.h"
#include <razorback/uuids.h>
#include <razorback/log.h>
#include <razorback/block_pool.h>
#ifdef _MSC_VER
#include "safewindows.h"
#include <Shlwapi.h>
#include "bobins.h"
#else
#define MAGIC_FILE ETC_DIR "/magic"
#endif
#include <ctype.h>
#include <string.h>
#include <stdio.h>




#define HOWMANY  4096

#define MAXstring 64    /* max leng of "string" types */
#define    EATAB {while (isspace(*l))  ++l;}

struct magic {
    struct magic *next;     /* link to next entry */
    int lineno;             /* line number from magic file */

    short flag;
#define INDIR  1            /* if '>(...)' appears,  */
#define UNSIGNED 2          /* comparison is unsigned */
    short cont_level;       /* level of ">" */
    struct {
        char type;          /* byte short long */
        long offset;        /* offset from indirection */
    } in;
    long offset;            /* offset to magic number */
    unsigned char reln;     /* relation (0=eq, '>'=gt, etc) */
    char type;              /* int, short, long or string. */
    char vallen;            /* length of string value, if any */
#define BYTE      1
#define SHORT     2
#define LONG      4
#define STRING    5
#define DATE      6
#define BESHORT   7
#define BELONG    8
#define BEDATE    9
#define LESHORT  10
#define LELONG   11
#define LEDATE   12
    union VALUETYPE {
        unsigned char b;
        unsigned short h;
        unsigned long l;
        char s[MAXstring];
        unsigned char hs[2];   /* 2 bytes of a fixed-endian "short" */
        unsigned char hl[4];   /* 2 bytes of a fixed-endian "long" */
    } value;                   /* either number or string */
    unsigned long mask;        /* mask before comparison with value */
    char nospflag;             /* supress space character */

    /* NOTE: this string is suspected of overrunning - find it! */
    char *desc;        /* description */
};

struct magic_rsl {
    const char *str;                  /* string, possibly a fragment */
    struct magic_rsl_s *next;   /* pointer to next fragment */
};

struct magic *sg_magic = NULL;      /* head of magic config list */
struct magic *sg_last = NULL;

/* per-request info */
struct magic_request {
    struct magic_rsl *head;          /* result string list */
    struct magic_rsl *tail;
    unsigned suf_recursion;   /* recursion depth in suffix check */
};



static int Magic_parseLine(char *, int);
static int Magic_getvalue(struct magic *, char **);
static char * Magic_getstr(register char *s, register char *p, int plen, int *slen);
static int Magic_hextoint(int c);
static bool Magic_tryit(struct BlockPoolItem *, unsigned char *, size_t, int);
static int Magic_softmagic(struct BlockPoolItem*, unsigned char *buf, size_t nbytes);
static int Magic_match(struct BlockPoolItem*, unsigned char *s, size_t nbytes);
static int Magic_mget(union VALUETYPE *, unsigned char *,
                        struct magic *, size_t);
static int Magic_mcheck(union VALUETYPE *, struct magic *);
static void Magic_mprint(struct BlockPoolItem *, union VALUETYPE *, struct magic *);
static char * Magic_GetFile(void);




/*
 * extend the sign bit if the comparison is to be signed
 */
static unsigned long 
Magic_signextend(struct magic *m, unsigned long v)
{
    if (!(m->flag & UNSIGNED))
        switch (m->type) {
            /*
             * Do not remove the casts below.  They are vital. When later
             * compared with the data, the sign extension must have happened.
             */
        case BYTE:
            v = (char) v;
            break;
        case SHORT:
        case BESHORT:
        case LESHORT:
            v = (short) v;
            break;
        case DATE:
        case BEDATE:
        case LEDATE:
        case LONG:
        case BELONG:
        case LELONG:
            v = (long) v;
            break;
        case STRING:
            break;
        default:
            rzb_log(LOG_ERR, "%s: can't happen: m->type=%d",__func__,  m->type);
            return -1; //XXX: This might be a bug
        }
    return v;
}

/*
 * parse one line from magic file, put into magic[index++] if valid
 */
static int 
Magic_parseLine(char *l, int lineno)
{
    struct magic *m;
    char *t, *s;

    /* allocate magic structure entry */
    m = (struct magic *)calloc(1, sizeof(struct magic));

    /* append to linked list */
    m->next = NULL;
    if (!sg_magic || !sg_last) {
        sg_magic = sg_last = m;
    }
    else {
        sg_last->next = m;
        sg_last = m;
    }

    /* set values in magic structure */
    m->flag = 0;
    m->cont_level = 0;
    m->lineno = lineno;

    while (*l == '>') {
        ++l;  /* step over */
        m->cont_level++;
    }

    if (m->cont_level != 0 && *l == '(') {
        ++l;  /* step over */
        m->flag |= INDIR;
    }

    /* get offset, then skip over it */
    m->offset = (int) strtol(l, &t, 0);
    if (l == t) {
        rzb_log(LOG_ERR, "%s: Offset %s invalid line: %d", __func__, l, lineno); 
    }
    l = t;

    if (m->flag & INDIR) {
        m->in.type = LONG;
        m->in.offset = 0;
        /*
         * read [.lbs][+-]nnnnn)
         */
        if (*l == '.') {
            switch (*++l) {
            case 'l':
                m->in.type = LONG;
                break;
            case 's':
                m->in.type = SHORT;
                break;
            case 'b':
                m->in.type = BYTE;
                break;
            default:
                rzb_log(LOG_ERR, "%s: Indirect offset type %c invalid line: %d", __func__, *l, lineno);
                break;
            }
            l++;
        }

        s = l;
        if (*l == '+' || *l == '-')
            l++;
        if (isdigit((unsigned char) *l)) {
            m->in.offset = strtol(l, &t, 0);
            if (*s == '-')
                m->in.offset = -m->in.offset;
        }
        else
            t = l;
        if (*t++ != ')') {
            rzb_log(LOG_ERR, "%s: missing ')' in indirect offset line: %d", __func__, lineno);
        }
        l = t;
    }


    while (isdigit((unsigned char) *l))
        ++l;
    EATAB;

#define NBYTE           4
#define NSHORT          5
#define NLONG           4
#define NSTRING         6
#define NDATE           4
#define NBESHORT        7
#define NBELONG         6
#define NBEDATE         6
#define NLESHORT        7
#define NLELONG         6
#define NLEDATE         6

    if (*l == 'u') {
        ++l;
        m->flag |= UNSIGNED;
    }

    /* get type, skip it */
    if (strncmp(l, "byte", NBYTE) == 0) {
        m->type = BYTE;
        l += NBYTE;
    }
    else if (strncmp(l, "short", NSHORT) == 0) {
        m->type = SHORT;
        l += NSHORT;
    }
    else if (strncmp(l, "long", NLONG) == 0) {
        m->type = LONG;
        l += NLONG;
    }
    else if (strncmp(l, "string", NSTRING) == 0) {
        m->type = STRING;
        l += NSTRING;
    }
    else if (strncmp(l, "date", NDATE) == 0) {
        m->type = DATE;
        l += NDATE;
    }
    else if (strncmp(l, "beshort", NBESHORT) == 0) {
        m->type = BESHORT;
        l += NBESHORT;
    }
    else if (strncmp(l, "belong", NBELONG) == 0) {
        m->type = BELONG;
        l += NBELONG;
    }
    else if (strncmp(l, "bedate", NBEDATE) == 0) {
        m->type = BEDATE;
        l += NBEDATE;
    }
    else if (strncmp(l, "leshort", NLESHORT) == 0) {
        m->type = LESHORT;
        l += NLESHORT;
    }
    else if (strncmp(l, "lelong", NLELONG) == 0) {
        m->type = LELONG;
        l += NLELONG;
    }
    else if (strncmp(l, "ledate", NLEDATE) == 0) {
        m->type = LEDATE;
        l += NLEDATE;
    }
    else {
        rzb_log(LOG_ERR,  "%s: type %s invalid line: %d", __func__, l, lineno);
        return -1;
    }
    /* New-style anding: "0 byte&0x80 =0x80 dynamically linked" */
    if (*l == '&') {
        ++l;
        m->mask = Magic_signextend(m, strtol(l, &l, 0));
    }
    else
        m->mask = ~0L;
    EATAB;

    switch (*l) {
    case '>':
    case '<':
        /* Old-style anding: "0 byte &0x80 dynamically linked" */
    case '&':
    case '^':
    case '=':
        m->reln = *l;
        ++l;
        break;
    case '!':
        if (m->type != STRING) {
            m->reln = *l;
            ++l;
            break;
        }
        /* FALL THROUGH */
    default:
        if (*l == 'x' && isspace(l[1])) {
            m->reln = *l;
            ++l;
            goto GetDesc;  /* Bill The Cat */
        }
        m->reln = '=';
        break;
    }
    EATAB;

    if (Magic_getvalue(m, &l))
        return -1;
    /*
     * now get last part - the description
     */
  GetDesc:
    EATAB;
    if (l[0] == '\b') {
        ++l;
        m->nospflag = 1;
    }
    else if ((l[0] == '\\') && (l[1] == 'b')) {
        ++l;
        ++l;
        m->nospflag = 1;
    }
    else
        m->nospflag = 0;
    if (asprintf(&m->desc, "%s", l) == -1)
        return -1;

#if MIME_MAGIC_DEBUG
    rzb_log(LOG_DEBUG,
                "%s: parse line=%d m=%x next=%x cont=%d desc=%s",
                __func__,
                lineno, m, m->next, m->cont_level, m->desc);
#endif /* MIME_MAGIC_DEBUG */

    return 0;
}


/*
 * Read a numeric value from a pointer, into the value union of a magic
 * pointer, according to the magic type.  Update the string pointer to point
 * just after the number read.  Return 0 for success, non-zero for failure.
 */
static int 
Magic_getvalue(struct magic *m, char **p)
{
    int slen;

    if (m->type == STRING) {
        *p = Magic_getstr(*p, m->value.s, sizeof(m->value.s), &slen);
        m->vallen = slen;
    }
    else if (m->reln != 'x')
        m->value.l = Magic_signextend(m, strtol(*p, p, 0));
    return 0;
}


/*
 * Convert a string containing C character escapes.  Stop at an unescaped
 * space or tab. Copy the converted version to "p", returning its length in
 * *slen. Return updated scan pointer as function result.
 */
static char *
Magic_getstr(register char *s, register char *p,
                    int plen, int *slen)
{
    char *origs = s, *origp = p;
    char *pmax = p + plen - 1;
    register int c;
    register int val;

    while ((c = *s++) != '\0') {
        if (isspace(c))
            break;
        if (p >= pmax) {
            rzb_log(LOG_ERR, "%s: string too long: %s", __func__, origs);
            break;
        }
        if (c == '\\') {
            switch (c = *s++) {

            case '\0':
                goto out;

            default:
                *p++ = (char) c;
                break;

            case 'n':
                *p++ = '\n';
                break;

            case 'r':
                *p++ = '\r';
                break;

            case 'b':
                *p++ = '\b';
                break;

            case 't':
                *p++ = '\t';
                break;

            case 'f':
                *p++ = '\f';
                break;

            case 'v':
                *p++ = '\v';
                break;

                /* \ and up to 3 octal digits */
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                val = c - '0';
                c = *s++;  /* try for 2 */
                if (c >= '0' && c <= '7') {
                    val = (val << 3) | (c - '0');
                    c = *s++;  /* try for 3 */
                    if (c >= '0' && c <= '7')
                        val = (val << 3) | (c - '0');
                    else
                        --s;
                }
                else
                    --s;
                *p++ = (char) val;
                break;

                /* \x and up to 3 hex digits */
            case 'x':
                val = 'x';            /* Default if no digits */
                c = Magic_hextoint(*s++);   /* Get next char */
                if (c >= 0) {
                    val = c;
                    c = Magic_hextoint(*s++);
                    if (c >= 0) {
                        val = (val << 4) + c;
                        c = Magic_hextoint(*s++);
                        if (c >= 0) {
                            val = (val << 4) + c;
                        }
                        else
                            --s;
                    }
                    else
                        --s;
                }
                else
                    --s;
                *p++ = (char) val;
                break;
            }
        }
        else
            *p++ = (char) c;
    }
  out:
    *p = '\0';
    *slen = p - origp;
    return s;
}

/* Single hex char to int; -1 if not a hex char. */
static int 
Magic_hextoint(int c)
{
    if (isdigit(c))
        return c - '0';
    if ((c >= 'a') && (c <= 'f'))
        return c + 10 - 'a';
    if ((c >= 'A') && (c <= 'F'))
        return c + 10 - 'A';
    return -1;
}


/*
 * apprentice - load configuration from the magic file r
 *  API request record
 */
static int 
Magic_apprentice()
{
    FILE *f = NULL;
    char line[BUFSIZ + 1];
    int errs = 0;
    int lineno;
	char * file = NULL;
#if MIME_MAGIC_DEBUG
    int rule = 0;
    struct magic *m, *prevm;
#endif
	file = Magic_GetFile();
    if ((f = fopen(file, "r")) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: can't read magic file %s", __func__, file);
        return -1;
    }

    /* set up the magic list (empty) */
    sg_magic = sg_last = NULL;

    /* parse it */
    for (lineno = 1; fgets(line, BUFSIZ, f) != NULL; lineno++) {
        int ws_offset;
        char *last = line + strlen(line) - 1; /* guaranteed that len >= 1 since an
                                               * "empty" line contains a '\n'
                                               */

        /* delete newline and any other trailing whitespace */
        while (last >= line
               && isspace(*last)) {
            *last = '\0';
            --last;
        }

        /* skip leading whitespace */
        ws_offset = 0;
        while (line[ws_offset] && isspace(line[ws_offset])) {
            ws_offset++;
        }

        /* skip blank lines */
        if (line[ws_offset] == 0) {
            continue;
        }

        /* comment, do not parse */
        if (line[ws_offset] == '#')
            continue;

#if MIME_MAGIC_DEBUG
        /* if we get here, we're going to use it so count it */
        rule++;
#endif

        /* parse it */
        if (Magic_parseLine(line + ws_offset, lineno) != 0)
            ++errs;
    }

    fclose(f);

#if MIME_MAGIC_DEBUG
    rzb_log(LOG_DEBUG,
                "%s: apprentice m=%s m->next=%s last=%s",
                __func__,
                sg_magic ? "set" : "NULL",
                (sg_magic && sg_magic->next) ? "set" : "NULL",
                sg_last ? "set" : "NULL");
    rzb_log(LOG_DEBUG,
                "%s: apprentice read %d lines, %d rules, %d errors", __func__,
                lineno, rule, errs);
#endif

#if MIME_MAGIC_DEBUG
    prevm = 0;
    rzb_log(LOG_DEBUG,
                "%s: apprentice test", __func__);
    for (m = sg_magic; m; m = m->next) {
        if (isprint((((unsigned long) m) >> 24) & 255) &&
            isprint((((unsigned long) m) >> 16) & 255) &&
            isprint((((unsigned long) m) >> 8) & 255) &&
            isprint(((unsigned long) m) & 255)) {
            rzb_log(LOG_DEBUG,
                        "%s: apprentice: POINTER CLOBBERED! m=\"%c%c%c%c\" line=%d", __func__,
                        (((unsigned long) m) >> 24) & 255,
                        (((unsigned long) m) >> 16) & 255,
                        (((unsigned long) m) >> 8) & 255,
                        ((unsigned long) m) & 255,
                        prevm ? prevm->lineno : -1);
            break;
        }
        prevm = m;
    }
#endif

    return (errs ? -1 : 0);
}

bool
Magic_Init(void)
{
#if MIME_MAGIC_DEBUG
    struct magic *m, *prevm;
#endif /* MIME_MAGIC_DEBUG */

    if (Magic_apprentice() == -1)
        return false;

#if MIME_MAGIC_DEBUG
    prevm = 0;
    rzb_log(LOG_DEBUG,
                "%s: magic_init 1 test", __func__);
    for (m = sg_magic; m; m = m->next) {
        if (isprint((((unsigned long) m) >> 24) & 255) &&
            isprint((((unsigned long) m) >> 16) & 255) &&
            isprint((((unsigned long) m) >> 8) & 255) &&
            isprint(((unsigned long) m) & 255)) {
            rzb_log(LOG_DEBUG,
                        "%s: magic_init 1: POINTER CLOBBERED! m=\"%c%c%c%c\" line=%d", __func__,
                        (((unsigned long) m) >> 24) & 255,
                        (((unsigned long) m) >> 16) & 255,
                        (((unsigned long) m) >> 8) & 255,
                        ((unsigned long) m) & 255,
                        prevm ? prevm->lineno : -1);
            break;
        }
        prevm = m;
    }
#endif

    return true;
}


/*
 * magic_process - process input block pool item
 * (formerly called "process" in file command, prefix added for clarity) 
 * Coallesses data out of the data list into a buffer and runs magic on it.
 */
bool
Magic_process(struct BlockPoolItem *item)
{
    unsigned char buf[HOWMANY + 1];  /* one extra for terminating '\0' */
    size_t nbytes = 0;           /* number of bytes read from a datafile */
    size_t toCopy =0;
    struct BlockPoolData *dataItem = NULL;

    memset(buf,0, HOWMANY+1);
    /*
     * try looking at the first HOWMANY bytes
     */
    dataItem = item->pDataHead;
    while( nbytes < HOWMANY && dataItem != NULL)
    {
        toCopy = ( ( nbytes + dataItem->iLength ) > HOWMANY ? ( HOWMANY - nbytes ) : dataItem->iLength);
#if MIME_MAGIC_DEBUG
        rzb_log(LOG_DEBUG, "%s: Coallessing data buffers Have=%zu Getting=%zu Result=%zu", __func__, nbytes, toCopy, nbytes+toCopy);
#endif
        if (dataItem->iFlags == BLOCK_POOL_DATA_FLAG_FILE)
        {
            if (fread(buf,1,toCopy,dataItem->data.file) != toCopy)
                return false;
            rewind(dataItem->data.file);
        }
        else
            memcpy(buf, dataItem->data.pointer, toCopy);

        nbytes += toCopy;
    }

    buf[nbytes++] = '\0';  /* null-terminate it */
    if (!Magic_tryit(item, buf, nbytes, 1))
        return false;

    return true;
}


static bool
Magic_tryit(struct BlockPoolItem *item, unsigned char *buf, size_t nb, int checkzmagic)
{

    /*
     * try tests in PREFIX/etc/razorback/magic (or surrogate magic file)
     */
    if (Magic_softmagic(item, buf, nb) == 1)
        return true;

    /*
     * try known keywords, check for ascii-ness too.
     */
#if 0
    if (ascmagic(buf, nb) == 1)
        return true;
#endif
    /*
     * abandon hope, all ye who remain here
     */
    return false;
}

/*
 * softmagic - lookup one file in database (already read from /etc/magic by
 * apprentice.c). Passed the name and FILE * of one file to be typed.
 */
                /* ARGSUSED1 *//* nbytes passed for regularity, maybe need later */
static int 
Magic_softmagic(struct BlockPoolItem *item, unsigned char *buf, size_t nbytes)
{
    if (Magic_match(item, buf, nbytes))
        return 1;

    return 0;
}


/*
 * Go through the whole list, stopping if you find a match.  Process all the
 * continuations of that match before returning.
 *
 * We support multi-level continuations:
 *
 * At any time when processing a successful top-level match, there is a current
 * continuation level; it represents the level of the last successfully
 * matched continuation.
 *
 * Continuations above that level are skipped as, if we see one, it means that
 * the continuation that controls them - i.e, the lower-level continuation
 * preceding them - failed to match.
 *
 * Continuations below that level are processed as, if we see one, it means
 * we've finished processing or skipping higher-level continuations under the
 * control of a successful or unsuccessful lower-level continuation, and are
 * now seeing the next lower-level continuation and should process it.  The
 * current continuation level reverts to the level of the one we're seeing.
 *
 * Continuations at the current level are processed as, if we see one, there's
 * no lower-level continuation that may have failed.
 *
 * If a continuation matches, we bump the current continuation level so that
 * higher-level continuations are processed.
 */
static int 
Magic_match(struct BlockPoolItem *item, unsigned char *s, size_t nbytes)
{
#if MIME_MAGIC_DEBUG
    int rule_counter = 0;
#endif
    int cont_level = 0;
    int need_separator = 0;
    union VALUETYPE p;
    struct magic *m = NULL;

    for (m = sg_magic; m; m = m->next) {
#if MIME_MAGIC_DEBUG
        rule_counter++;
        rzb_log(LOG_DEBUG,
                    "%s: line=%d desc=%s", __func__, m->lineno, m->desc);
#endif
        memset(&p, 0, sizeof(union VALUETYPE));
        /* check if main entry matches */
        if (!Magic_mget(&p, s, m, nbytes) ||
            !Magic_mcheck(&p, m)) {
            struct magic *m_cont;

            /*
             * main entry didn't match, flush its continuations
             */
            if (!m->next || (m->next->cont_level == 0)) {
                continue;
            }

            m_cont = m->next;
            while (m_cont && (m_cont->cont_level != 0)) {
#if MIME_MAGIC_DEBUG
                rule_counter++;
                rzb_log(LOG_DEBUG,
                        "%s: line=%d mc=%x mc->next=%x cont=%d desc=%s", __func__,
                            m_cont->lineno, m_cont,
                            m_cont->next, m_cont->cont_level,
                            m_cont->desc);
#endif
                /*
                 * this trick allows us to keep *m in sync when the continue
                 * advances the pointer
                 */
                m = m_cont;
                m_cont = m_cont->next;
            }
            continue;
        }

        /* if we get here, the main entry rule was a match */
        /* this will be the last run through the loop */
#if MIME_MAGIC_DEBUG
        rzb_log(LOG_DEBUG,
                    "%s: rule matched, line=%d type=%d %s", __func__,
                    m->lineno, m->type,
                    (m->type == STRING) ? m->value.s : "");
#endif

        /* print the match */
        Magic_mprint(item, &p, m);

        /*
         * If we printed something, we'll need to print a blank before we
         * print something else.
         */
        if (m->desc[0])
            need_separator = 1;
        /* and any continuations that match */
        cont_level++;
        /*
         * while (m && m->next && m->next->cont_level != 0 && ( m = m->next
         * ))
         */
        m = m->next;
        while (m && (m->cont_level != 0)) {
#if MIME_MAGIC_DEBUG
            rzb_log(LOG_DEBUG,
                        "%s: match line=%d cont=%d type=%d %s", __func__,
                        m->lineno, m->cont_level, m->type,
                        (m->type == STRING) ? m->value.s : "");
#endif
            if (cont_level >= m->cont_level) {
                if (cont_level > m->cont_level) {
                    /*
                     * We're at the end of the level "cont_level"
                     * continuations.
                     */
                    cont_level = m->cont_level;
                }
                if (Magic_mget(&p, s, m, nbytes) &&
                    Magic_mcheck(&p, m)) {
                    /*
                     * This continuation matched. Print its message, with a
                     * blank before it if the previous item printed and this
                     * item isn't empty.
                     */
                    /* space if previous printed */
                    if (need_separator
                        && (m->nospflag == 0)
                        && (m->desc[0] != '\0')
                        ) {
                        //XXX:
                        //(void) magic_rsl_putchar(r, ' ');
                        need_separator = 0;
                    }
                    Magic_mprint(item, &p, m);
                    if (m->desc[0])
                        need_separator = 1;

                    /*
                     * If we see any continuations at a higher level, process
                     * them.
                     */
                    cont_level++;
                }
            }

            /* move to next continuation record */
            m = m->next;
        }
#if MIME_MAGIC_DEBUG
        rzb_log(LOG_DEBUG,
                    "%s: matched after %d rules", __func__, rule_counter);
#endif
        return 1;  /* all through */
    }
#if MIME_MAGIC_DEBUG
    rzb_log(LOG_DEBUG,
                "%s: failed after %d rules", __func__, rule_counter);
#endif
    return 0;  /* no match at all */
}
/*
 * Convert the byte order of the data we are looking at
 */
static int 
Magic_mconvert(union VALUETYPE *p, struct magic *m)
{
    char *rt;

    switch (m->type) {
    case BYTE:
    case SHORT:
    case LONG:
    case DATE:
        return 1;
    case STRING:
        /* Null terminate and eat the return */
        p->s[sizeof(p->s) - 1] = '\0';
        if ((rt = strchr(p->s, '\n')) != NULL)
            *rt = '\0';
        return 1;
    case BESHORT:
        p->h = (short) ((p->hs[0] << 8) | (p->hs[1]));
        return 1;
    case BELONG:
    case BEDATE:
        p->l = (long)
            ((p->hl[0] << 24) | (p->hl[1] << 16) | (p->hl[2] << 8) | (p->hl[3]));
        return 1;
    case LESHORT:
        p->h = (short) ((p->hs[1] << 8) | (p->hs[0]));
        return 1;
    case LELONG:
    case LEDATE:
        p->l = (long)
            ((p->hl[3] << 24) | (p->hl[2] << 16) | (p->hl[1] << 8) | (p->hl[0]));
        return 1;
    default:
        rzb_log(LOG_ERR,
                    "%s: invalid type %d in mconvert().", __func__, m->type);
        return 0;
    }
}

static int 
Magic_mget(union VALUETYPE *p, unsigned char *s,
                struct magic *m, size_t nbytes)
{
    long offset = m->offset;

    if (offset + sizeof(union VALUETYPE) > nbytes)
                  return 0;

    memcpy(p, s + offset, sizeof(union VALUETYPE));

    if (!Magic_mconvert(p, m))
        return 0;

    if (m->flag & INDIR) {

        switch (m->in.type) {
        case BYTE:
            offset = p->b + m->in.offset;
            break;
        case SHORT:
            offset = p->h + m->in.offset;
            break;
        case LONG:
            offset = p->l + m->in.offset;
            break;
        }

        if (offset + sizeof(union VALUETYPE) > nbytes)
                      return 0;

        memcpy(p, s + offset, sizeof(union VALUETYPE));

        if (!Magic_mconvert(p, m))
            return 0;
    }
    return 1;
}

static int Magic_mcheck(union VALUETYPE *p, struct magic *m)
{
    register unsigned long l = m->value.l;
    register unsigned long v;
    int matched;

    if ((m->value.s[0] == 'x') && (m->value.s[1] == '\0')) {
        rzb_log(LOG_ERR,
                    "%s: BOINK", __func__);
        return 1;
    }

    switch (m->type) {
    case BYTE:
        v = p->b;
        break;

    case SHORT:
    case BESHORT:
    case LESHORT:
        v = p->h;
        break;

    case LONG:
    case BELONG:
    case LELONG:
    case DATE:
    case BEDATE:
    case LEDATE:
        v = p->l;
        break;

    case STRING:
        l = 0;
        /*
         * What we want here is: v = strncmp(m->value.s, p->s, m->vallen);
         * but ignoring any nulls.  bcmp doesn't give -/+/0 and isn't
         * universally available anyway.
         */
        v = 0;
        {
            register unsigned char *a = (unsigned char *) m->value.s;
            register unsigned char *b = (unsigned char *) p->s;
            register int len = m->vallen;

            while (--len >= 0)
                if ((v = *b++ - *a++) != 0)
                    break;
        }
        break;
    default:
        /*  bogosity, pretend that it just wasn't a match */
        rzb_log(LOG_ERR,
                    "%s: invalid type %d in mcheck().", __func__, m->type);
        return 0;
    }

    v = Magic_signextend(m, v) & m->mask;

    switch (m->reln) {
    case 'x':
#if MIME_MAGIC_DEBUG
        rzb_log(LOG_DEBUG,
                    "%s: %lu == *any* = 1", __func__, v);
#endif
        matched = 1;
        break;

    case '!':
        matched = v != l;
#if MIME_MAGIC_DEBUG
        rzb_log(LOG_DEBUG,
                    "%s: %lu != %lu = %d", __func__, v, l, matched);
#endif
        break;

    case '=':
        matched = v == l;
#if MIME_MAGIC_DEBUG
        rzb_log(LOG_DEBUG, 
                    "%lu == %lu = %d", __func__, v, l, matched);
#endif
        break;

    case '>':
        if (m->flag & UNSIGNED) {
            matched = v > l;
#if MIME_MAGIC_DEBUG
            rzb_log(LOG_DEBUG,
                    "%s: %lu > %lu = %d", __func__, v, l, matched);
#endif
        }
        else {
            matched = (long) v > (long) l;
#if MIME_MAGIC_DEBUG
            rzb_log(LOG_DEBUG,
                    "%s: %ld > %ld = %d", __func__, v, l, matched);
#endif
        }
        break;

    case '<':
        if (m->flag & UNSIGNED) {
            matched = v < l;
#if MIME_MAGIC_DEBUG
            rzb_log(LOG_DEBUG,
                    "%s: %lu < %lu = %d", __func__, v, l, matched);
#endif
        }
        else {
            matched = (long) v < (long) l;
#if MIME_MAGIC_DEBUG
            rzb_log(LOG_DEBUG,
                    "%s: %ld < %ld = %d", __func__, v, l, matched);
#endif
        }
        break;

    case '&':
        matched = (v & l) == l;
#if MIME_MAGIC_DEBUG
        rzb_log(LOG_DEBUG,
                    "%s: ((%lx & %lx) == %lx) = %d", __func__, v, l, l, matched);
#endif
        break;

    case '^':
        matched = (v & l) != l;
#if MIME_MAGIC_DEBUG
        rzb_log(LOG_DEBUG, 
                    "%s: ((%lx & %lx) != %lx) = %d", __func__, v, l, l, matched);
#endif
        break;

    default:
        /* bogosity, pretend it didn't match */
        matched = 0;
        rzb_log(LOG_ERR,
                    "%s: can't happen: invalid relation %d.", __func__,
                    m->reln);
        break;
    }

    return matched;
}

static void 
Magic_mprint(struct BlockPoolItem *item,  union VALUETYPE *p, struct magic *m)
{
    //char *pp;
    unsigned long v;
    char time_str[26]; // Magic number from opengroup -> http://pubs.opengroup.org/onlinepubs/9699919799/functions/ctime.html Why is this not defined in time.h?

    switch (m->type) {
    case BYTE:
        v = p->b;
        break;

    case SHORT:
    case BESHORT:
    case LESHORT:
        v = p->h;
        break;

    case LONG:
    case BELONG:
    case LELONG:
        v = p->l;
        break;

    case STRING:
        if (m->reln == '=') {
           //XXX: FIX 
       //     (void) magic_rsl_printf(r, m->desc, m->value.s);
            //rzb_log(LOG_DEBUG, m->desc, m->value.s);
            UUID_Get_UUID(m->desc, UUID_TYPE_DATA_TYPE,item->pEvent->pBlock->pId->uuidDataType);
        }
        else {
            //XXX:Fix
            //(void) magic_rsl_printf(r, m->desc, p->s);
            //rzb_log(LOG_DEBUG, m->desc, p->s);
            UUID_Get_UUID(m->desc, UUID_TYPE_DATA_TYPE,item->pEvent->pBlock->pId->uuidDataType);
            
        }
        return;

    case DATE:
    case BEDATE:
    case LEDATE:
        ctime_r(((time_t *)&p->l), time_str);
        //pp = time_str;
        //XXX:FIX
        //(void) magic_rsl_printf(r, m->desc, pp);
        //rzb_log(LOG_DEBUG, m->desc, pp);
        UUID_Get_UUID(m->desc, UUID_TYPE_DATA_TYPE,item->pEvent->pBlock->pId->uuidDataType);
        return;
    default:
        rzb_log(LOG_ERR,
                    "%s: invalid m->type (%d)", __func__,
                    m->type);
        return;
    }

    v = Magic_signextend(m, v) & m->mask;
    //XXX:Fix
    //(void) magic_rsl_printf(r, m->desc,(unsigned long) v); 
    //rzb_log(LOG_DEBUG, m->desc, (unsigned long) v);
    UUID_Get_UUID(m->desc, UUID_TYPE_DATA_TYPE,item->pEvent->pBlock->pId->uuidDataType);
}
#ifdef _MSC_VER
// Worlds largest kludge
#undef LONG
static char *
Magic_GetFile(void)
{
	char *path;
	HKEY hkey = HKEY_LOCAL_MACHINE;
	HKEY razorback;
	LONG lRet;
	DWORD type, RegValueLen;

	lRet = RegOpenKeyA(
		hkey,
		"SOFTWARE\\Razorback",
		&razorback);
	
	if(lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed because registry key does not exist. SOFTWARE", __func__);
		return false;
	}
	if ((path = calloc(MAX_PATH, sizeof(char)))== NULL)
		return NULL;

	lRet = RegQueryValueExA(
		razorback, 
		"Path",
		NULL, 
		&type, 
		NULL, 
		&RegValueLen);

	if (lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed to query registry value length", __func__);
		return false;
	}
	if (RegValueLen > MAX_PATH -7)
	{
		rzb_log(LOG_ERR, "%s: Key to large", __func__);
		return false;
	}

	lRet = RegQueryValueExA(
		razorback, 
		"Path",
		NULL, 
		&type, 
		(LPBYTE)path, 
		&RegValueLen);

	if (lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed to query registry value", __func__);
		return false;
	}

	if (type != REG_SZ) {
		rzb_log(LOG_ERR, "%s: Failed because registry key is not the right type", __func__);
		return false;
	}

	PathAppendA(path, "magic");
	return path;
}
#else //_MSC_VER
static char * Magic_GetFile(void)
{
	return (char *)MAGIC_FILE;
}
#endif
