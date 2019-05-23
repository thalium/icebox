/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_string.h"
#include "cr_error.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int crStrlen( const char *str )
{
	const char *temp;
	if (!str) return 0;
	for (temp = str ; *temp ; temp++);
	return temp-str;
}

char *crStrdup( const char *str )
{
	int len;
	char *ret;
	
	/* Allow strdup'ing of NULL strings -- this makes the __fillin functions 
	 * much cleaner. */
	
	if (str == NULL) return NULL;
	
	len = crStrlen(str);
	ret = (char*)crAlloc( len+1 );
	crMemcpy( ret, str, len );
	ret[len] = '\0';
	return ret;
}

char *crStrndup( const char *str, unsigned int len )
{
	char *ret = (char*)crAlloc( len+1 );
	crMemcpy( ret, str, len );
	ret[len] = '\0';
	return ret;
}

int crStrcmp( const char *str1, const char *str2 )
{
	while (*str1 && *str2)
	{
		if (*str1 != *str2)
		{
			break;
		}
		str1++; str2++;
	}
	return (*str1 - *str2);
}

int crStrncmp( const char *str1, const char *str2, int n )
{
	int i = 0;
	while (*str1 && *str2 && i < n)
	{
		if (*str1 != *str2)
		{
			break;
		}
		str1++; str2++; i++;
	}
	if (i == n) return 0;
	return (*str1 - *str2);
}

static char lowercase[256] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007', 
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017', 
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027', 
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037', 
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047', 
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057', 
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067', 
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077', 
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147', 
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157', 
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167', 
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137', 
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147', 
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157', 
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167', 
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177', 
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207', 
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217', 
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227', 
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237', 
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247', 
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257', 
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267', 
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277', 
	'\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307', 
	'\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317', 
	'\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327', 
	'\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337', 
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347', 
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357', 
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367', 
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377' 
};

int crStrcasecmp( const char *str1, const char *str2 )
{
	while (*str1 && *str2)
	{
		if (lowercase[(int) *str1] != lowercase[(int) *str2])
		{
			break;
		}
		str1++; str2++;
	}
	return (lowercase[(int) *str1] - lowercase[(int) *str2]);
}

void crStrcpy( char *dest, const char *src )
{
	while ((*dest++ = *src++))
		;
}

void crStrncpy( char *dest, const char *src, unsigned int len )
{
	const unsigned int str_len = crStrlen(src);
	if (str_len > len - 1) {
		crMemcpy( dest, src, len );  /* NOTE: not null-terminated! */
	}
	else {
		crMemcpy( dest, src, str_len + 1 );  /* includes null terminator */
	}
}

void crStrcat( char *dest, const char *src )
{
	crStrcpy( dest + crStrlen(dest), src );
}

char *crStrjoin( const char *str1, const char *str2 )
{
	const int len1 = crStrlen(str1), len2 = crStrlen(str2);
	char *s = crAlloc(len1 + len2 + 1);
	if (s)
	{
		crMemcpy( s, str1, len1 );
		crMemcpy( s + len1, str2, len2 );
		s[len1 + len2] = '\0';
	}
	return s;
}

char *crStrjoin3( const char *str1, const char *str2, const char *str3 )
{
	const int len1 = crStrlen(str1), len2 = crStrlen(str2), len3 = crStrlen(str3);
	char *s = crAlloc(len1 + len2 + len3 + 1);
	if (s)
	{
		crMemcpy( s, str1, len1 );
		crMemcpy( s + len1, str2, len2 );
		crMemcpy( s + len1 + len2, str3, len3 );
		s[len1 + len2 + len3] = '\0';
	}
	return s;
}

char *crStrstr( const char *str, const char *pat )
{
	int pat_len = crStrlen( pat );
	const char *end = str + crStrlen(str) - pat_len;
	char first_char = *pat;
	if (!str) return NULL;
	for (; str <= end ; str++)
	{
		if (*str == first_char && !crMemcmp( str, pat, pat_len ))
			return (char *) str;
	}
	return NULL;
}

char *crStrchr( const char *str, char c )
{
	for ( ; *str ; str++ )
	{
		if (*str == c)
			return (char *) str;
	}
	return NULL;
}

char *crStrrchr( const char *str, char c )
{
	const char *temp = str + crStrlen( str );
	for ( ; temp >= str ; temp-- )
	{
		if (*temp == c)
			return (char *) temp;
	}
	return NULL;
}

/* These functions are from the old wiregl net.c -- hexdumps?  Not sure quite yet. */
	
void crBytesToString( char *string, int nstring, void *data, int ndata )
{
	int i, offset;
	unsigned char *udata;

	offset = 0;
	udata = (unsigned char *) data;
	for ( i = 0; i < ndata && ( offset + 4 <= nstring ); i++ ) 
	{
		offset += sprintf( string + offset, "%02x ", udata[i] );
	}

	if ( i == ndata && offset > 0 )
		string[offset-1] = '\0';
	else
		crStrcpy( string + offset - 3, "..." );
}

void crWordsToString( char *string, int nstring, void *data, int ndata )
{
	int i, offset, ellipsis;
	unsigned int *udata;

	/* turn byte count into word count */
	ndata /= 4;

	/* we need an ellipsis if all the words won't fit in the string */
	ellipsis = ( ndata * 9 > nstring );
	if ( ellipsis )
	{
		ndata = nstring / 9;

		/* if the ellipsis won't fit then print one less word */
		if ( ndata * 9 + 3 > nstring )
			ndata--;
	}
		
	offset = 0;
	udata = (unsigned int *) data;
	for ( i = 0; i < ndata; i++ ) 
	{
		offset += sprintf( string + offset, "%08x ", udata[i] );
	}

	if ( ellipsis )
		crStrcpy( string + offset, "..." );
	else if ( offset > 0 )
		string[offset-1] = 0;
}

int crStrToInt( const char *str )
{
	if (!str) return 0;

	return atoi(str);
}

float crStrToFloat( const char *str )
{
	if (!str) return 0.0f;

	return (float) atof(str);
}

static int __numOccurrences( const char *str, const char *substr )
{
	int ret = 0;
	char *temp = (char *) str;
	while ((temp = crStrstr( temp, substr )) != NULL )
	{
		temp += crStrlen(substr);
		ret++;
	}
	return ret;
}

/**
 * Split str into a NULL-terminated array of strings, using splitstr as
 * the separator.
 * It's the same as the Python call string.split(str, splitstr).
 * Note: crStrSplit("a b  c", " ") returns ["a", "b", "", "c", NULL] though!!!
 */
char **crStrSplit( const char *str, const char *splitstr )
{
	char  *temp = (char *) str;
	int num_args = __numOccurrences( str, splitstr ) + 1;
	char **faked_argv = (char **) crAlloc( (num_args + 1)*sizeof( *faked_argv ) );
	int i;

	for (i = 0 ; i < num_args ; i++)
	{
		char *end;
		end = crStrstr( temp, splitstr );
		if (!end)
			end = temp + crStrlen( temp );
		faked_argv[i] = crStrndup( temp, end-temp );
		temp = end + crStrlen(splitstr);
	}
	faked_argv[num_args] = NULL;
	return faked_argv;
}

char **crStrSplitn( const char *str, const char *splitstr, int n )
{
	char **faked_argv;
	int i;
	char *temp = (char *) str;
	int num_args = __numOccurrences( str, splitstr );

	if (num_args > n)
		num_args = n;
	num_args++;

	faked_argv = (char **) crAlloc( (num_args + 1) * sizeof( *faked_argv ) );
	for (i = 0 ; i < num_args ; i++)
	{
		char *end;
		end = crStrstr( temp, splitstr );
		if (!end || i == num_args - 1 )
			end = temp + crStrlen( temp );
		faked_argv[i] = crStrndup( temp, end-temp );
		temp = end + crStrlen(splitstr);
	}
	faked_argv[num_args] = NULL;
	return faked_argv;
}

/* Free an array of strings, as returned by crStrSplit() and crStrSplitn(). */
void crFreeStrings( char **strings )
{
	int i;
	for (i = 0; strings[i]; i++) {
		crFree(strings[i]);
	}
	crFree(strings);
}


/* Intersect two strings on a word-by-word basis (separated by spaces).
 * We typically use this to intersect OpenGL extension strings.
 * Example: if s1 = "apple banana plum pear"
 *         and s2 = "plum banana orange"
 *      then return "banana plum" (or "plum banana").
 */
char *crStrIntersect( const char *s1, const char *s2 )
{
	int len1, len2;
	int resultLen;
	char *result;
	char **exten1, **exten2;
	int i, j;

	if (!s1 || !s2) {
		/* null strings, no intersection */
		return NULL;
	}

	len1 = crStrlen(s1);
	len2 = crStrlen(s2);

	/* allocate storage for result (a conservative estimate) */
	resultLen = ((len1 > len2) ? len1 : len2) + 2;
	result = (char *) crAlloc(resultLen);
	if (!result)
	{
		return NULL;
	}
	result[0] = 0;

	/* split s1 and s2 at space chars */
	exten1 = crStrSplit(s1, " ");
	exten2 = crStrSplit(s2, " ");

	for (i = 0; exten1[i]; i++)
	{
		for (j = 0; exten2[j]; j++)
		{
			if (crStrcmp(exten1[i], exten2[j]) == 0)
			{
				/* found an intersection, append to result */
				crStrcat(result, exten1[i]);
				crStrcat(result, " ");
				break;
			}
		}
	}

	/* free split strings */
	crFreeStrings( exten1 );
	crFreeStrings( exten2 );

	/*CRASSERT(crStrlen(result) < resultLen);*/

	/* all done! */
	return result;
}


int crIsDigit(char c)
{
  return c >= '0' && c <= '9';
}


static int crStrParseGlSubver(const char * ver, const char ** pNext, bool bSpacePrefixAllowed)
{
    const char * initVer = ver;
    int val = 0;

    for(;;++ver)
    {
        if(*ver >= '0' && *ver <= '9')
        {
            if(!val)
            {
                if(*ver == '0')
                    continue;
            }
            else
            {
                val *= 10;
            }
            val += *ver - '0';
        }
        else if(*ver == '.')
        {
            *pNext = ver+1;
            break;
        }
        else if(*ver == '\0')
        {
            *pNext = NULL;
            break;
        }
        else if(*ver == ' ' || *ver == '\t' ||  *ver == 0x0d || *ver == 0x0a)
        {
            if(bSpacePrefixAllowed)
            {
                if(!val)
                {
                    continue;
                }
            }

            /* treat this as the end of version string */
            *pNext = NULL;
            break;
        }
        else
        {
            crWarning("error parsing version %s", initVer);
            val = -1;
            break;
        }
    }

    return val;
}

int crStrParseGlVersion(const char * ver)
{
    const char * initVer = ver;
    int tmp;
    int iVer = crStrParseGlSubver(ver, &ver, true);
    if(iVer <= 0)
    {
        crWarning("parsing major version returned %d, '%s'", iVer, initVer);
        return iVer;
    }

    if (iVer > CR_GLVERSION_MAX_MAJOR)
    {
        crWarning("major version %d is bigger than the max supported %#x, this is somewhat not expected, failing", iVer, CR_GLVERSION_MAX_MAJOR);
        return -1;
    }

    iVer <<= CR_GLVERSION_OFFSET_MAJOR;
    if(!ver)
    {
        crDebug("no minor version supplied");
        goto done;
    }

    tmp = crStrParseGlSubver(ver, &ver, false);
    if (tmp < 0)
    {
        crWarning("parsing minor version failed, '%s'", initVer);
        return -1;
    }

    if (tmp > CR_GLVERSION_MAX_MINOR)
    {
        crWarning("minor version %d is bigger than the max supported %#x, this is somewhat not expected, failing", iVer, CR_GLVERSION_MAX_MAJOR);
        return -1;
    }

    iVer |= tmp << CR_GLVERSION_OFFSET_MINOR;

    if (!ver)
    {
        crDebug("no build version supplied");
        goto done;
    }

    tmp = crStrParseGlSubver(ver, &ver, false);
    if (tmp < 0)
    {
        crWarning("parsing build version failed, '%s', using 0", initVer);
        tmp = 0;
    }

    if (tmp > CR_GLVERSION_MAX_BUILD)
    {
        crWarning("build version %d is bigger than the max supported, using max supported val %#x", tmp, CR_GLVERSION_MAX_BUILD);
        tmp = CR_GLVERSION_MAX_MAJOR;
    }

    iVer |= tmp << CR_GLVERSION_OFFSET_BUILD;

done:
    crDebug("returning version %#x for string '%s'", iVer, initVer);

    return iVer;
}

int32_t crStrParseI32(const char *pszStr, const int32_t defaultVal)
{
    int32_t result = 0;
    bool neg = false;
    unsigned char iDigit = 0;
    if (!pszStr || pszStr[0] == '\0')
        return defaultVal;

    for (;;)
    {
        if (pszStr[0] == '\0')
            return defaultVal;

        if (pszStr[0] == ' ' || pszStr[0] == '\t' || pszStr[0] == '\n')
        {
            ++pszStr;
            continue;
        }

        if (pszStr[0] == '-')
        {
            if (neg)
                return defaultVal;

            neg = true;
            ++pszStr;
            continue;
        }

        break;
    }

    for (;;)
    {
        unsigned char digit;
        if (pszStr[0] == '\0')
        {
            if (!iDigit)
                return defaultVal;
            break;
        }

        digit = pszStr[0] - '0';
        if (digit > 9)
            return defaultVal;

        result *= 10;
        result += digit;
        ++iDigit;

        ++pszStr;
    }

    return !neg ? result : -result;
}
