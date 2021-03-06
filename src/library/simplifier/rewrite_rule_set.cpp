/*
Copyright (c) 2014 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include "util/splay_tree.h"
#include "util/list_fn.h"
#include "util/sstream.h"
#include "kernel/environment.h"
#include "library/io_state_stream.h"
#include "library/equality.h"
#include "library/kernel_bindings.h"
#include "library/simplifier/ceq.h"
#include "library/simplifier/rewrite_rule_set.h"

namespace lean {
rewrite_rule::rewrite_rule(name const & id, expr const & lhs, expr const & rhs, expr const & ceq, expr const & proof,
                           unsigned num_args, bool is_permutation, bool must_check):
    m_id(id), m_lhs(lhs), m_rhs(rhs), m_ceq(ceq), m_proof(proof), m_num_args(num_args),
    m_is_permutation(is_permutation), m_must_check_types(must_check) {
}

rewrite_rule_set::rewrite_rule_set(ro_environment const & env):m_env(env.to_weak_ref()) {}
rewrite_rule_set::rewrite_rule_set(rewrite_rule_set const & other):
    m_env(other.m_env), m_rule_set(other.m_rule_set), m_disabled_rules(other.m_disabled_rules), m_congr_thms(other.m_congr_thms) {}
rewrite_rule_set::~rewrite_rule_set() {}

void rewrite_rule_set::insert(name const & id, expr const & th, expr const & proof, optional<ro_metavar_env> const & menv) {
    ro_environment env(m_env);
    for (auto const & p : to_ceqs(env, menv, th, proof)) {
        expr const & ceq   = p.first;
        expr const & proof = p.second;
        bool is_perm       = is_permutation_ceq(ceq);
        expr eq = ceq;
        unsigned num = 0;
        while (is_pi(eq)) {
            eq = abst_body(eq);
            num++;
        }
        lean_assert(is_equality(eq));
        bool must_check = !is_safe_to_skip_check_ceq_types(m_env, menv, ceq);
        m_rule_set = cons(rewrite_rule(id, arg(eq, num_args(eq) - 2), arg(eq, num_args(eq) - 1),
                                       ceq, proof, num, is_perm, must_check),
                          m_rule_set);
    }
}

void rewrite_rule_set::insert(name const & th_name) {
    ro_environment env(m_env);
    auto obj = env->find_object(th_name);
    if (obj && (obj->is_theorem() || obj->is_axiom())) {
        insert(th_name, obj->get_type(), mk_constant(th_name), none_ro_menv());
    } else {
        throw exception(sstream() << "'" << th_name << "' is not a theorem nor an axiom");
    }
}

bool rewrite_rule_set::enabled(rewrite_rule const & rule) const {
    return !m_disabled_rules.contains(rule.get_id());
}

bool rewrite_rule_set::enabled(name const & id) const {
    return !m_disabled_rules.contains(id);
}

void rewrite_rule_set::enable(name const & id, bool f) {
    if (f)
        m_disabled_rules.erase(id);
    else
        m_disabled_rules.insert(id);
}

void rewrite_rule_set::insert_congr(expr const & e) {
    ro_environment env(m_env);
    m_congr_thms.emplace_front(check_congr_theorem(env, e));
}

void rewrite_rule_set::insert_congr(name const & th_name) {
    insert_congr(mk_constant(th_name));
}

bool rewrite_rule_set::find_match(expr const &, match_fn const & fn) const {
    auto l = m_rule_set;
    for (auto const & rule : l) {
        if (enabled(rule) && fn(rule))
            return true;
    }
    return false;
}

void rewrite_rule_set::for_each(visit_fn const & fn) const {
    auto l = m_rule_set;
    for (auto const & rule : l) {
        fn(rule, enabled(rule));
    }
}

void rewrite_rule_set::for_each_congr(visit_congr_fn const & fn) const {
    for (auto const & congr_th : m_congr_thms) {
        fn(congr_th);
    }
}

format rewrite_rule_set::pp(formatter const & fmt, options const & opts) const {
    format r;
    bool first = true;
    unsigned indent = get_pp_indent(opts);
    for_each([&](rewrite_rule const & rule, bool enabled) {
            if (first)
                first = false;
            else
                r += line();
            r += format(rule.get_id());
            if (!enabled)
                r += format(" [disabled]");
            if (rule.must_check_types())
                r += format(" [check]");
            r += format{space(), colon(), space()};
            r += nest(indent, fmt(rule.get_ceq(), opts));
        });
    return r;
}

io_state_stream const & operator<<(io_state_stream const & out, rewrite_rule_set const & rs) {
    out << mk_pair(rs.pp(out.get_formatter(), out.get_options()), out.get_options());
    return out;
}

class mk_rewrite_rule_set_obj : public neutral_object_cell {
    name m_rule_set_id;
public:
    mk_rewrite_rule_set_obj(name const & id):m_rule_set_id(id) {}
    virtual ~mk_rewrite_rule_set_obj() {}
    virtual char const * keyword() const { return "mk_rewrite_rule_set"; }
    virtual void write(serializer & s) const { s << "mk_rrs" << m_rule_set_id; }
};
static void read_rrs(environment const & env, io_state const &, deserializer & d) {
    name n = read_name(d);
    mk_rewrite_rule_set(env, n);
}
static object_cell::register_deserializer_fn mk_rrs_ds("mk_rrs", read_rrs);

class add_rewrite_rules_obj : public neutral_object_cell {
    name m_rule_set_id;
    name m_th_name;
public:
    add_rewrite_rules_obj(name const & rsid, name const & th_name):m_rule_set_id(rsid), m_th_name(th_name) {}
    virtual ~add_rewrite_rules_obj() {}
    virtual char const * keyword() const { return "add_rewrite_rules"; }
    virtual void write(serializer & s) const { s << "add_rr" << m_rule_set_id << m_th_name; }
};
static void read_arr(environment const & env, io_state const &, deserializer & d) {
    name rsid = read_name(d);
    name th   = read_name(d);
    add_rewrite_rules(env, rsid, th);
}
static object_cell::register_deserializer_fn add_rr_ds("add_rr", read_arr);

class enable_rewrite_rules_obj : public neutral_object_cell {
    name m_rule_set_id;
    name m_rule_id;
    bool m_flag;
public:
    enable_rewrite_rules_obj(name const & rsid, name const & id, bool flag):m_rule_set_id(rsid), m_rule_id(id), m_flag(flag) {}
    virtual ~enable_rewrite_rules_obj() {}
    virtual char const * keyword() const { return "enable_rewrite_rules_obj"; }
    virtual void write(serializer & s) const { s << "enable_rr" << m_rule_set_id << m_rule_id << m_flag; }
};
static void read_enable_rr(environment const & env, io_state const &, deserializer & d) {
    name rsid = read_name(d);
    name id   = read_name(d);
    bool flag = d.read_bool();
    enable_rewrite_rules(env, rsid, id, flag);
}
static object_cell::register_deserializer_fn enable_rr_ds("enable_rr", read_enable_rr);

class add_congr_theorem_obj : public neutral_object_cell {
    name m_rule_set_id;
    name m_th_name;
public:
    add_congr_theorem_obj(name const & rsid, name const & th_name):m_rule_set_id(rsid), m_th_name(th_name) {}
    virtual ~add_congr_theorem_obj() {}
    virtual char const * keyword() const { return "add_congr_theorem"; }
    virtual void write(serializer & s) const { s << "add_ct" << m_rule_set_id << m_th_name; }
};
static void read_ct(environment const & env, io_state const &, deserializer & d) {
    name rsid = read_name(d);
    name th   = read_name(d);
    add_congr_theorem(env, rsid, th);
}
static object_cell::register_deserializer_fn add_ct_ds("add_ct", read_ct);

/**
   \brief Extension for managing rewrite rule sets.
*/
struct rewrite_rule_set_extension : public environment_extension {
    name_map<rewrite_rule_set> m_rule_sets;

    rewrite_rule_set_extension const * get_parent() const {
        return environment_extension::get_parent<rewrite_rule_set_extension>();
    }

    rewrite_rule_set const * find_ro_core(name const & rule_set_id) const {
        auto it = m_rule_sets.find(rule_set_id);
        if (it != m_rule_sets.end()) {
            return &(it->second);
        }
        auto p = get_parent();
        if (p) {
            return p->find_ro_core(rule_set_id);
        } else {
            return nullptr;
        }
    }

    rewrite_rule_set const & find_ro(name const & rule_set_id) const {
        auto rs = find_ro_core(rule_set_id);
        if (rs)
            return *rs;
        throw exception(sstream() << "environment does not contain a rewrite rule set named '" << rule_set_id << "'");
    }

    rewrite_rule_set & find_rw(name const & rule_set_id) {
        auto it = m_rule_sets.find(rule_set_id);
        if (it != m_rule_sets.end())
            return it->second;
        auto p = get_parent();
        if (p) {
            auto const & rs = p->find_ro(rule_set_id);
            m_rule_sets.insert(mk_pair(rule_set_id, rewrite_rule_set(rs)));
            return m_rule_sets.find(rule_set_id)->second;
        }
        throw exception(sstream() << "environment does not contain a rewrite rule set named '" << rule_set_id << "'");
    }

    void mk_rewrite_rule_set(environment const & env, name const & rule_set_id) {
        if (find_ro_core(rule_set_id))
            throw exception(sstream() << "environment already contains a rewrite rule set named '" << rule_set_id << "'");
        m_rule_sets.insert(mk_pair(rule_set_id, rewrite_rule_set(env)));
        env->add_neutral_object(new mk_rewrite_rule_set_obj(rule_set_id));
    }

    void add_rewrite_rules(environment const & env, name const & rule_set_id, name const & th_name) {
        auto & rs = find_rw(rule_set_id);
        rs.insert(th_name);
        env->add_neutral_object(new add_rewrite_rules_obj(rule_set_id, th_name));
    }

    void enable_rewrite_rules(environment const & env, name const & rule_set_id, name const & rule_id, bool flag) {
        auto & rs = find_rw(rule_set_id);
        rs.enable(rule_id, flag);
        env->add_neutral_object(new enable_rewrite_rules_obj(rule_set_id, rule_id, flag));
    }

    void add_congr_theorem(environment const & env, name const & rule_set_id, name const & th_name) {
        auto & rs = find_rw(rule_set_id);
        rs.insert_congr(th_name);
        env->add_neutral_object(new add_congr_theorem_obj(rule_set_id, th_name));
    }

    rewrite_rule_set get_rewrite_rule_set(name const & rule_set_id) const {
        return find_ro(rule_set_id);
    }
};

struct rewrite_rule_set_extension_initializer {
    unsigned m_extid;
    rewrite_rule_set_extension_initializer() {
        m_extid = environment_cell::register_extension([](){
                return std::unique_ptr<environment_extension>(new rewrite_rule_set_extension());
            });
    }
};

static rewrite_rule_set_extension_initializer g_rewrite_rule_set_extension_initializer;

static rewrite_rule_set_extension const & to_ext(ro_environment const & env) {
    return env->get_extension<rewrite_rule_set_extension>(g_rewrite_rule_set_extension_initializer.m_extid);
}

static rewrite_rule_set_extension & to_ext(environment const & env) {
    return env->get_extension<rewrite_rule_set_extension>(g_rewrite_rule_set_extension_initializer.m_extid);
}

static name g_default_rewrite_rule_set_id("default");
name const & get_default_rewrite_rule_set_id() {
    return g_default_rewrite_rule_set_id;
}

void mk_rewrite_rule_set(environment const & env, name const & rule_set_id) {
    to_ext(env).mk_rewrite_rule_set(env, rule_set_id);
}

void add_rewrite_rules(environment const & env, name const & rule_set_id, name const & th_name) {
    to_ext(env).add_rewrite_rules(env, rule_set_id, th_name);
}

void enable_rewrite_rules(environment const & env, name const & rule_set_id, name const & rule_id, bool flag) {
    to_ext(env).enable_rewrite_rules(env, rule_set_id, rule_id, flag);
}

void add_congr_theorem(environment const & env, name const & rule_set_id, name const & th_name) {
    to_ext(env).add_congr_theorem(env, rule_set_id, th_name);
}

rewrite_rule_set get_rewrite_rule_set(ro_environment const & env, name const & rule_set_id) {
    return to_ext(env).get_rewrite_rule_set(rule_set_id);
}

static int mk_rewrite_rule_set(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 0)
        mk_rewrite_rule_set(rw_shared_environment(L));
    else if (nargs == 1)
        mk_rewrite_rule_set(rw_shared_environment(L), to_name_ext(L, 1));
    else
        mk_rewrite_rule_set(rw_shared_environment(L, 2), to_name_ext(L, 1));
    return 0;
}

static int add_rewrite_rules(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 1)
        add_rewrite_rules(rw_shared_environment(L), to_name_ext(L, 1));
    else if (nargs == 2)
        add_rewrite_rules(rw_shared_environment(L), to_name_ext(L, 1), to_name_ext(L, 2));
    else
        add_rewrite_rules(rw_shared_environment(L, 3), to_name_ext(L, 1), to_name_ext(L, 2));
    return 0;
}

static int enable_rewrite_rules(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 2)
        enable_rewrite_rules(rw_shared_environment(L), to_name_ext(L, 1), lua_toboolean(L, 2));
    else if (nargs == 3)
        enable_rewrite_rules(rw_shared_environment(L), to_name_ext(L, 1), to_name_ext(L, 2), lua_toboolean(L, 3));
    else
        enable_rewrite_rules(rw_shared_environment(L, 4), to_name_ext(L, 1), to_name_ext(L, 2), lua_toboolean(L, 3));
    return 0;
}

static int add_congr_theorem(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 1)
        add_congr_theorem(rw_shared_environment(L), to_name_ext(L, 1));
    else if (nargs == 2)
        add_congr_theorem(rw_shared_environment(L), to_name_ext(L, 1), to_name_ext(L, 2));
    else
        add_congr_theorem(rw_shared_environment(L, 3), to_name_ext(L, 1), to_name_ext(L, 2));
    return 0;
}

static int show_rewrite_rules(lua_State * L) {
    int nargs = lua_gettop(L);
    formatter fmt  = get_global_formatter(L);
    options   opts = get_global_options(L);
    format r;
    if (nargs == 0)
        r = get_rewrite_rule_set(ro_shared_environment(L)).pp(fmt, opts);
    else if (nargs == 1)
        r = get_rewrite_rule_set(ro_shared_environment(L), to_name_ext(L, 1)).pp(fmt, opts);
    else
        r = get_rewrite_rule_set(ro_shared_environment(L, 2), to_name_ext(L, 1)).pp(fmt, opts);
    io_state * ios = get_io_state(L);
    if (ios)
        regular(*ios) << mk_pair(r, opts) << endl;
    else
        std::cout << mk_pair(r, opts) << "\n";
    return 0;
}

void open_rewrite_rule_set(lua_State * L) {
    SET_GLOBAL_FUN(mk_rewrite_rule_set,  "mk_rewrite_rule_set");
    SET_GLOBAL_FUN(add_rewrite_rules,    "add_rewrite_rules");
    SET_GLOBAL_FUN(enable_rewrite_rules, "enable_rewrite_rules");
    SET_GLOBAL_FUN(add_congr_theorem,    "add_congr_theorem");
    SET_GLOBAL_FUN(show_rewrite_rules,   "show_rewrite_rules");
}
}
