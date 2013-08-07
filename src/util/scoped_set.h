/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include <unordered_set>
#include <vector>
#include "debug.h"

#ifndef LEAN_SCOPED_SET_INITIAL_BUCKET_SIZE
#define LEAN_SCOPED_SET_INITIAL_BUCKET_SIZE 8
#endif

namespace lean {
/**
   \brief Scoped sets (aka backtrackable sets).
*/
template<typename Key, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
class scoped_set {
    typedef std::unordered_set<Key, Hash, KeyEqual> set;
    typedef typename set::size_type                 size_type;
    enum class action_kind { Insert, Erase };
    set                                      m_set;
    std::vector<std::pair<action_kind, Key>> m_actions;
    std::vector<unsigned>                    m_scopes;
public:
    explicit scoped_set(size_type bucket_count = LEAN_SCOPED_SET_INITIAL_BUCKET_SIZE,
                        const Hash& hash = Hash(),
                        const KeyEqual& equal = KeyEqual()):
        m_set(bucket_count, hash, equal) {}

    /** \brief Return the number of scopes. */
    unsigned num_scopes() const {
        return m_scopes.size();
    }

    /** \brief Return true iff there are no scopes. */
    bool at_base_lvl() const {
        return m_scopes.empty();
    }

    /** \brief Create a new scope (it allows us to restore the current state of the set). */
    void push() {
        m_scopes.push_back(m_actions.size());
    }

    /** \brief Remove \c num scopes, and restores the state of the set. */
    void pop(unsigned num = 1) {
        lean_assert(num <= num_scopes());
        unsigned old_sz = m_scopes[num_scopes() - num];
        lean_assert(old_sz <= m_actions.size());
        unsigned i = m_actions.size();
        while (i > old_sz) {
            --i;
            auto const & p = m_actions.back();
            if (p.first == action_kind::Insert) {
                m_set.erase(p.second);
            } else {
                m_set.insert(p.second);
            }
            m_actions.pop_back();
        }
        lean_assert(m_actions.size() == old_sz);
        m_scopes.resize(num_scopes() - num);
    }

    /** \brief Return true iff the set is empty */
    bool empty() const {
        return m_set.empty();
    }

    /** \brief Return the number of elements stored in the set. */
    unsigned size() const {
        return m_set.size();
    }

    /** \brief Insert an element in the set */
    void insert(Key const & k) {
        if (!at_base_lvl()) {
            if (m_set.find(k) == m_set.end())
                m_actions.push_back(std::make_pair(action_kind::Insert, k));
        }
        m_set.insert(k);
    }

    /** \brief Remove an element from the set */
    void erase(Key const & k) {
        if (!at_base_lvl()) {
            if (m_set.find(k) != m_set.end())
                m_actions.push_back(std::make_pair(action_kind::Erase, k));
        }
        m_set.erase(k);
    }

    /** \brief Remove all elements and scopes */
    void clear() {
        m_set.clear();
        m_actions.clear();
        m_scopes.clear();
    }

    typedef typename set::const_iterator const_iterator;
    const_iterator find(Key const & k) const {
        return m_set.find(k);
    }

    const_iterator begin() const {
        return m_set.begin();
    }

    const_iterator end() const {
        return m_set.end();
    }
};
}