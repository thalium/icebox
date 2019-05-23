/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_spu.h"
#include "cr_error.h"
#include "cr_string.h"
#include <stdio.h>

/**
 * \mainpage spu_loader 
 *
 * \section Spu_loaderIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The spu_loader module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 */
void crSPUInitDispatchTable( SPUDispatchTable *table )
{
    table->copyList = NULL;
    table->copy_of = NULL;
    table->mark = 0;
    table->server = NULL;
}

#if 0 /* unused */

static int validate_int( const char *response, 
             const char *min,
             const char *max )
{
    int i, imin, imax;   
    if (sscanf(response, "%d", &i) != 1)
        return 0;
    if (min && sscanf(min, "%d", &imin) == 1 && imin > i)
        return 0;
    if (max && sscanf(max, "%d", &imax) == 1 && imax < i)
        return 0;
    return 1;
}

static int validate_float( const char *response, 
             const char *min,
             const char *max )
{
    float f, fmin, fmax;
    if (sscanf(response, "%f", &f) != 1)
        return 0;
    if (min && sscanf(min, "%f", &fmin) == 1 && fmin > f)
        return 0;
    if (max && sscanf(max, "%f", &fmax) == 1 && fmax < f)
        return 0;
    return 1;
}

static int validate_one_option( const SPUOptions *opt, 
                const char *response,
                const char *min,
                const char *max )
{
    switch (opt->type) {
    case CR_BOOL:
        return validate_int( response, "0", "1" );
    case CR_INT:
        return validate_int( response, min, max );
    case CR_FLOAT:
        return validate_float( response, min, max );
    case CR_ENUM:
        /* Make sure response string is present in the min string.
         * For enums, the min string is a comma-separated list of valid values.
         */
        CRASSERT(opt->numValues == 1); /* an enum limitation for now */
        {
            const char *p = crStrstr(min, response);
            if (!p)
                return 0;  /* invalid value! */
            if (p[-1] != '\'')
                return 0;  /* right substring */
            if (p[crStrlen(response)] != '\'')
                return 0;  /* left substring */
            return 1;
        }
    default:
        return 0;
    }
}


/**
 * Make sure the response matches the opt's parameters (right number
 * and type of values, etc.)
 * Return 1 if OK, 0 if error.
 */
static int validate_option( const SPUOptions *opt, const char *response )
{
    const char *min = opt->min;
    const char *max = opt->max;
    int i = 0;
    int retval;

    if (opt->type == CR_STRING)
        return 1;
   
    CRASSERT(opt->numValues > 0);

    /* skip leading [ for multi-value options */
    if (opt->numValues > 1) {
        /* multi-valued options must be enclosed in brackets */
        if (*response != '[')
            return 0;
        response++; /* skip [ */
        /* make sure min and max are bracketed as well */
        if (min) {
            CRASSERT(*min == '[');  /* error in <foo>spu_config.c code!!! */
            min++;
        }
        if (max) {
            CRASSERT(*max == '[');  /* error in <foo>spu_config.c code!!! */
            max++;
        }
    }

    for (;;)
    {
        if (!validate_one_option( opt, response, min, max ))
        {
            retval = 0;
            break;
        }
        if (++i == opt->numValues)
        {
            retval = 1; /* all done! */
            break;
        }
        /* advance pointers to next item */
        if (min)
        {
            while (*min != ' ' && *min)
                min++;
            while (*min == ' ')
                min++;
        }
        if (max)
        {
            while (*max != ' ' && *max)
                max++;
            while (*max == ' ')
                max++;
        }
        if (response)
        {
            while (*response != ' ' && *response)
                response++;
            while (*response == ' ')
                response++;
        }
    }

    return retval;
}

#endif /* unused */

/** Use the default values for all the options:
 */
void crSPUSetDefaultParams( void *spu, SPUOptions *options )
{
    int i;
    
    for (i = 0 ; options[i].option ; i++)
    {
        SPUOptions *opt = &options[i];
        opt->cb( spu, opt->deflt );
    }
}


/**
 * Find the index of the given enum value in the SPUOption's list of
 * possible enum values.
 * Return the enum index, or -1 if not found.
 */
int crSPUGetEnumIndex( const SPUOptions *options, const char *optName, const char *value )
{
    const SPUOptions *opt;
    const int valueLen = crStrlen(value);

    /* first, find the right option */
    for (opt = options; opt->option; opt++) {
        if (crStrcmp(opt->option, optName) == 0) {
            char **values;
            int i;

            CRASSERT(opt->type == CR_ENUM);

            /* break into array of strings */
            /* min string should be of form "'enum1', 'enum2', 'enum3', etc" */
            values = crStrSplit(opt->min, ",");

            /* search the array */
            for (i = 0; values[i]; i++) {
                /* find leading quote */
                const char *e = crStrchr(values[i], '\'');
                CRASSERT(e);
                if (e) {
                    /* test for match */
                    if (crStrncmp(value, e + 1, valueLen) == 0 &&   e[valueLen + 1] == '\'') {
                        crFreeStrings(values);
                        return i;
                    }
                }
            }

            /* enum value not found! */
            crFreeStrings(values);
            return -1;
        }
    }

    return -1;
}
