/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STRING_H
#define CR_STRING_H

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

DECLEXPORT(char *)  crStrdup( const char *str );
DECLEXPORT(char *)  crStrndup( const char *str, unsigned int len );
DECLEXPORT(int)     crStrlen( const char *str );
DECLEXPORT(int)     crStrcmp( const char *str1, const char *str2 );
DECLEXPORT(int)     crStrncmp( const char *str1, const char *str2, int n );
DECLEXPORT(int)     crStrcasecmp( const char *str1, const char *str2 );
DECLEXPORT(void)    crStrcpy( char *dst, const char *src );
DECLEXPORT(void)    crStrncpy( char *dst, const char *src, unsigned int len );
DECLEXPORT(void)    crStrcat( char *dst, const char *src );
DECLEXPORT(char *)  crStrjoin( const char *src1, const char *src2 );
DECLEXPORT(char *)  crStrjoin3( const char *src1, const char *src2, const char *src3 );
DECLEXPORT(char *)  crStrstr( const char *str, const char *pat );
DECLEXPORT(char *)  crStrchr( const char *str, char c );
DECLEXPORT(char *)  crStrrchr( const char *str, char c );
DECLEXPORT(int)     crStrToInt( const char *str );
DECLEXPORT(float)   crStrToFloat( const char *str );
DECLEXPORT(char **) crStrSplit( const char *str, const char *splitstr );
DECLEXPORT(char **) crStrSplitn( const char *str, const char *splitstr, int n );
DECLEXPORT(void)    crFreeStrings( char **strings );
DECLEXPORT(char *)  crStrIntersect( const char *s1, const char *s2 );
DECLEXPORT(int)     crIsDigit( char c );

DECLEXPORT(void)    crBytesToString( char *string, int nstring, void *data, int ndata );
DECLEXPORT(void)    crWordsToString( char *string, int nstring, void *data, int ndata );

#define CR_GLVERSION_OFFSET_MAJOR (24)
#define CR_GLVERSION_OFFSET_MINOR (16)
#define CR_GLVERSION_OFFSET_BUILD (0)

#define CR_GLVERSION_MAX_MAJOR (0x7f)
#define CR_GLVERSION_MAX_MINOR (0xff)
#define CR_GLVERSION_MAX_BUILD (0xffff)

#define CR_GLVERSION_MASK_MAJOR (CR_GLVERSION_MAX_MAJOR << CR_GLVERSION_OFFSET_MAJOR)
#define CR_GLVERSION_MASK_MINOR (CR_GLVERSION_MAX_MINOR << CR_GLVERSION_OFFSET_MINOR)
#define CR_GLVERSION_MASK_BUILD (CR_GLVERSION_MAX_BUILD << CR_GLVERSION_OFFSET_BUILD)

#define CR_GLVERSION_COMPOSE_EL(_val, _type) (((_val) << CR_GLVERSION_OFFSET_##_type) & CR_GLVERSION_MASK_##_type)

#define CR_GLVERSION_COMPOSE(_maj, _min, _build) (CR_GLVERSION_COMPOSE_EL((_maj), MAJOR) \
        + CR_GLVERSION_COMPOSE_EL((_min), MINOR) \
        + CR_GLVERSION_COMPOSE_EL((_build), BUILD))

#define CR_GLVERSION_GET_EL(_val, _type) (((_val) & CR_GLVERSION_MASK_##_type) >> CR_GLVERSION_OFFSET_##_type)
#define CR_GLVERSION_GET_MAJOR(_val) CR_GLVERSION_GET_EL((_val), MAJOR)
#define CR_GLVERSION_GET_MINOR(_val) CR_GLVERSION_GET_EL((_val), MINOR)
#define CR_GLVERSION_GET_BUILD(_val) CR_GLVERSION_GET_EL((_val), BUILD)

DECLEXPORT(int) crStrParseGlVersion(const char * ver);
DECLEXPORT(int32_t) crStrParseI32(const char *pszStr, const int32_t defaultVal);
RT_C_DECLS_END

#endif /* CR_STRING_H */
