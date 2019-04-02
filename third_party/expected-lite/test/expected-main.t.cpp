// Copyright (c) 2016-2018 Martin Moene
//
// https://github.com/martinmoene/expected-lite
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "expected-main.t.hpp"

#define expected_PRESENT( x ) \
    std::cout << #x << ": " << x << "\n"

#define expected_ABSENT( x ) \
    std::cout << #x << ": (undefined)\n"

// Suppress:
// - unused parameter, for cases without assertions such as [.std...]
#if defined(__clang__)
# pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined __GNUC__
# pragma GCC   diagnostic ignored "-Wunused-parameter"
#endif

lest::tests & specification()
{
    static lest::tests tests;
    return tests;
}

CASE( "expected-lite version" "[.expected][.version]" )
{
    expected_PRESENT( expected_lite_MAJOR );
    expected_PRESENT( expected_lite_MINOR );
    expected_PRESENT( expected_lite_PATCH );
    expected_PRESENT( expected_lite_VERSION );
}

CASE( "any configuration" "[.expected][.config]" )
{
    expected_PRESENT( nsel_HAVE_STD_EXPECTED );
    expected_PRESENT( nsel_USES_STD_EXPECTED );
    expected_PRESENT( nsel_CONFIG_SELECT_EXPECTED );
    expected_PRESENT( nsel_EXPECTED_DEFAULT );
    expected_PRESENT( nsel_EXPECTED_NONSTD );
    expected_PRESENT( nsel_EXPECTED_STD );
    expected_PRESENT( nsel_CPLUSPLUS );
}

CASE( "__cplusplus" "[.stdc++]" )
{
    expected_PRESENT( __cplusplus );
}

#if 0

CASE( "compiler version" "[.compiler]" )
{
#if expected_COMPILER_GNUC_VERSION
    expected_PRESENT( expected_COMPILER_GNUC_VERSION );
#else
    expected_ABSENT(  expected_COMPILER_GNUC_VERSION );
#endif

#if expected_COMPILER_MSVC_VERSION
    expected_PRESENT( expected_COMPILER_MSVC_VERSION );
#else
    expected_ABSENT(  expected_COMPILER_MSVC_VERSION );
#endif
}

CASE( "presence of C++ language features" "[.stdlanguage]" )
{
#if expected_HAVE_AUTO
    expected_PRESENT( expected_HAVE_AUTO );
#else
    expected_ABSENT(  expected_HAVE_AUTO );
#endif

#if expected_HAVE_NULLPTR
    expected_PRESENT( expected_HAVE_NULLPTR );
#else
    expected_ABSENT(  expected_HAVE_NULLPTR );
#endif

#if expected_HAVE_STATIC_ASSERT
    expected_PRESENT( expected_HAVE_STATIC_ASSERT );
#else
    expected_ABSENT(  expected_HAVE_STATIC_ASSERT );
#endif

#if expected_HAVE_DEFAULT_FUNCTION_TEMPLATE_ARG
    expected_PRESENT( expected_HAVE_DEFAULT_FUNCTION_TEMPLATE_ARG );
#else
    expected_ABSENT(  expected_HAVE_DEFAULT_FUNCTION_TEMPLATE_ARG );
#endif

#if expected_HAVE_ALIAS_TEMPLATE
    expected_PRESENT( expected_HAVE_ALIAS_TEMPLATE );
#else
    expected_ABSENT(  expected_HAVE_ALIAS_TEMPLATE );
#endif

#if expected_HAVE_CONSTEXPR_11
    expected_PRESENT( expected_HAVE_CONSTEXPR_11 );
#else
    expected_ABSENT(  expected_HAVE_CONSTEXPR_11 );
#endif

#if expected_HAVE_CONSTEXPR_14
    expected_PRESENT( expected_HAVE_CONSTEXPR_14 );
#else
    expected_ABSENT(  expected_HAVE_CONSTEXPR_14 );
#endif

#if expected_HAVE_ENUM_CLASS
    expected_PRESENT( expected_HAVE_ENUM_CLASS );
#else
    expected_ABSENT(  expected_HAVE_ENUM_CLASS );
#endif

#if expected_HAVE_ENUM_CLASS_CONSTRUCTION_FROM_UNDERLYING_TYPE
    expected_PRESENT( expected_HAVE_ENUM_CLASS_CONSTRUCTION_FROM_UNDERLYING_TYPE );
#else
    expected_ABSENT(  expected_HAVE_ENUM_CLASS_CONSTRUCTION_FROM_UNDERLYING_TYPE );
#endif

#if expected_HAVE_EXPLICIT_CONVERSION
    expected_PRESENT( expected_HAVE_EXPLICIT_CONVERSION );
#else
    expected_ABSENT(  expected_HAVE_EXPLICIT_CONVERSION );
#endif

#if expected_HAVE_INITIALIZER_LIST
    expected_PRESENT( expected_HAVE_INITIALIZER_LIST );
#else
    expected_ABSENT(  expected_HAVE_INITIALIZER_LIST );
#endif

#if expected_HAVE_IS_DEFAULT
    expected_PRESENT( expected_HAVE_IS_DEFAULT );
#else
    expected_ABSENT(  expected_HAVE_IS_DEFAULT );
#endif

#if expected_HAVE_IS_DELETE
    expected_PRESENT( expected_HAVE_IS_DELETE );
#else
    expected_ABSENT(  expected_HAVE_IS_DELETE );
#endif

#if expected_HAVE_NOEXCEPT
    expected_PRESENT( expected_HAVE_NOEXCEPT );
#else
    expected_ABSENT(  expected_HAVE_NOEXCEPT );
#endif

#if expected_HAVE_REF_QUALIFIER
    expected_PRESENT( expected_HAVE_REF_QUALIFIER );
#else
    expected_ABSENT(  expected_HAVE_REF_QUALIFIER );
#endif
}

CASE( "presence of C++ library features" "[.stdlibrary]" )
{
#if expected_HAVE_ARRAY
    expected_PRESENT( expected_HAVE_ARRAY );
#else
    expected_ABSENT(  expected_HAVE_ARRAY );
#endif

#if expected_HAVE_CONDITIONAL
    expected_PRESENT( expected_HAVE_CONDITIONAL );
#else
    expected_ABSENT(  expected_HAVE_CONDITIONAL );
#endif

#if expected_HAVE_CONTAINER_DATA_METHOD
    expected_PRESENT( expected_HAVE_CONTAINER_DATA_METHOD );
#else
    expected_ABSENT(  expected_HAVE_CONTAINER_DATA_METHOD );
#endif

#if expected_HAVE_REMOVE_CV
    expected_PRESENT( expected_HAVE_REMOVE_CV );
#else
    expected_ABSENT(  expected_HAVE_REMOVE_CV );
#endif

#if expected_HAVE_SIZED_TYPES
    expected_PRESENT( expected_HAVE_SIZED_TYPES );
#else
    expected_ABSENT(  expected_HAVE_SIZED_TYPES );
#endif

#if expected_HAVE_TYPE_TRAITS
    expected_PRESENT( expected_HAVE_TYPE_TRAITS );
#else
    expected_ABSENT(  expected_HAVE_TYPE_TRAITS );
#endif

#if _HAS_CPP0X
    expected_PRESENT( _HAS_CPP0X );
#else
    expected_ABSENT(  _HAS_CPP0X );
#endif
}

#endif // 0

int main( int argc, char * argv[] )
{
    return lest::run( specification(), argc, argv );
}

#if 0
g++            -I../include/nonstd -o expected-lite.t.exe expected-lite.t.cpp && expected-lite.t.exe --pass
g++ -std=c++98 -I../include/nonstd -o expected-lite.t.exe expected-lite.t.cpp && expected-lite.t.exe --pass
g++ -std=c++03 -I../include/nonstd -o expected-lite.t.exe expected-lite.t.cpp && expected-lite.t.exe --pass
g++ -std=c++0x -I../include/nonstd -o expected-lite.t.exe expected-lite.t.cpp && expected-lite.t.exe --pass
g++ -std=c++11 -I../include/nonstd -o expected-lite.t.exe expected-lite.t.cpp && expected-lite.t.exe --pass
g++ -std=c++14 -I../include/nonstd -o expected-lite.t.exe expected-lite.t.cpp && expected-lite.t.exe --pass
g++ -std=c++17 -I../include/nonstd -o expected-lite.t.exe expected-lite.t.cpp && expected-lite.t.exe --pass

cl -EHsc -I../include/nonstd expected-lite.t.cpp && expected-lite.t.exe --pass
#endif

// end of file
