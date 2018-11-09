// Copyright (c) 2016-2018 Martin Moene
//
// https://github.com/martinmoene/expected-lite
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef TEST_EXPECTED_LITE_H_INCLUDED
#define TEST_EXPECTED_LITE_H_INCLUDED

// Limit C++ Core Guidelines checking to expected-lite:

#include "expected.hpp"

#if defined(_MSC_VER) && _MSC_VER >= 1910
# include <CppCoreCheck/Warnings.h>
# pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)
#endif

#include <iosfwd>
namespace lest {

template< typename T, typename E >
std::ostream & operator<<( std::ostream & os, nonstd::expected<T,E> const & );

template< typename E >
std::ostream & operator<<( std::ostream & os, nonstd::expected<void,E> const & );
} // namespace lest

#include "lest.hpp"

using namespace nonstd;

#define CASE( name ) lest_CASE( specification(), name )

extern lest::tests & specification();

namespace lest {

// use oparator<< instead of to_string() overload;
// see  http://stackoverflow.com/a/10651752/437272

template< typename T, typename E >
inline std::ostream & operator<<( std::ostream & os, nonstd::expected<T,E> const & v )
{
    using lest::to_string;
    return os << "[expected:" << (v ? to_string(*v) : "[empty]") << "]";
}

template< typename E >
inline std::ostream & operator<<( std::ostream & os, nonstd::expected<void,E> const & v )
{
    using lest::to_string;
    return os << "[expected<void>:" << (v ? "[non-empty]" : "[empty]") << "]";
}

} // namespace lest

#endif // TEST_EXPECTED_LITE_H_INCLUDED

// end of file
