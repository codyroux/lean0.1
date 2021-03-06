/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include <utility>
#include "kernel/kernel.h"
#include "library/fo_unify.h"
#include "library/expr_pair.h"
#include "library/kernel_bindings.h"
#include "library/equality.h"

namespace lean {
static void assign(substitution & s, expr const & mvar, expr const & e) {
    lean_assert(is_metavar(mvar));
    s.insert(metavar_name(mvar), e);
}

static bool is_metavar_wo_local_context(expr const & e) {
    return is_metavar(e) && !metavar_lctx(e);
}

optional<substitution> fo_unify(expr e1, expr e2) {
    substitution s;
    unsigned i1, i2;
    buffer<expr_pair> todo;
    todo.emplace_back(e1, e2);
    while (!todo.empty()) {
        auto p = todo.back();
        todo.pop_back();
        e1 = find(s, p.first);
        e2 = find(s, p.second);
        if (e1 != e2) {
            if (is_metavar_wo_local_context(e1)) {
                assign(s, e1, e2);
            } else if (is_metavar_wo_local_context(e2)) {
                assign(s, e2, e1);
            } else if (is_equality(e1) && is_equality(e2)) {
                expr_pair p1 = get_equality_args(e1);
                expr_pair p2 = get_equality_args(e2);
                todo.emplace_back(p1.second, p2.second);
                todo.emplace_back(p1.first,  p2.first);
            } else {
                if (e1.kind() != e2.kind())
                    return optional<substitution>();
                switch (e1.kind()) {
                case expr_kind::Var: case expr_kind::Constant: case expr_kind::Type: case expr_kind::Value: case expr_kind::MetaVar:
                    return optional<substitution>();
                case expr_kind::App:
                    i1 = num_args(e1);
                    i2 = num_args(e2);
                    while (i1 > 0 && i2 > 0) {
                        --i1;
                        --i2;
                        if (i1 == 0 && i2 > 0) {
                            todo.emplace_back(arg(e1, i1), mk_app(i2+1, begin_args(e2)));
                        } else if (i2 == 0 && i1 > 0) {
                            todo.emplace_back(mk_app(i1+1, begin_args(e1)), arg(e2, i2));
                        } else {
                            todo.emplace_back(arg(e1, i1), arg(e2, i2));
                        }
                    }
                    break;
                case expr_kind::HEq:
                    todo.emplace_back(heq_lhs(e1), heq_lhs(e2));
                    todo.emplace_back(heq_rhs(e1), heq_rhs(e2));
                    break;
                case expr_kind::Proj:
                    if (proj_first(e1) != proj_first(e2))
                        return optional<substitution>();
                    todo.emplace_back(proj_arg(e1), proj_arg(e2));
                    break;
                case expr_kind::Pair:
                    todo.emplace_back(pair_first(e1), pair_first(e2));
                    todo.emplace_back(pair_second(e1), pair_second(e2));
                    todo.emplace_back(pair_type(e1), pair_type(e2));
                    break;
                case expr_kind::Lambda: case expr_kind::Pi: case expr_kind::Sigma:
                    todo.emplace_back(abst_body(e1), abst_body(e2));
                    todo.emplace_back(abst_domain(e1), abst_domain(e2));
                    break;
                case expr_kind::Let:
                    todo.emplace_back(let_body(e1), let_body(e2));
                    todo.emplace_back(let_value(e1), let_value(e2));
                    if (static_cast<bool>(let_type(e1)) != static_cast<bool>(let_type(e2)))
                        return optional<substitution>();
                    if (let_type(e1)) {
                        lean_assert(let_type(e2));
                        todo.emplace_back(*let_type(e1), *let_type(e2));
                    }
                    break;
                }
            }
        }
    }
    return optional<substitution>(s);
}


static int fo_unify(lua_State * L) {
    optional<substitution> r = fo_unify(to_expr(L, 1), to_expr(L, 2));
    if (!r) {
        lua_pushnil(L);
        return 1;
    } else {
        return push_substitution(L, *r);
    }
}

void open_fo_unify(lua_State * L) {
    SET_GLOBAL_FUN(fo_unify, "fo_unify");
}
}
