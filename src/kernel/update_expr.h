/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include "kernel/expr.h"

namespace lean {
/**
   \brief Return an application equal to \c app but argument \c i has been replaced with \c new_arg.

   \remark Return \c app if <tt>is_eqp(new_arg, app[i])</tt>.
*/
expr update_app(expr const & app, unsigned i, expr const & new_arg);
expr update_app(expr const & app, unsigned num_args, expr const * new_args);
inline expr update_app(expr const & app, buffer<expr> const & new_args) { return update_app(app, new_args.size(), new_args.data()); }
/**
   \brief Return a lambda expression based on \c lambda with domain \c d and \c body b.

   \remark Return \c lambda if the given domain and body are (pointer) equal to the ones in \c lambda.
*/
expr update_lambda(expr const & lambda, expr const & d, expr const & b);
/**
   \brief Return a Pi expression based on \c pi with domain \c d and \c body b.

   \remark Return \c pi if the given domain and body are (pointer) equal to the ones in \c pi.
*/
expr update_pi(expr const & pi, expr const & d, expr const & b);
/**
   \brief Return a Sigma expression based on \c sig with domain \c d and \c body b.

   \remark Return \c sig if the given domain and body are (pointer) equal to the ones in \c sig.
*/
expr update_sigma(expr const & sig, expr const & d, expr const & b);
/**
   \brief Return a lambda/pi expression based on \c abst with domain \c d and \c body b.
*/
expr update_abstraction(expr const & abst, expr const & d, expr const & b);
/**
   \brief Return a let expression based on \c let with type \c t value \c v and \c body b.

   \remark Return \c let if the given value and body are (pointer) equal to the ones in \c let.
*/
expr update_let(expr const & let, optional<expr> const & t, expr const & v, expr const & b);
/**
   \brief Return a new equality with lhs \c l and rhs \c r.

   \remark Return \c eq if the given lhs and rhs are (pointer) equal to the ones in \c eq.
*/
expr update_heq(expr const & eq, expr const & l, expr const & r);
}
