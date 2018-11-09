// This version targets C++11 and later.
//
// Copyright (c) 2016-2018 Martin Moene.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// expected lite is based on:
//   A proposal to add a utility class to represent expected monad
//   by Vicente J. Botet Escriba and Pierre Talbot, http:://wg21.link/p0323

#include "expected-main.t.hpp"

// Suppress:
// - unused parameter, for cases without assertions such as [.std...]
#if defined(__clang__)
# pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined __GNUC__
# pragma GCC   diagnostic ignored "-Wunused-parameter"
#endif

using namespace nonstd;

enum State
{
    sDefaultConstructed,
    sValueCopyConstructed,
    sValueMoveConstructed,
    sCopyConstructed,
    sMoveConstructed,
    sMoveAssigned,
    sCopyAssigned,
    sValueCopyAssigned,
    sValueMoveAssigned,
    sMovedFrom,
    sValueConstructed
};

struct OracleVal
{
    State s;
    int i;
    OracleVal(int i = 0) : s(sValueConstructed), i(i) {}

    bool operator==( OracleVal const & other ) const { return s==other.s && i==other.i; }
};

struct Oracle
{
    State s;
    OracleVal val;

    Oracle() : s(sDefaultConstructed) {}
    Oracle(const OracleVal& v) : s(sValueCopyConstructed), val(v) {}
    Oracle(OracleVal&& v) : s(sValueMoveConstructed), val(std::move(v)) {v.s = sMovedFrom;}
    Oracle(const Oracle& o) : s(sCopyConstructed), val(o.val) {}
    Oracle(Oracle&& o) : s(sMoveConstructed), val(std::move(o.val)) {o.s = sMovedFrom;}

    Oracle& operator=(const OracleVal& v) { s = sValueCopyConstructed; val = v; return *this; }
    Oracle& operator=(OracleVal&& v) { s = sValueMoveConstructed; val = std::move(v); v.s = sMovedFrom; return *this; }
    Oracle& operator=(const Oracle& o) { s = sCopyConstructed; val = o.val; return *this; }
    Oracle& operator=(Oracle&& o) { s = sMoveConstructed; val = std::move(o.val); o.s = sMovedFrom; return *this; }

    bool operator==( Oracle const & other ) const { return s == other.s && val == other.val;}
};

std::ostream & operator<<( std::ostream & os, OracleVal const & o )
{
    using lest::to_string;
    return os << "[oracle:" << to_string( o.i ) << "]";
}

namespace nonstd {

    template< typename T, typename E >
    std::ostream & operator<<( std::ostream & os, expected<T,E> const & e )
    {
        using lest::to_string;
        return os << ( e ? to_string( *e ) : "[error:" + to_string( e.error() ) + "]" );
    }

    template< typename E >
    std::ostream & operator<<( std::ostream & os, unexpected_type<E> const & u )
    {
        using lest::to_string;
        return os << "[unexp:" << to_string( u.value() ) << "]";
    }
}

using namespace nonstd;

std::exception_ptr make_ep()
{
    try
    {
        // this generates an std::out_of_range:
        std::string().at(1);
    }
    catch(...)
    {
        return std::current_exception();
    }
    return nullptr;
}

CASE( "A C++11 union can contain non-POD types" "[.]" )
{
    struct nonpod { nonpod(){} };

    union U
    {
        char c;
        nonpod np;
    };
}

// -----------------------------------------------------------------------
// storage_t<>

CASE( "[storage_t]" "[.implement]" )
{
}

// -----------------------------------------------------------------------
// unexpected_type<>, unexpected_type<std::exception_ptr>

CASE( "unexpected_type<>: Disallows default construction" )
{
#if nsel_CONFIG_CONFIRMS_COMPILATION_ERRORS
    unexpected_type<Oracle> u;
#endif
}

CASE( "unexpected_type<>: Disallows default construction, std::exception_ptr specialization" )
{
#if nsel_CONFIG_CONFIRMS_COMPILATION_ERRORS
    unexpected_type<std::exception_ptr> u;
#endif
}

CASE( "unexpected_type<>: Allows to copy-construct from error_type" )
{
    Oracle o;

    unexpected_type<Oracle> u{ o };

    EXPECT( u.value().s == sCopyConstructed );
}

CASE( "unexpected_type<>: Allows to copy-construct from error_type, std::exception_ptr specialization" )
{
    auto ep = make_ep();

    unexpected_type<std::exception_ptr> u{ ep };

    EXPECT( u.value() == ep );
}

CASE( "unexpected_type<>: Allows to move-construct from error_type" )
{
    unexpected_type<Oracle> u{ Oracle{} };

    EXPECT( u.value().s == sMoveConstructed );
}

CASE( "unexpected_type<>: Allows to move-construct from error_type, std::exception_ptr specialization" )
{
    auto ep_move = make_ep();
    auto const ep_copy = ep_move;

    unexpected_type<std::exception_ptr> u{ std::move( ep_move ) };

    EXPECT( u.value() == ep_copy );
}

CASE( "unexpected_type<>: Allows to copy-construct from an exception, std::exception_ptr specialization" )
{
    std::string text = "hello, world";

    unexpected_type<std::exception_ptr> u{ std::make_exception_ptr( std::logic_error( text.c_str() ) ) };

    try
    {
         std::rethrow_exception( u.value() );
    }
    catch( std::exception const & e )
    {
        EXPECT( e.what() == text );
    }
}

CASE( "unexpected_type<>: Allows to observe its value" )
{
    auto const error_value = 7;
    unexpected_type<int> u{ error_value };

    EXPECT( u.value() == error_value );
}

CASE( "unexpected_type<>: Allows to observe its value, std::exception_ptr specialization" )
{
    auto const ep = make_ep();
    unexpected_type<std::exception_ptr> u{ ep };

    EXPECT( u.value() == ep );
}

CASE( "unexpected_type<>: Allows to modify its value" )
{
    auto const error_value = 9;
    unexpected_type<int> u{ 7 };

    u.value() = error_value;

    EXPECT( u.value() == error_value );
}

CASE( "unexpected_type<>: Allows to modify its value, std::exception_ptr specialization" )
{
    auto const ep1 = make_ep();
    auto const ep2 = make_ep();
    unexpected_type<std::exception_ptr> u{ ep1 };

    u.value() = ep2;

    EXPECT( u.value() == ep2 );
}

//CASE( "unexpected_type<>: Allows reset via = {}" )
//{
//    unexpected_type<int> u( 3 );
//
//    u = {};
//}

//CASE( "unexpected_type<>: Allows reset via = {}, std::exception_ptr specialization" )
//{
//    unexpected_type<int> u( 3 );
//
//    u = {};
//}

// unexpected_type<> relational operators

CASE( "unexpected_type<>: Provides relational operators" )
{
    SETUP( "" ) {
        unexpected_type<int> u1( 6 );
        unexpected_type<int> u2( 7 );

    // compare engaged expected with engaged expected

    SECTION( "==" ) { EXPECT( u1 == u1 ); }
    SECTION( "!=" ) { EXPECT( u1 != u2 ); }
#if nsel_P0323R <= 2
    SECTION( "< " ) { EXPECT( u1 <  u2 ); }
    SECTION( "> " ) { EXPECT( u2 >  u1 ); }
    SECTION( "<=" ) { EXPECT( u1 <= u1 ); }
    SECTION( "<=" ) { EXPECT( u1 <= u2 ); }
    SECTION( ">=" ) { EXPECT( u1 >= u1 ); }
    SECTION( ">=" ) { EXPECT( u2 >= u1 ); }
#endif
    }
}

CASE( "unexpected_type<>: Provides relational operators, std::exception_ptr specialization" )
{
    SETUP( "" ) {
        unexpected_type<std::exception_ptr>  u( make_ep() );
        unexpected_type<std::exception_ptr> u2( make_ep() );

    // compare engaged expected with engaged expected

    SECTION( "==" ) { EXPECT    ( u == u ); }
    SECTION( "!=" ) { EXPECT    ( u != u2); }
#if nsel_P0323R <= 2
    SECTION( "< " ) { EXPECT_NOT( u <  u ); }
    SECTION( "> " ) { EXPECT_NOT( u >  u ); }
    SECTION( "< " ) { EXPECT_NOT( u <  u2); }
    SECTION( "> " ) { EXPECT_NOT( u >  u2); }
    SECTION( "<=" ) { EXPECT    ( u <= u ); }
    SECTION( ">=" ) { EXPECT    ( u >= u ); }
#endif
    }
}

// unexpected: traits

CASE( "is_unexpected<X>: Is true for unexpected_type" "[.deprecated]" )
{
#if nsel_P0323R <= 3
    EXPECT( is_unexpected<unexpected_type<std::exception_ptr>>::value );
#else
    EXPECT( !!"is_unexpected<> is not available (nsel_P0323R > 3)" );
#endif
}

CASE( "is_unexpected<X>: Is false for non-unexpected_type (int)" "[.deprecated]" )
{
#if nsel_P0323R <= 3
    EXPECT_NOT( is_unexpected<int>::value );
#else
    EXPECT( !!"is_unexpected<> is not available (nsel_P0323R > 3)" );
#endif
}

// unexpected: factory

CASE( "make_unexpected(): Allows to create an unexpected_type<E> from an E" )
{
    auto const error = 7;
    auto u = make_unexpected( error );

    EXPECT( u.value() == error );
}

CASE( "make_unexpected_from_current_exception(): Allows to create an unexpected_type<std::exception_ptr> from the current exception" "[.deprecated]" )
{
#if nsel_P0323R <= 2
    std::string text = "hello, world";

    try
    {
         throw std::logic_error( text.c_str() );
    }
    catch( std::exception const & )
    {
        auto u = make_unexpected_from_current_exception() ;

        try
        {
            std::rethrow_exception( u.value() );
        }
        catch( std::exception const & e )
        {
            EXPECT( e.what() == text );
        }
    }
#else
    EXPECT( !!"make_unexpected_from_current_exception() is not available (nsel_P0323R > 2)" );
#endif
}

CASE( "unexpected<>: C++17 and later provide unexpected_type as unexpected" )
{
#if nsel_CPP17_OR_GREATER && nsel_COMPILER_MSVC_VERSION > 141
    unexpected<Oracle> u;
#else
    EXPECT( !!"unexpected<> is not available (no C++17)." );
#endif
}

// -----------------------------------------------------------------------
// bad_expected_access<>

CASE( "bad_expected_access<>: Disallows default construction" )
{
#if nsel_CONFIG_CONFIRMS_COMPILATION_ERRORS
    bad_expected_access<int> bad;
#endif
}

CASE( "bad_expected_access<>: Allows construction from error_type" )
{
    bad_expected_access<int> bea( 123 );
}

CASE( "bad_expected_access<>: Allows to observe its error" )
{
    const int error = 7;
    bad_expected_access<int> bea( error );

    EXPECT( bea.error() == error );
}

CASE( "bad_expected_access<>: Allows to change its error" )
{
    const int old_error = 0;
    const int new_error = 7;
    bad_expected_access<int> bea( old_error );

    bea.error() = new_error;

    EXPECT( bea.error() == new_error  );
}

// -----------------------------------------------------------------------
// expected<>

// expected<> constructors

CASE( "expected<>: Allows default construction" )
{
    expected<int, char> e;
}

CASE( "expected<>: Allows to copy-construct from value_type" )
{
    Oracle o;

    expected<Oracle, char> e{ o };

    EXPECT( e.value().s == sCopyConstructed );
}

CASE( "expected<>: Allows to move-construct from value_type" )
{
    expected<Oracle, char> e{ Oracle{} };

    EXPECT( e.value().s == sMoveConstructed );
}

CASE( "expected<>: Allows to copy-construct from expected<>" )
{
    auto const value = 7;
    OracleVal v{ value };
    expected<Oracle, char> eo{ v };

    expected<expected<Oracle, char>, char> eeo{ eo };

    EXPECT( eeo.value().value().s     == sCopyConstructed );
    EXPECT( eeo.value().value().val.s == sValueConstructed );
    EXPECT( eeo.value().value().val   == value );
}

CASE( "expected<>: Allows to move-construct from expected<>" )
{
    auto const value = 7;

    expected<expected<Oracle, char>, char> eeo{ expected<Oracle, char>{ OracleVal{ value } } };

    EXPECT( eeo.value().value().s == sMoveConstructed );
    EXPECT( eeo.value().value().val == value );
}

CASE( "expected<>: Allows to in-place-construct value_type" )
{
    auto const value = 7;

    expected<OracleVal, char> eo{ in_place, value };

    EXPECT( eo.value().s == sValueConstructed );
    EXPECT( eo.value().i == value );
}

CASE( "expected<>: Allows to in-place-construct value_type, with initializer_list" "[.implement]" )
{
    EXPECT( !"implement" );
}

CASE( "expected<>: Allows to copy-construct from unexpected_type<>" )
{
    auto const value = 7;
    unexpected_type<Oracle> u{ OracleVal{ value } };

    expected<int, Oracle> e{ u };

    EXPECT( e.error().s == sCopyConstructed );
    EXPECT( e.error().val.i == value );
}

CASE( "expected<>: Allows to move-construct from unexpected_type<>" )
{
    auto const value = 7;

    expected<int, Oracle> e{ unexpected_type<Oracle>{ OracleVal{ value } } };

    EXPECT( e.error().s == sMoveConstructed );
    EXPECT( e.error().val.i == value );
}

CASE( "expected<>: Allows to in-place-construct unexpected_type<>" )
{
    auto const value = 7;

    expected<int, Oracle> em{ unexpect, OracleVal{ value } };

    EXPECT( em.error().s == sMoveConstructed );
    EXPECT( em.error().val.i == value );

    // Note: ov is moved-from:
    OracleVal const ov{ value };

    expected<int, Oracle> ec{ unexpect, ov };

    EXPECT( ec.error().s == sMoveConstructed );
    EXPECT( ec.error().val.i == value );
}

CASE( "expected<>: Allows to in-place-construct from unexpected_type<>, with initializer_list" "[.implement]" )
{
    EXPECT( !"implement" );
}

// expected<> assignment

CASE( "expected<>: Allows to copy-assign from expected<>" )
{
    auto const value = 7;
    expected<Oracle, char> ev{ OracleVal{ value } };

    expected<Oracle, char> ec{ ev };

    EXPECT( ev.value().s == sValueMoveConstructed );
    EXPECT( ec.value().s == sCopyConstructed );

    EXPECT( ev.value().val.i == value );
    EXPECT( ec.value().val.i == value );
}

CASE( "expected<>: Allows to move-assign from expected<>" )
{
    auto const value = 7;
    expected<Oracle, char> ev{ OracleVal{ value } };

    expected<Oracle, char> ec{ std::move(ev) };

    EXPECT( ev.value().s == sMovedFrom );
    EXPECT( ec.value().s == sMoveConstructed );

    EXPECT( ev.value().val.i == value );
    EXPECT( ec.value().val.i == value );
}

CASE( "expected<>: Allows to copy-assign from unexpected_type<>" "[.implement]" )
{
    EXPECT( !"implement" );
}

CASE( "expected<>: Allows to move-assign from unexpected_type<>" "[.implement]" )
{
    EXPECT( !"implement" );
}

CASE( "expected<>: Allows to copy-assign from type convertible to value_type" )
{
    auto const value = 7;
    OracleVal ov{ value };

    expected<Oracle, char> ec{ ov };

    EXPECT( ov.s == sValueConstructed );
    EXPECT( ec.value().s == sValueCopyConstructed);
    EXPECT( ec.value().val.i == value );
}

CASE( "expected<>: Allows to move-assign from type convertible to value_type" )
{
    auto const value = 7;
    OracleVal om{ value };

    expected<Oracle, char> em{ std::move(om) };

    EXPECT( om.s == sMovedFrom );
    EXPECT( em.value().s == sValueMoveConstructed );
    EXPECT( em.value().val.i == value );
}

CASE( "expected<>: Allows to emplace a value_type" "[.implement]" )
{
    EXPECT( !"implement" );
}

CASE( "expected<>: Allows to emplace a value_type, with initializer_list" "[.implement]" )
{
    EXPECT( !"implement" );
}

// expected<> swap

CASE( "expected<>: Allows to be swapped" )
{
    auto const vl = OracleVal{ 3 } ;
    auto const vr = OracleVal{ 7 } ;
    {
    expected<Oracle, char> el{ vl };
    expected<Oracle, char> er{ vr };

    el.swap( er );

    EXPECT( el.value().val == vr );
    EXPECT( er.value().val == vl );

    using std::swap;
    swap( el, er );

    EXPECT( el.value().val == vl );
    EXPECT( er.value().val == vr );

    }{

    expected<int, Oracle> el{ unexpect, vl };
    expected<int, Oracle> er{ unexpect, vr };

    el.swap( er );

    EXPECT( el.error().val == vr );
    EXPECT( er.error().val == vl );

    using std::swap;
    swap( el, er );

    EXPECT( el.error().val == vl );
    EXPECT( er.error().val == vr );
    }
}

// expected<> observers

struct Composite { int member; };

CASE( "expected<>: Allows to observe its value via a pointer" )
{
    auto const value = 7;
    expected<Composite, char> ei{ Composite{value} };

    EXPECT( ei->member == value );
}

CASE( "expected<>: Allows to observe its value via a pointer to constant" )
{
    auto const value = 7;
    const expected<Composite, char> ei{ Composite{value} };

    EXPECT( ei->member == value );
}

CASE( "expected<>: Allows to modify its value via a pointer" )
{
    auto const old_value = 3;
    auto const new_value = 7;
    expected<Composite, char> ei{ Composite{old_value} };

    ei->member = new_value;

    EXPECT( ei->member == new_value );
}

CASE( "expected<>: Allows to observe its value via a reference" )
{
    auto const value = 7;
    expected<int, char> const ei{ value };

    EXPECT( *ei == value );
}

CASE( "expected<>: Allows to observe its value via a r-value reference" )
{
    auto const value = 7;

    EXPECT( (expected<int, char>{ value }) == value );
}

CASE( "expected<>: Allows to modify its value via a reference" )
{
    auto const old_value = 3;
    auto const new_value = 7;
    expected<int, char> ei{ old_value };

    *ei = new_value;

    EXPECT( *ei == new_value );
}

CASE( "expected<>: Allows to observe if it contains a value (or error)" )
{
    expected<int, int> ei;
    expected<int, int> ee{ unexpect, 3 };

    EXPECT(  ei );
    EXPECT( !ee );
}

CASE( "expected<>: Allows to observe its value" )
{
    auto const value = 7;
    expected<int, char> const ei{ value };

    EXPECT( ei.value() == value );
}

CASE( "expected<>: Allows to modify its value" )
{
    auto const old_value = 3;
    auto const new_value = 7;
    expected<int, char> ei{ old_value };

    ei.value() = new_value;

    EXPECT( ei.value() == new_value );
}

CASE( "expected<>: Allows to move its value" )
{
    auto const value = 7;
    expected<Oracle, char> m{ OracleVal{ value } };

    expected<Oracle, char> e{ std::move( m.value() ) };

    EXPECT( m.value().s == sMovedFrom );
    EXPECT( e.value().val.i == value );
}

CASE( "expected<>: Allows to observe its error" )
{
    auto const value = 7;
    expected<int, int> const ee{ unexpect, value };

    EXPECT( ee.error() == value );
}

CASE( "expected<>: Allows to modify its error" )
{
    auto const old_value = 3;
    auto const new_value = 7;
    expected<int, int> ee{ unexpect, old_value };

    ee.error() = new_value;

    EXPECT( ee.error() == new_value );
}

CASE( "expected<>: Allows to move its error" )
{
    auto const value = 7;
    expected<int, Oracle> m{ unexpect, OracleVal{ value } };

    expected<int, Oracle> e{ unexpect, std::move( m.error() ) };

    EXPECT( m.error().s == sMovedFrom );
    EXPECT( e.error().val.i == value );
}

CASE( "expected<>: Allows to observe its error as unexpected<>" )
{
    auto const value = 7;
    expected<int, int> e{ unexpect, value };

    EXPECT( e.get_unexpected().value() == value );
}

CASE( "expected<>: Allows to query if it contains an exception of a specific base type" )
{
    expected<int, std::out_of_range> e{ unexpect, std::out_of_range( "oor" ) };

    EXPECT( e.has_exception< std::logic_error >() );
    EXPECT( !e.has_exception< std::runtime_error >() );
}

CASE( "expected<>: Allows to observe its value if available, or obtain a specified value otherwise" )
{
    auto const e_value = 3;
    auto const u_value = 7;
    expected<int, int> e{ e_value };
    expected<int, int> u{ unexpect, 0 };

    EXPECT( e.value_or( u_value ) == e_value );
    EXPECT( u.value_or( u_value ) == u_value );
}

CASE( "expected<>: Allows to move its value if available, or obtain a specified value otherwise" )
{
    auto ov = Oracle{ 3 };
    auto uv = Oracle{ 7 };
    expected<Oracle, char> m{ ov };
    EXPECT( m.value().s == sCopyConstructed );

    expected<Oracle, char> e{ std::move( m ).value_or( std::forward<Oracle>( uv ) ) };

    EXPECT( e.value().val == ov.val );
    EXPECT( m.value().s == sMovedFrom );
    EXPECT( e.value().s == sMoveConstructed );
}

// -----------------------------------------------------------------------
// expected<void> specialization

// constructors

CASE( "expected<void>: Allows to default-construct" )
{
    expected<void, char> ev;

    EXPECT( ev.has_value() );
}

CASE( "expected<void>: Allows to copy-construct from expected<void>" )
{
    auto const value = 7;
    OracleVal v{ value };
    expected<void, Oracle> ev1{ unexpect, v };

    expected<void, Oracle> ev2{ ev1 };

    EXPECT( ev2.error().s     == sCopyConstructed );
    EXPECT( ev2.error().val.s == sValueConstructed );
    EXPECT( ev2.error().val   == value );
}

CASE( "expected<void>: Allows to move-construct from expected<void>" )
{
    auto const value = 7;

    expected<void, Oracle> ev{ expected<void, Oracle>{ unexpect, OracleVal{ value } } };

    EXPECT( ev.error().s   == sMoveConstructed );
    EXPECT( ev.error().val == value );
}

CASE( "expected<void>: Allows to in-place-construct" )
{
    expected<void, char> ev{ in_place };

    EXPECT( ev.has_value() );
}

CASE( "expected<void>: Allows to copy-construct from unexpected_type<>" )
{
    auto const value = 7;
    unexpected_type<Oracle> u{ OracleVal{ value } };

    expected<void, Oracle> ev{ u };

    EXPECT( ev.error().s == sCopyConstructed );
    EXPECT( ev.error().val.i == value );
}

CASE( "expected<void>: Allows to move-construct from unexpected_type<>" )
{
    auto const value = 7;

    expected<void, Oracle> ev{ unexpected_type<Oracle>{ OracleVal{ value } } };

    EXPECT( ev.error().s == sMoveConstructed );
    EXPECT( ev.error().val.i == value );
}

CASE( "expected<void>: Allows to in-place-construct unexpected_type<>" )
{
    auto const value = 7;

    expected<void, Oracle> em{ unexpect, OracleVal{ value } };

    EXPECT( em.error().s == sMoveConstructed );
    EXPECT( em.error().val.i == value );

    // Note: ov is moved-from:
    OracleVal const ov{ value };

    expected<void, Oracle> ec{ unexpect, ov };

    EXPECT( ec.error().s == sMoveConstructed );
    EXPECT( ec.error().val.i == value );
}

CASE( "expected<void>: Allows to copy-assign from expected<>" )
{
    auto const value = 7;
    expected<void, Oracle> ev{ unexpect, OracleVal{ value } };

    expected<void, Oracle> ec{ ev };

    EXPECT( ev.error().s == sMoveConstructed );
    EXPECT( ec.error().s == sCopyConstructed );

    EXPECT( ev.error().val.i == value );
    EXPECT( ec.error().val.i == value );
}

CASE( "expected<void>: Allows to move-assign from expected<>" )
{
    auto const value = 7;
    expected<void, Oracle> ev{ unexpect, OracleVal{ value } };

    expected<void, Oracle> ec{ std::move(ev) };

    EXPECT( ev.error().s == sMovedFrom );
    EXPECT( ec.error().s == sMoveConstructed );

    EXPECT( ev.error().val.i == value );
    EXPECT( ec.error().val.i == value );
}

CASE( "expected<void>: Allows to be swapped" )
{
    auto const vl = OracleVal{ 3 } ;
    auto const vr = OracleVal{ 7 } ;

    expected<void, Oracle> el{ unexpect, vl };
    expected<void, Oracle> er{ unexpect, vr };

    el.swap( er );

    EXPECT( el.error().val == vr );
    EXPECT( er.error().val == vl );

    using std::swap;
    swap( el, er );

    EXPECT( el.error().val == vl );
    EXPECT( er.error().val == vr );
}

CASE( "expected<void>: Allows to observe if it contains a value (or error)" )
{
    expected<void, int> ev;
    expected<void, int> ee{ unexpect, 3 };

    EXPECT(  ev );
    EXPECT( !ee );
}

CASE( "expected<void>: Allows to observe its error" )
{
    auto const value = 7;
    expected<int, int> const ee{ unexpect, value };

    EXPECT( ee.error() == value );
}

CASE( "expected<void>: Allows to modify its error" )
{
    auto const old_value = 3;
    auto const new_value = 7;
    expected<void, int> ee{ unexpect, old_value };

    ee.error() = new_value;

    EXPECT( ee.error() == new_value );
}

CASE( "expected<void>: Allows to move its error" )
{
    auto const value = 7;
    expected<void, Oracle> m{ unexpect, OracleVal{ value } };

    expected<void, Oracle> e{ unexpect, std::move( m.error() ) };

    EXPECT( m.error().s == sMovedFrom );
    EXPECT( e.error().val.i == value );
}

CASE( "expected<void>: Allows to observe its error as unexpected<>" )
{
    auto const value = 7;
    expected<void, int> e{ unexpect, value };

    EXPECT( e.get_unexpected().value() == value );
}

// [expected<> unwrap()]

// [expected<> factories]

// expected<> relational operators

CASE( "operator op: Provides relational operators" )
{
    SETUP( "" ) {
        expected<int, char> e1( 6 );
        expected<int, char> e2( 7 );

        unexpected_type<char> u( 'u' );
        expected<int, char> d( u );

    // compare engaged expected with engaged expected

    SECTION( "engaged    == engaged"    ) { EXPECT( e1 == e1 ); }
    SECTION( "engaged    != engaged"    ) { EXPECT( e1 != e2 ); }
#if nsel_P0323R <= 2
    SECTION( "engaged    <  engaged"    ) { EXPECT( e1 <  e2 ); }
    SECTION( "engaged    >  engaged"    ) { EXPECT( e2 >  e1 ); }
    SECTION( "engaged    <= engaged"    ) { EXPECT( e1 <= e1 ); }
    SECTION( "engaged    <= engaged"    ) { EXPECT( e1 <= e2 ); }
    SECTION( "engaged    >= engaged"    ) { EXPECT( e1 >= e1 ); }
    SECTION( "engaged    >= engaged"    ) { EXPECT( e2 >= e1 ); }
#endif

    // compare engaged expected with value

    SECTION( "engaged    == value"      ) { EXPECT( e1 == 6  ); }
    SECTION( "value      == engaged"    ) { EXPECT(  6 == e1 ); }
    SECTION( "engaged    != value"      ) { EXPECT( e1 != 7  ); }
    SECTION( "value      != engaged"    ) { EXPECT(  6 != e2 ); }
#if nsel_P0323R <= 2
    SECTION( "engaged    <  value"      ) { EXPECT( e1 <  7  ); }
    SECTION( "value      <  engaged"    ) { EXPECT(  6 <  e2 ); }
    SECTION( "engaged    >  value"      ) { EXPECT( e2 >  6  ); }
    SECTION( "value      >  engaged"    ) { EXPECT(  7 >  e1 ); }
    SECTION( "engaged    <= value"      ) { EXPECT( e1 <= 7  ); }
    SECTION( "value      <= engaged"    ) { EXPECT(  6 <= e2 ); }
    SECTION( "engaged    >= value"      ) { EXPECT( e2 >= 6  ); }
    SECTION( "value      >= engaged"    ) { EXPECT(  7 >= e1 ); }
#endif

    // compare engaged expected with disengaged expected

    SECTION( "engaged    == disengaged" ) { EXPECT_NOT( e1 == d  ); }
    SECTION( "disengaged == engaged"    ) { EXPECT_NOT( d  == e1 ); }
    SECTION( "engaged    != disengaged" ) { EXPECT    ( e1 != d  ); }
    SECTION( "disengaged != engaged"    ) { EXPECT    ( d  != e2 ); }
#if nsel_P0323R <= 2
    SECTION( "engaged    <  disengaged" ) { EXPECT_NOT( e1 <  d  ); }
    SECTION( "disengaged <  engaged"    ) { EXPECT    ( d  <  e2 ); }
    SECTION( "engaged    >  disengaged" ) { EXPECT    ( e2 >  d  ); }
    SECTION( "disengaged >  engaged"    ) { EXPECT_NOT( d  >  e1 ); }
    SECTION( "engaged    <= disengaged" ) { EXPECT_NOT( e1 <= d  ); }
    SECTION( "disengaged <= engaged"    ) { EXPECT    ( d  <= e2 ); }
    SECTION( "engaged    >= disengaged" ) { EXPECT    ( e2 >= d  ); }
    SECTION( "disengaged >= engaged"    ) { EXPECT_NOT( d  >= e1 ); }
#endif

    // compare engaged expected with unexpected

    SECTION( "disengaged == unexpected" ) { EXPECT    ( d  == u  ); }
    SECTION( "unexpected == disengaged" ) { EXPECT    ( u  == d  ); }
    SECTION( "engaged    != unexpected" ) { EXPECT    ( e1 != u  ); }
    SECTION( "unexpected != engaged"    ) { EXPECT    ( u  != e1 ); }
#if nsel_P0323R <= 2
    SECTION( "disengaged <  unexpected" ) { EXPECT_NOT( d  <  u  ); }
    SECTION( "unexpected <  disengaged" ) { EXPECT_NOT( u  <  d  ); }
    SECTION( "disengaged <= unexpected" ) { EXPECT    ( d  <= u  ); }
    SECTION( "unexpected <= disengaged" ) { EXPECT    ( u  <= d  ); }
    SECTION( "disengaged >  unexpected" ) { EXPECT_NOT( d  >  u  ); }
    SECTION( "unexpected >  disengaged" ) { EXPECT_NOT( u  >  d  ); }
    SECTION( "disengaged >= unexpected" ) { EXPECT    ( d  >= u  ); }
    SECTION( "unexpected >= disengaged" ) { EXPECT    ( u  >= d  ); }
#endif

    }
}

// -----------------------------------------------------------------------
// expected: specialized algorithms

// -----------------------------------------------------------------------
// Other

#if nsel_P0323R <= 3

#include <memory>

void vfoo() {}

expected<int> foo()
{
    return make_expected( 7 );
}

expected<std::unique_ptr<int>> bar()
{
    return make_expected( std::unique_ptr<int>( new int(7) ) );
}

#endif // nsel_P0323R

CASE( "make_expected_from_call(): ..." "[.deprecated]" )
{
#if nsel_P0323R <= 3
    expected<int> ei = foo();
    expected<std::unique_ptr<int>> eup = bar();

    auto ev   = make_expected_from_call( vfoo );
    auto e2   = make_expected_from_call( foo );
    auto eup2 = make_expected_from_call( bar );
#else
    EXPECT( !!"make_expected_from_call() is not available (nsel_P0323R > 3)" );
#endif
}

// -----------------------------------------------------------------------
// expected: issues

// issue #15, https://github.com/martinmoene/expected-dark/issues/15

CASE( "issue-15d" )
{
    nonstd::expected< int, std::error_code > e = 12;
    (void)e.value();
}

// issue #15, https://github.com/martinmoene/expected-lite/issues/15

CASE( "issue-15" )
{
    (void) nonstd::expected< int, int >( 12).value();
}

// -----------------------------------------------------------------------
//  using as optional

#if 1

/// disengaged optional state tag

struct nullopt_t{};

const nullopt_t nullopt{};

/// optional expressed in expected

template< typename T >
using optional = expected<T, nullopt_t>;

#endif
