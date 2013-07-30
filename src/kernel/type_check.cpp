/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include <sstream>
#include "type_check.h"
#include "normalize.h"
#include "free_vars.h"
#include "exception.h"
#include "trace.h"

namespace lean {
class infer_type_fn {
    environment const & m_env;

    expr normalize(expr const & e, context const & ctx) {
        return ::lean::normalize(e, m_env, ctx);
    }

    expr lookup(context const & c, unsigned i) {
        context const & def_c = ::lean::lookup(c, i);
        return lift_free_vars(head(def_c).get_type(), length(c) - length(def_c));
    }

    level infer_universe(expr const & t, context const & ctx) {
        lean_trace("type_check", tout << "infer universe\n" << t << "\n";);
        expr u = normalize(infer_type(t, ctx), ctx);
        if (is_type(u))
            return ty_level(u);
        std::ostringstream buffer;
        buffer << "type expected, in context:\n" << ctx << "\ngiven:\n" << t;
        throw exception(buffer.str());
    }

    expr infer_pi(expr const & e, context const & ctx) {
        lean_trace("type_check", tout << "infer pi\n" << e << "\n";);
        expr p = normalize(infer_type(e, ctx), ctx);
        if (is_pi(p))
            return p;
        std::ostringstream buffer;
        buffer << "function expected, in context:\n" << ctx << "\ngiven:\n" << e;
        throw exception(buffer.str());
    }

    bool check_type_core(expr const & expected, expr const & given) {
        if (expected == given)
            return true;
        if (is_type(expected) && is_type(given)) {
            if (m_env.is_ge(ty_level(expected), ty_level(given)))
                return true;
        }
        return false;
    }

    void check_type(expr const & e, unsigned i, expr const & expected, expr const & given, context const & ctx) {
        if (check_type_core(expected, given))
            return;
        expr e_n = normalize(expected, ctx);
        expr g_n = normalize(given, ctx);
        if (check_type_core(e_n, g_n))
            return;
        std::ostringstream buffer;
        buffer << "type mismatch at argument " << i << " of\n" << e
               << "\nexpected type:\n" << expected
               << "\ngiven type:\n" << given << "\nin context:\n" << ctx;;
        throw exception(buffer.str());
    }

    expr infer_type(expr const & e, context const & ctx) {
        lean_trace("type_check", tout << "infer type\n" << e << "\n" << ctx << "\n";);
        switch (e.kind()) {
        case expr_kind::Constant:
            // TODO
            return e;
        case expr_kind::Var:      return lookup(ctx, var_idx(e));
        case expr_kind::Type:     return type(ty_level(e) + 1);
        case expr_kind::App: {
            expr f_t     = infer_pi(arg(e, 0), ctx);
            unsigned i   = 1;
            unsigned num = num_args(e);
            lean_assert(num >= 2);
            while (true) {
                expr const & c = arg(e, i);
                expr c_t       = infer_type(c, ctx);
                check_type(e, i, abst_type(f_t), c_t, ctx);
                f_t = normalize(abst_body(f_t), extend(ctx, abst_name(f_t), c_t));
                i++;
                if (i == num)
                    return f_t;
                if (!is_pi(f_t))
                    throw exception("function expected");
            }
        }
        case expr_kind::Lambda: {
            infer_universe(abst_type(e), ctx);
            expr t = infer_type(abst_body(e), extend(ctx, abst_name(e), abst_type(e)));
            return pi(abst_name(e), abst_type(e), t);
        }
        case expr_kind::Pi: {
            level l1 = infer_universe(abst_type(e), ctx);
            level l2 = infer_universe(abst_body(e), extend(ctx, abst_name(e), abst_type(e)));
            return type(max(l1, l2));
        }
        case expr_kind::Numeral:
            // TODO
            return e;
        }
        lean_unreachable();
        return e;
    }
public:
    infer_type_fn(environment const & env):
        m_env(env) {
    }

    expr operator()(expr const & e, context const & ctx) {
        return infer_type(e, ctx);
    }
};

expr  infer_type(expr const & e, environment const & env, context const & ctx) {
    return infer_type_fn(env)(e, ctx);
}
}