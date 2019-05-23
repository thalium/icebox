/* $Id: ValidationKitCodingGuidelines.cpp $ */
/** @file
 * VirtualBox Validation Kit - Coding Guidelines.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/** @page pg_validationkit_guideline    Validation Kit Coding Guidelines
 *
 * The guidelines extends the VBox coding guidelines (@ref pg_vbox_guideline)
 * and currently only defines python prefixes and linting.
 *
 *
 * @section sec_validationkit_guideline_python      Python
 *
 * Python is a typeless language so using prefixes to indicate the intended
 * type of a variable or attribute can be very helpful.
 *
 * Type prefixes:
 *      - 'b' for byte (octect).
 *      - 'ch' for a single character.
 *      - 'f' for boolean and flags.
 *      - 'fn' for function or method references.
 *      - 'fp' for floating point values.
 *      - 'i' for integers.
 *      - 'l' for long integers.
 *      - 'o' for objects, structures and anything with attributes that doesn't
 *        match any of the other type prefixes.
 *      - 'r' for a range or xrange.
 *      - 's' for a string (can be unicode).
 *      - 'su' for a unicode string when the distinction is important.
 *
 * Collection qualifiers:
 *      - 'a' for a list or an array.
 *      - 'd' for a dictionary.
 *      - 'h' for a hash.
 *      - 't' for a tuple.
 *
 * Other qualifiers:
 *      - 'c' for a count. Implies integer or long integer type. Higest
 *        priority.
 *      - 'sec' for a second value. Implies long integer type.
 *      - 'ms' for a millisecond value. Implies long integer type.
 *      - 'us' for a microsecond value. Implies long integer type.
 *      - 'ns' for a nanosecond value. Implies long integer type.
 *
 * The 'ms', 'us', 'ns' and 'se' qualifiers can be capitalized when prefixed by
 * 'c', e.g. cMsElapsed.  While this technically means they are no longer a
 * prefix, it's easier to read and everyone understands what it means.
 *
 * The type collection qualifiers comes first, then the other qualifiers and
 * finally the type qualifier.
 *
 */

