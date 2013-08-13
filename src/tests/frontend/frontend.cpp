/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include "frontend.h"
#include "environment.h"
#include "operator_info.h"
#include "test.h"
using namespace lean;

static void tst1() {
    frontend f;
    f.add_uvar("tst");
    frontend c = f.mk_child();
    lean_assert(c.get_uvar("tst") == f.get_uvar("tst"));
    lean_assert(f.env().has_children());
}

static void tst2() {
    operator_info op1 = infixl(name("and"), 10);
    operator_info op2 = infixr(name("implies"), 20);
    lean_assert(op1.get_precedence() == 10);
    lean_assert(op1.get_associativity() == associativity::Left);
    lean_assert(op1.get_fixity() == fixity::Infix);
    op1.add_internal_name(name{"Bool","And"});
    operator_info op3 = infixl(name("+"), 10);
    op3.add_internal_name(name{"Int", "plus"});
    op3.add_internal_name(name{"Real", "plus"});
    std::cout << op3.get_internal_names() << "\n";
    lean_assert(length(op3.get_internal_names()) == 2);
    operator_info op4 = op3.copy();
    op4.add_internal_name(name{"Complex", "plus"});
    std::cout << op4.get_internal_names() << "\n";
    lean_assert(length(op3.get_internal_names()) == 2);
    lean_assert(length(op4.get_internal_names()) == 3);
    lean_assert(op4.get_fixity() == fixity::Infix);
    lean_assert(op4.get_op_name() == op3.get_op_name());
    lean_assert(prefix("tst", 20).get_fixity() == fixity::Prefix);
    lean_assert(postfix("!", 20).get_fixity() == fixity::Postfix);
    lean_assert(length(mixfixl({"if", "then", "else"}, 10).get_op_name_parts()) == 3);
    lean_assert(mixfixc({"if", "then", "else", "endif"}, 10).get_fixity() == fixity::Mixfixc);
    std::cout << mixfixc({"if", "then", "else", "endif"}, 10).get_op_name_parts() << "\n";
    std::cout << op4 << "\n";
    std::cout << op2 << "\n";
    std::cout << mixfixc({"if", "then", "else", "endif"}, 10) << "\n";
}

int main() {
    continue_on_violation(true);
    tst1();
    tst2();
    return has_violations() ? 1 : 0;
}