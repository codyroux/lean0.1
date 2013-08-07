/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include "expr.h"
#include "sets.h"

namespace lean {
template<typename F>
class for_each_fn {
    expr_cell_offset_set m_visited;
    F                    m_f;

    void apply(expr const & e, unsigned offset) {
        if (is_shared(e)) {
            expr_cell_offset p(e.raw(), offset);
            if (m_visited.find(p) != m_visited.end())
                return;
            m_visited.insert(p);
        }

       m_f(e, offset);

        switch (e.kind()) {
        case expr_kind::Constant: case expr_kind::Type: case expr_kind::Value: case expr_kind::Var:
            return;
        case expr_kind::App:
            std::for_each(begin_args(e), end_args(e), [=](expr const & arg){ return apply(arg, offset); });
            return;
        case expr_kind::Eq:
            apply(eq_lhs(e), offset);
            apply(eq_rhs(e), offset);
            return;
        case expr_kind::Lambda:
        case expr_kind::Pi:
            apply(abst_domain(e), offset);
            apply(abst_body(e), offset + 1);
            return;
        case expr_kind::Let:
            apply(let_value(e), offset);
            apply(let_body(e), offset + 1);
            return;
        }
    }
public:
    for_each_fn(F const & f):m_f(f) {}
    void operator()(expr const & e) { apply(e, 0); }
};
}