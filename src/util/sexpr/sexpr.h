/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include <iostream>

namespace lean {
class name;
class mpq;
class mpz;
struct sexpr_cell;

enum class sexpr_kind { NIL, STRING, BOOL, INT, DOUBLE, NAME, MPZ, MPQ, CONS };

/**
   \brief Simple LISP-like S-expressions.
   1. Atoms: nil, string, int, name, mpz or mpq
   2. Pair: (x . y) where x and y are S-expressions.

   S-expressions are used for tracking configuration options, statistics, etc.

   Remark: this datastructure does not allow destructive updates.
*/
class sexpr {
    sexpr_cell * m_ptr;
public:
    sexpr():m_ptr(nullptr) {}
    explicit sexpr(char const * v);
    explicit sexpr(std::string const & v);
    explicit sexpr(bool v);
    explicit sexpr(int v);
    explicit sexpr(double v);
    explicit sexpr(name const & v);
    explicit sexpr(mpz const & v);
    explicit sexpr(mpq const & v);
    sexpr(sexpr const & h, sexpr const & t);
    template<typename T>
    sexpr(T const & h, sexpr const & t):sexpr(sexpr(h), t) {}
    template<typename T1, typename T2>
    sexpr(T1 const & h, T2 const & t):sexpr(sexpr(h), sexpr(t)) {}
    sexpr(sexpr const & s);
    sexpr(sexpr && s);
    template<typename T>
    sexpr(std::initializer_list<T> const & l):sexpr() {
        auto it = l.end();
        while (it != l.begin()) {
            --it;
            *this = sexpr(*it, *this);
        }
    }
    ~sexpr();

    sexpr_kind kind() const;

    friend bool is_nil(sexpr const & s) { return s.m_ptr == nullptr; }
    friend sexpr const & head(sexpr const & s);
    friend sexpr const & tail(sexpr const & s);

    std::string const & get_string() const;
    int get_int() const;
    bool get_bool() const;
    double get_double() const;
    name const & get_name() const;
    mpz const & get_mpz() const;
    mpq const & get_mpq() const;

    /** \brief Hash code for this S-expression*/
    unsigned hash() const;

    sexpr & operator=(sexpr const & s);
    sexpr & operator=(sexpr&& s);
    template<typename T>
    sexpr & operator=(T const & v) { return operator=(sexpr(v)); }

    friend void swap(sexpr & a, sexpr & b) { std::swap(a.m_ptr, b.m_ptr); }

    /** \brief Pointer equality */
    friend bool is_eqp(sexpr const & a, sexpr const & b) { return a.m_ptr == b.m_ptr; }

    friend std::ostream & operator<<(std::ostream & out, sexpr const & s);

};

/** \brief Return the nil S-expression */
inline sexpr nil() { return sexpr(); }
/** \brief Return a cons-cell (aka pair) composed of \c head and \c tail */
inline sexpr cons(sexpr const & head, sexpr const & tail) { return sexpr(head, tail); }
/**
    \brief Return the first argument of the given cons cell (aka pair).
    \pre is_cons(s)
*/
inline sexpr const & car(sexpr const & s) { return head(s); }
/**
    \brief Return the second argument of the given cons cell (aka pair).
    \pre is_cons(s)
*/
inline sexpr const & cdr(sexpr const & s) { return tail(s); }
/** \brief Return true iff \c s is not an atom (i.e., it is not a cons cell). */
inline bool is_atom(sexpr const & s)   { return s.kind() != sexpr_kind::CONS; }
/** \brief Return true iff \c s is not a cons cell. */
inline bool is_cons(sexpr const & s)   { return s.kind() == sexpr_kind::CONS; }
inline bool is_string(sexpr const & s) { return s.kind() == sexpr_kind::STRING; }
inline bool is_bool(sexpr const & s)   { return s.kind() == sexpr_kind::BOOL; }
inline bool is_int(sexpr const & s)    { return s.kind() == sexpr_kind::INT; }
inline bool is_double(sexpr const & s) { return s.kind() == sexpr_kind::DOUBLE; }
inline bool is_name(sexpr const & s)   { return s.kind() == sexpr_kind::NAME; }
inline bool is_mpz(sexpr const & s)    { return s.kind() == sexpr_kind::MPZ; }
inline bool is_mpq(sexpr const & s)    { return s.kind() == sexpr_kind::MPQ; }

inline std::string const & to_string(sexpr const & s) { return s.get_string(); }
inline bool to_bool(sexpr const & s) { return s.get_bool(); }
inline int to_int(sexpr const & s) { return s.get_int(); }
inline double to_double(sexpr const & s) { return s.get_double(); }
inline name const & to_name(sexpr const & s) { return s.get_name(); }
inline mpz const & to_mpz(sexpr const & s) { return s.get_mpz(); }
inline mpq const & to_mpq(sexpr const & s) { return s.get_mpq(); }

/** \brief Return true iff \c s is nil or \c s is a cons cell where \c is_list(tail(s)). */
bool is_list(sexpr const & s);
/**
    \brief Return the length of the given list.
    \pre is_list(s)
*/
unsigned length(sexpr const & s);
/** \brief Alias for #length. */
inline unsigned len(sexpr const & s) { return length(s); }

/** \brief Return true iff the two given S-expressions are structurally identical.
    \warning This is not pointer equality.
*/
bool operator==(sexpr const & a, sexpr const & b);
inline bool operator==(sexpr const & a, bool b) { return is_int(a) && to_bool(a) == b; }
inline bool operator==(sexpr const & a, int b) { return is_int(a) && to_int(a) == b; }
inline bool operator==(sexpr const & a, double b) { return is_double(a) && to_double(a) == b; }
inline bool operator==(sexpr const & a, std::string const & b) { return is_string(a) && to_string(a) == b; }
inline bool operator==(sexpr const & a, char const * b) { return is_string(a) && to_string(a) == b; }
bool operator==(sexpr const & a, name const & b);
bool operator==(sexpr const & a, mpz const & b);
bool operator==(sexpr const & a, mpq const & b);
template<typename T> inline bool operator==(T const & a, sexpr const & b) { return b == a; }
inline bool operator!=(sexpr const & a, sexpr const & b) { return !(a == b); }
template<typename T> inline bool operator!=(sexpr const & a, T const & b) { return !(a == b); }
template<typename T> inline bool operator!=(T const & a, sexpr const & b) { return !(a == b); }
int cmp(sexpr const & a, sexpr const & b);
inline bool operator<(sexpr const & a, sexpr const & b) { return cmp(a, b) < 0; }
inline bool operator>(sexpr const & a, sexpr const & b) { return cmp(a, b) > 0; }
inline bool operator<=(sexpr const & a, sexpr const & b) { return cmp(a, b) <= 0; }
inline bool operator>=(sexpr const & a, sexpr const & b) { return cmp(a, b) >= 0; }

}