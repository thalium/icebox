/* $Id: VBox-CodingGuidelines.cpp $ */
/** @file
 * VBox - Coding Guidelines.
 */

/*
 * Copyright (C) 2006-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/** @page pg_vbox_guideline                 VBox Coding Guidelines
 *
 * The VBox Coding guidelines are followed by all of VBox with the exception of
 * qemu. Qemu is using whatever the frenchman does.
 *
 * There are a few compulsory rules and a bunch of optional ones. The following
 * sections will describe these in details. In addition there is a section of
 * Subversion 'rules'.
 *
 *
 *
 * @section sec_vbox_guideline_compulsory       Compulsory
 *
 * <ul>
 *
 *   <li> The indentation size is 4 chars.
 *
 *   <li> Tabs are only ever used in makefiles.
 *
 *   <li> Use RT and VBOX types.
 *
 *   <li> Use Runtime functions.
 *
 *   <li> Use the standard bool, uintptr_t, intptr_t and [u]int[1-9+]_t types.
 *
 *   <li> Avoid using plain unsigned and int.
 *
 *   <li> Use static wherever possible. This makes the namespace less polluted
 *        and avoids nasty name clash problems which can occur, especially on
 *        Unix-like systems. (1)  It also simplifies locating callers when
 *        changing it (single source file vs entire VBox tree).
 *
 *   <li> Public names are of the form Domain[Subdomain[]]Method, using mixed
 *        casing to mark the words. The main domain is all uppercase.
 *        (Think like java, mapping domain and subdomain to packages/classes.)
 *
 *   <li> Public names are always declared using the appropriate DECL macro. (2)
 *
 *   <li> Internal names starts with a lowercased main domain.
 *
 *   <li> Defines are all uppercase and separate words with underscore.
 *        This applies to enum values too.
 *
 *   <li> Typedefs are all uppercase and contain no underscores to distinguish
 *        them from defines.
 *
 *   <li> Pointer typedefs start with 'P'. If pointer to const then 'PC'.
 *
 *   <li> Function typedefs start with 'FN'. If pointer to FN then 'PFN'.
 *
 *   <li> All files are case sensitive.
 *
 *   <li> Slashes are unix slashes ('/') runtime converts when necessary.
 *
 *   <li> char strings are UTF-8.
 *
 *   <li> Strings from any external source must be treated with utmost care as
 *        they do not have to be valid UTF-8.  Only trust internal strings.
 *
 *   <li> All functions return VBox status codes. There are three general
 *        exceptions from this:
 *
 *       <ol>
 *             <li>Predicate functions. These are function which are boolean in
 *                 nature and usage. They return bool. The function name will
 *                 include 'Has', 'Is' or similar.
 *             <li>Functions which by nature cannot possibly fail.
 *                 These return void.
 *             <li>"Get"-functions which return what they ask for.
 *                 A get function becomes a "Query" function if there is any
 *                 doubt about getting what is ask for.
 *       </ol>
 *
 *   <li> VBox status codes have three subdivisions:
 *         <ol>
 *           <li> Errors, which are VERR_ prefixed and negative.
 *           <li> Warnings, which are VWRN_ prefixed and positive.
 *           <li> Informational, which are VINF_ prefixed and positive.
 *        </ol>
 *
 *   <li> Platform/OS operation are generalized and put in the IPRT.
 *
 *   <li> Other useful constructs are also put in the IPRT.
 *
 *   <li> The code shall not cause compiler warnings. Check this on ALL
 *        the platforms.
 *
 *   <li> The use of symbols leading with single or double underscores is
 *        forbidden as that intrudes on reserved compiler/system namespace. (3)
 *
 *   <li> All files have file headers with $Id and a file tag which describes
 *        the file in a sentence or two.
 *        Note: Use the svn-ps.cmd/svn-ps.sh utility with the -a option to add
 *              new sources with keyword expansion and exporting correctly
 *              configured.
 *
 *   <li> All public functions are fully documented in Doxygen style using the
 *        javadoc dialect (using the 'at' instead of the 'slash' as
 *        commandprefix.)
 *
 *   <li> All structures in header files are described, including all their
 *        members. (Doxygen style, of course.)
 *
 *   <li> All modules have a documentation '\@page' in the main source file
 *        which describes the intent and actual implementation.
 *
 *   <li> Code which is doing things that are not immediately comprehensible
 *        shall include explanatory comments.
 *
 *   <li> Documentation and comments are kept up to date.
 *
 *   <li> Headers in /include/VBox shall not contain any slash-slash C++
 *        comments, only ANSI C comments!
 *
 *   <li> Comments on \#else indicates what begins while the comment on a
 *        \#endif indicates what ended.  Only add these when there are more than
 *        a few lines (6-10) of \#ifdef'ed code, otherwise they're just clutter.
 * </ul>
 *
 * (1) It is common practice on Unix to have a single symbol namespace for an
 *     entire process. If one is careless symbols might be resolved in a
 *     different way that one expects, leading to weird problems.
 *
 * (2) This is common practice among most projects dealing with modules in
 *     shared libraries. The Windows / PE __declspect(import) and
 *     __declspect(export) constructs are the main reason for this.
 *     OTOH, we do perhaps have a bit too detailed graining of this in VMM...
 *
 * (3) There are guys out there grepping public sources for symbols leading with
 *     single and double underscores as well as gotos and other things
 *     considered bad practice.  They'll post statistics on how bad our sources
 *     are on some mailing list, forum or similar.
 *
 *
 * @subsection sec_vbox_guideline_compulsory_sub64  64-bit and 32-bit
 *
 * Here are some amendments which address 64-bit vs. 32-bit portability issues.
 *
 * Some facts first:
 *
 * <ul>
 *
 *   <li> On 64-bit Windows the type long remains 32-bit. On nearly all other
 *        64-bit platforms long is 64-bit.
 *
 *   <li> On all 64-bit platforms we care about, int is 32-bit, short is 16 bit
 *        and char is 8-bit.
 *        (I don't know about any platforms yet where this isn't true.)
 *
 *   <li> size_t, ssize_t, uintptr_t, ptrdiff_t and similar are all 64-bit on
 *        64-bit platforms. (These are 32-bit on 32-bit platforms.)
 *
 *   <li> There is no inline assembly support in the 64-bit Microsoft compilers.
 *
 * </ul>
 *
 * Now for the guidelines:
 *
 * <ul>
 *
 *   <li> Never, ever, use int, long, ULONG, LONG, DWORD or similar to cast a
 *        pointer to integer.  Use uintptr_t or intptr_t. If you have to use
 *        NT/Windows types, there is the choice of ULONG_PTR and DWORD_PTR.
 *
 *   <li> Avoid where ever possible the use of the types 'long' and 'unsigned
 *        long' as these differs in size between windows and the other hosts
 *        (see above).
 *
 *   <li> RT_OS_WINDOWS is defined to indicate Windows. Do not use __WIN32__,
 *        __WIN64__ and __WIN__ because they are all deprecated and scheduled
 *        for removal (if not removed already). Do not use the compiler
 *        defined _WIN32, _WIN64, or similar either. The bitness can be
 *        determined by testing ARCH_BITS.
 *        Example:
 *        @code
 *              #ifdef RT_OS_WINDOWS
 *              // call win32/64 api.
 *              #endif
 *              #ifdef RT_OS_WINDOWS
 *              # if ARCH_BITS == 64
 *              // call win64 api.
 *              # else  // ARCH_BITS == 32
 *              // call win32 api.
 *              # endif // ARCH_BITS == 32
 *              #else  // !RT_OS_WINDOWS
 *              // call posix api
 *              #endif // !RT_OS_WINDOWS
 *        @endcode
 *
 *   <li> There are RT_OS_xxx defines for each OS, just like RT_OS_WINDOWS
 *        mentioned above. Use these defines instead of any predefined
 *        compiler stuff or defines from system headers.
 *
 *   <li> RT_ARCH_X86 is defined when compiling for the x86 the architecture.
 *        Do not use __x86__, __X86__, __[Ii]386__, __[Ii]586__, or similar
 *        for this purpose.
 *
 *   <li> RT_ARCH_AMD64 is defined when compiling for the AMD64 architecture.
 *        Do not use __AMD64__, __amd64__ or __x64_86__.
 *
 *   <li> Take care and use size_t when you have to, esp. when passing a pointer
 *        to a size_t as a parameter.
 *
 *   <li> Be wary of type promotion to (signed) integer. For example the
 *        following will cause u8 to be promoted to int in the shift, and then
 *        sign extended in the assignment 64-bit:
 *        @code
 *              uint8_t u8 = 0xfe;
 *              uint64_t u64 = u8 << 24;
 *              // u64 == 0xfffffffffe000000
 *        @endcode
 *
 * </ul>
 *
 * @subsection sec_vbox_guideline_compulsory_cppmain   C++ guidelines for Main
 *
 * Main is currently (2009) full of hard-to-maintain code that uses complicated
 * templates. The new mid-term goal for Main is to have less custom templates
 * instead of more for the following reasons:
 *
 * <ul>
 *
 *   <li>  Template code is harder to read and understand. Custom templates create
 *         territories which only the code writer understands.
 *
 *   <li>  Errors in using templates create terrible C++ compiler messages.
 *
 *   <li>  Template code is really hard to look at in a debugger.
 *
 *   <li>  Templates slow down the compiler a lot.
 *
 * </ul>
 *
 * In particular, the following bits should be considered deprecated and should
 * NOT be used in new code:
 *
 * <ul>
 *
 *   <li>  everything in include/iprt/cpputils.h (auto_ref_ptr, exception_trap_base,
 *         char_auto_ptr and friends)
 *
 * </ul>
 *
 * Generally, in many cases, a simple class with a proper destructor can achieve
 * the same effect as a 1,000-line template include file, and the code is
 * much more accessible that way.
 *
 * Using standard STL templates like std::list, std::vector and std::map is OK.
 * Exceptions are:
 *
 * <ul>
 *
 *   <li>  Guest Additions because we don't want to link against libstdc++ there.
 *
 *   <li>  std::string should not be used because we have iprt::MiniString and
 *     com::Utf8Str which can convert efficiently with COM's UTF-16 strings.
 *
 *   <li>  std::auto_ptr<> in general; that part of the C++ standard is just broken.
 *     Write a destructor that calls delete.
 *
 * </ul>
 *
 * @subsection sec_vbox_guideline_compulsory_cppqtgui   C++ guidelines for the Qt GUI
 *
 * The Qt GUI is currently (2010) on its way to become more compatible to the
 * rest of VirtualBox coding style wise. From now on, all the coding style
 * rules described in this file are also mandatory for the Qt GUI. Additionally
 * the following rules should be respected:
 *
 * <ul>
 *
 *   <li> GUI classes which correspond to GUI tasks should be prefixed by UI (no VBox anymore)
 *
 *   <li> Classes which extents some of the Qt classes should be prefix by QI
 *
 *   <li> General task classes should be prefixed by C
 *
 *   <li> Slots are prefixed by slt -> sltName
 *
 *   <li> Signals are prefixed by sig -> sigName
 *
 *   <li> Use Qt classes for lists, strings and so on, the use of STL classes should
 *   be avoided
 *
 *   <li> All files like .cpp, .h, .ui, which belong together are located in the
 *   same directory and named the same
 *
 * </ul>
 *
 *
 * @subsection sec_vbox_guideline_compulsory_xslt  XSLT
 *
 * XSLT (eXtensible Stylesheet Language Transformations) is used quite a bit in
 * the Main API area of VirtualBox to generate sources and bindings to that API.
 * There are a couple of common pitfalls worth mentioning:
 *
 * <ul>
 *
 *   <li> Never do repeated //interface[\@name=...] and //enum[\@name=...] lookups
 *        because they are expensive. Instead delcare xsl:key elements for these
 *        searches and do the lookup using the key() function. xsltproc uses
 *        (per current document) hash tables for each xsl:key, i.e. very fast.
 *
 *   <li> When output type is 'text' make sure to call xsltprocNewlineOutputHack
 *        from typemap-shared.inc.xsl every few KB of output, or xsltproc will
 *        end up wasting all the time reallocating the output buffer.
 *
 * </ul>
 *
 * @subsection sec_vbox_guideline_compulsory_doxygen    Doxygen Comments
 *
 * As mentioned above, we shall use doxygen/javadoc style commenting of public
 * functions, typedefs, classes and such.  It is preferred to use this style in
 * as many places as possible.
 *
 * A couple of hints on how to best write doxygen comments:
 *
 * <ul>
 *
 *   <li> A good class, method, function, structure or enum doxygen comment
 *        starts with a one line sentence giving a brief description of the
 *        item.  Details comes in a new paragraph (after blank line).
 *
 *   <li> Except for list generators like \@todo, \@cfgm, \@gcfgm and others,
 *        all doxygen comments are related to things in the code.  So, for
 *        instance you DO NOT add a doxygen \@note comment in the middle of a
 *        because you've got something important to note, you add a normal
 *        comment like 'Note! blah, very importan blah!'
 *
 *   <li> We do NOT use TODO/XXX/BUGBUG or similar markers in the code to flag
 *        things needing fixing later, we always use \@todo doxygen comments.
 *
 *   <li> There is no colon after the \@todo.  And it is ALWAYS in a doxygen
 *        comment.
 *
 *   <li> The \@retval tag is used to explain status codes a method/function may
 *        returns.  It is not used to describe output parameters, that is done
 *        using the \@param or \@param[out] tag.
 *
 * </ul>
 *
 * See https://www.stack.nl/~dimitri/doxygen/manual/index.html for the official
 * doxygen documention.
 *
 *
 * @section sec_vbox_guideline_optional         Optional
 *
 * First part is the actual coding style and all the prefixes.  The second part
 * is a bunch of good advice.
 *
 *
 * @subsection sec_vbox_guideline_optional_layout   The code layout
 *
 * <ul>
 *
 *   <li> Max line length is 130 chars.  Exceptions are table-like
 *        code/initializers and Log*() statements (don't waste unnecessary
 *        vertical space on debug logging).
 *
 *   <li> Comments should try stay within the usual 80 columns as these are
 *        denser and too long lines may be harder to read.
 *
 *   <li> Curly brackets are not indented.  Example:
 *        @code
 *              if (true)
 *              {
 *                  Something1();
 *                  Something2();
 *              }
 *              else
 *              {
 *                  SomethingElse1().
 *                  SomethingElse2().
 *              }
 *        @endcode
 *
 *   <li> Space before the parentheses when it comes after a C keyword.
 *
 *   <li> No space between argument and parentheses. Exception for complex
 *        expression.  Example:
 *        @code
 *              if (PATMR3IsPatchGCAddr(pVM, GCPtr))
 *        @endcode
 *
 *   <li> The else of an if is always the first statement on a line. (No curly
 *        stuff before it!)
 *
 *   <li> else and if go on the same line if no { compound statement }
 *        follows the if.  Example:
 *        @code
 *              if (fFlags & MYFLAGS_1)
 *                  fFlags &= ~MYFLAGS_10;
 *              else if (fFlags & MYFLAGS_2)
 *              {
 *                  fFlags &= ~MYFLAGS_MASK;
 *                  fFlags |= MYFLAGS_5;
 *              }
 *              else if (fFlags & MYFLAGS_3)
 *        @endcode
 *
 *   <li> Slightly complex boolean expressions are split into multiple lines,
 *        putting the operators first on the line and indenting it all according
 *        to the nesting of the expression. The purpose is to make it as easy as
 *        possible to read.  Example:
 *        @code
 *              if (    RT_SUCCESS(rc)
 *                  ||  (fFlags & SOME_FLAG))
 *        @endcode
 *
 *   <li> When 'if' or  'while' statements gets long, the closing parentheses
 *        goes right below the opening parentheses.  This may be applied to
 *        sub-expression.  Example:
 *        @code
 *              if (    RT_SUCCESS(rc)
 *                  ||  (   fSomeStuff
 *                       && fSomeOtherStuff
 *                       && fEvenMoreStuff
 *                      )
 *                  ||  SomePredicateFunction()
 *                 )
 *              {
 *                  ...
 *              }
 *        @endcode
 *
 *   <li> The case is indented from the switch (to avoid having the braces for
 *        the 'case' at the same level as the 'switch' statement).
 *
 *   <li> If a case needs curly brackets they contain the entire case, are not
 *        indented from the case, and the break or return is placed inside them.
 *        Example:
 *        @code
 *              switch (pCur->eType)
 *              {
 *                  case PGMMAPPINGTYPE_PAGETABLES:
 *                  {
 *                      unsigned iPDE = pCur->GCPtr >> PGDIR_SHIFT;
 *                      unsigned iPT = (pCur->GCPtrEnd - pCur->GCPtr) >> PGDIR_SHIFT;
 *                      while (iPT-- > 0)
 *                          if (pPD->a[iPDE + iPT].n.u1Present)
 *                              return VERR_HYPERVISOR_CONFLICT;
 *                      break;
 *                  }
 *              }
 *        @endcode
 *
 *   <li> In a do while construction, the while is on the same line as the
 *        closing "}" if any are used.
 *        Example:
 *        @code
 *              do
 *              {
 *                  stuff;
 *                  i--;
 *              } while (i > 0);
 *        @endcode
 *
 *   <li> Comments are in C style.  C++ style comments are used for temporary
 *        disabling a few lines of code.
 *
 *   <li> No unnecessary parentheses in expressions (just don't over do this
 *        so that gcc / msc starts bitching). Find a correct C/C++ operator
 *        precedence table if needed.
 *
 *   <li> 'for (;;)' is preferred over 'while (true)' and 'while (1)'.
 *
 *   <li> Parameters are indented to the start parentheses when breaking up
 *        function calls, declarations or prototypes.  (This is in line with
 *        how 'if', 'for' and 'while' statements are done as well.) Example:
 *        @code
 *              RTPROCESS hProcess;
 *              int rc = RTProcCreateEx(papszArgs[0],
 *                                      papszArgs,
 *                                      RTENV_DEFAULT,
 *                                      fFlags,
 *                                      NULL,           // phStdIn
 *                                      NULL,           // phStdOut
 *                                      NULL,           // phStdErr
 *                                      NULL,           // pszAsUser
 *                                      NULL,           // pszPassword
 *                                      &hProcess);
 *        @endcode
 *
 *   <li> That Dijkstra is dead is no excuse for using gotos.
 *
 *   <li> Using do-while-false loops to avoid gotos is considered very bad form.
 *        They create hard to read code.  They tend to be either too short (i.e.
 *        pointless) or way to long (split up the function already), making
 *        tracking the state is difficult and prone to bugs.  Also, they cause
 *        the compiler to generate suboptimal code, because the break branches
 *        are by preferred over the main code flow (MSC has no branch hinting!).
 *        Instead, do make use the 130 columns (i.e. nested ifs) and split
 *        the code up into more functions!
 *
 *   <li> Avoid code like
 *        @code
 *              int foo;
 *              int rc;
 *              ...
 *              rc = FooBar();
 *              if (RT_SUCCESS(rc))
 *              {
 *                  foo = getFoo();
 *                  ...
 *                  pvBar = RTMemAlloc(sizeof(*pvBar));
 *                  if (!pvBar)
 *                     rc = VERR_NO_MEMORY;
 *              }   
 *              if (RT_SUCCESS(rc))
 *              {
 *                  buzz = foo;
 *                  ...
 *              }
 *        @endcode
 *        The intention of such code is probably to save some horizontal space
 *        but unfortunately it's hard to read and the scope of certain varables
 *        (e.g. foo in this example) is not optimal. Better use the following
 *        style:
 *        @code
 *              int rc;
 *              ...
 *              rc = FooBar();
 *              if (RT_SUCCESS(rc))
 *              {
 *                  int foo = getFoo();
 *                  ...
 *                  pvBar = RTMemAlloc(sizeof(*pvBar));
 *                  if (pvBar)
 *                  {
 *                      buzz = foo;
 *                      ...
 *                  }
 *                  else
 *                      rc = VERR_NO_MEMORY;
 *              }
 *        @endcode
 *
 * </ul>
 *
 * @subsection sec_vbox_guideline_optional_prefix   Variable / Member Prefixes
 *
 * Prefixes are meant to provide extra context clues to a variable/member, we
 * therefore avoid using prefixes that just indicating the type if a better
 * choice is available.
 *
 *
 * The prefixes:
 *
 * <ul>
 *
 *   <li> The 'g_' (or 'g') prefix means a global variable, either on file or module level.
 *
 *   <li> The 's_' (or 's') prefix means a static variable inside a function or
 *        class.  This is not used for static variables on file level, use 'g_'
 *        for those (logical, right).
 *
 *   <li> The 'm_' (or 'm') prefix means a class data member.
 *
 *        In new code in Main, use "m_" (and common sense).  As an exception,
 *        in Main, if a class encapsulates its member variables in an anonymous
 *        structure which is declared in the class, but defined only in the
 *        implementation (like this: 'class X { struct Data; Data *m; }'), then
 *        the pointer to that struct is called 'm' itself and its members then
 *        need no prefix, because the members are accessed with 'm->member'
 *        already which is clear enough.
 *
 *   <li> The 'a_' prefix means a parameter (argument) variable.  This is
 *        sometimes written 'a' in parts of the source code that does not use
 *        the array prefix.
 *
 *   <li> The 'p' prefix means pointer.  For instance 'pVM' is pointer to VM.
 *
 *   <li> The 'r' prefix means that something is passed by reference.
 *
 *   <li> The 'k' prefix means that something is a constant.  For instance
 *        'enum { kStuff };'.  This is usually not used in combination with
 *        'p', 'r' or any such thing, it's main main use is to make enums
 *        easily identifiable.
 *
 *   <li> The 'a' prefix means array.  For instance 'aPages' could be read as
 *        array of pages.
 *
 *   <li> The 'c' prefix means count.  For instance 'cbBlock' could be read,
 *        count of bytes in block. (1)
 *
 *   <li> The 'cx' prefix means width (count of 'x' units).
 *
 *   <li> The 'cy' prefix means height (count of 'y' units).
 *
 *   <li> The 'x', 'y' and 'z' prefix refers to the x-, y- , and z-axis
 *        respectively.
 *
 *   <li> The 'off' prefix means offset.
 *
 *   <li> The 'i' or 'idx' prefixes usually means index.  Although the 'i' one
 *        can sometimes just mean signed integer.
 *
 *   <li> The 'i[1-9]+' prefix means a fixed bit size variable.  Frequently
 *        used with the int[1-9]+_t types where the width is really important.
 *        In most cases 'i' is more appropriate.  [type]
 *
 *   <li> The 'e' (or 'enm') prefix means enum.
 *
 *   <li> The 'u' prefix usually means unsigned integer.  Exceptions follows.
 *
 *   <li> The 'u[1-9]+' prefix means a fixed bit size variable.  Frequently
 *        used with the uint[1-9]+_t types and with bitfields where the width is
 *        really important.  In most cases 'u' or 'b' (byte) would be more
 *        appropriate.  [type]
 *
 *   <li> The 'b' prefix means byte or bytes.  [type]
 *
 *   <li> The 'f' prefix means flags.  Flags are unsigned integers of some kind
 *        or booleans.
 *
 *   <li> TODO: need prefix for real float.  [type]
 *
 *   <li> The 'rd' prefix means real double and is used for 'double' variables.
 *        [type]
 *
 *   <li> The 'lrd' prefix means long real double and is used for 'long double'
 *        variables. [type]
 *
 *   <li> The 'ch' prefix means a char, the (signed) char type.  [type]
 *
 *   <li> The 'wc' prefix means a wide/windows char, the RTUTF16 type.  [type]
 *
 *   <li> The 'uc' prefix means a Unicode Code point, the RTUNICP type.  [type]
 *
 *   <li> The 'uch' prefix means unsigned char.  It's rarely used.  [type]
 *
 *   <li> The 'sz' prefix means zero terminated character string (array of
 *        chars). (UTF-8)
 *
 *   <li> The 'wsz' prefix means zero terminated wide/windows character string
 *        (array of RTUTF16).
 *
 *   <li> The 'usz' prefix means zero terminated Unicode string (array of
 *        RTUNICP).
 *
 *   <li> The 'str' prefix means C++ string; either a std::string or, in Main,
 *        a Utf8Str or, in Qt, a QString.  When used with 'p', 'r', 'a' or 'c'
 *        the first letter should be capitalized.
 *
 *   <li> The 'bstr' prefix, in Main, means a UTF-16 Bstr. When used with 'p',
 *        'r', 'a' or 'c' the first letter should be capitalized.
 *
 *   <li> The 'pfn' prefix means pointer to function. Common usage is 'pfnCallback'
 *        and such like.
 *
 *   <li> The 'psz' prefix is a combination of 'p' and 'sz' and thus means
 *        pointer to a zero terminated character string. (UTF-8)
 *
 *   <li> The 'pcsz' prefix is used to indicate constant string pointers in
 *        parts of the code.  Most code uses 'psz' for const and non-const
 *        string pointers, so please ignore this one.
 *
 *   <li> The 'l' prefix means (signed) long.  We try avoid using this,
 *        expecially with the 'LONG' types in Main as these are not 'long' on
 *        64-bit non-Windows platforms and can cause confusion. Alternatives:
 *        'i' or 'i32'.  [type]
 *
 *   <li> The 'ul' prefix means unsigned long.  We try avoid using this,
 *        expecially with the 'ULONG' types in Main as these are not 'unsigned
 *        long' on 64-bit non-Windows platforms and can cause confusion.
 *        Alternatives: 'u' or 'u32'.  [type]
 *
 * </ul>
 *
 * (1)  Except in the occasional 'pcsz' prefix, the 'c' prefix is never ever
 *      used in the meaning 'const'.
 *
 *
 * @subsection sec_vbox_guideline_optional_misc     Misc / Advice / Stuff
 *
 * <ul>
 *
 *   <li> When writing code think as the reader.
 *
 *   <li> When writing code think as the compiler. (2)
 *
 *   <li> When reading code think as if it's full of bugs - find them and fix them.
 *
 *   <li> Pointer within range tests like:
 *        @code
 *          if ((uintptr_t)pv >= (uintptr_t)pvBase && (uintptr_t)pv < (uintptr_t)pvBase + cbRange)
 *        @endcode
 *        Can also be written as (assuming cbRange unsigned):
 *        @code
 *          if ((uintptr_t)pv - (uintptr_t)pvBase < cbRange)
 *        @endcode
 *        Which is shorter and potentially faster. (1)
 *
 *   <li> Avoid unnecessary casting. All pointers automatically cast down to
 *        void *, at least for non class instance pointers.
 *
 *   <li> It's very very bad practise to write a function larger than a
 *        screen full (1024x768) without any comprehensibility and explaining
 *        comments.
 *
 *   <li> More to come....
 *
 * </ul>
 *
 * (1)  Important, be very careful with the casting. In particular, note that
 *      a compiler might treat pointers as signed (IIRC).
 *
 * (2)  "A really advanced hacker comes to understand the true inner workings of
 *      the machine - he sees through the language he's working in and glimpses
 *      the secret functioning of the binary code - becomes a Ba'al Shem of
 *      sorts."   (Neal Stephenson "Snow Crash")
 *
 *
 *
 * @section sec_vbox_guideline_warnings     Compiler Warnings
 *
 * The code should when possible compile on all platforms and compilers without any
 * warnings. That's a nice idea, however, if it means making the code harder to read,
 * less portable, unreliable or similar, the warning should not be fixed.
 *
 * Some of the warnings can seem kind of innocent at first glance. So, let's take the
 * most common ones and explain them.
 *
 *
 * @subsection sec_vbox_guideline_warnings_signed_unsigned_compare      Signed / Unsigned Compare
 *
 * GCC says: "warning: comparison between signed and unsigned integer expressions"
 * MSC says: "warning C4018: '<|<=|==|>=|>' : signed/unsigned mismatch"
 *
 * The following example will not output what you expect:
@code
#include <stdio.h>
int main()
{
    signed long a = -1;
    unsigned long b = 2294967295;
    if (a < b)
        printf("%ld < %lu: true\n", a, b);
    else
        printf("%ld < %lu: false\n", a, b);
    return 0;
}
@endcode
 * If I understood it correctly, the compiler will convert a to an
 * unsigned long before doing the compare.
 *
 *
 *
 * @section sec_vbox_guideline_svn          Subversion Commit Rules
 *
 *
 * Before checking in:
 *
 * <ul>
 *
 *   <li> Check Tinderbox and make sure the tree is green across all platforms. If it's
 *        red on a platform, don't check in. If you want, warn in the \#vbox channel and
 *        help make the responsible person fix it.
 *        NEVER CHECK IN TO A BROKEN BUILD.
 *
 *   <li> When checking in keep in mind that a commit is atomic and that the Tinderbox and
 *        developers are constantly checking out the tree. Therefore do not split up the
 *        commit unless it's into 100% independent parts. If you need to split it up in order
 *        to have sensible commit comments, make the sub-commits as rapid as possible.
 *
 *   <li> If you make a user visible change, such as fixing a reported bug,
 *        make sure you add an entry to doc/manual/user_ChangeLogImpl.xml.
 *
 *   <li> If you are adding files make sure set the right attributes.
 *        svn-ps.sh/cmd was created for this purpose, please make use of it.
 *
 * </ul>
 *
 * After checking in:
 *
 * <ul>
 *
 *   <li> After checking-in, you watch Tinderbox until your check-ins clear. You do not
 *        go home. You do not sleep. You do not log out or experiment with drugs. You do
 *        not become unavailable. If you break the tree, add a comment saying that you're
 *        fixing it. If you can't fix it and need help, ask in the \#innotek channel or back
 *        out the change.
 *
 * </ul>
 *
 * (Inspired by mozilla tree rules.)
 */

