/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include <initializer_list>
namespace lean {
static char g_safe_ascii[256];
/**
   \brief Initializer for the \c g_safe_ascii table of "safe" ASCII characters.
*/
class init_safe_ascii {
    void set(int i, bool v) { g_safe_ascii[static_cast<unsigned char>(i)] = v; }
public:
    init_safe_ascii() {
        for (int i = 0; i <= 255; i++)
            set(i, false);
        // digits and characters are safe
        for (int i = '0'; i <= '9'; i++) set(i, true);
        for (int i = 'a'; i <= 'z'; i++) set(i, true);
        for (int i = 'A'; i <= 'Z'; i++) set(i, true);
        // the following characters are also safe
        for (unsigned char b : {'_', ' ', '\t', '\r', '\n', '(', ')', '{', '}', ':', '.', ',', '\"', '\'', '`', '!', '#',
                    '=', '<', '>', '@', '^', '|', '&', '~', '+', '-', '*', '/', '\\', '$', '%', '?', ';', '[', ']'})
            set(b, true);
    }
};

static init_safe_ascii g_init_safe_ascii;

bool is_safe_ascii(char c) {
    return g_safe_ascii[static_cast<unsigned char>(c)];
}

bool is_safe_ascii(char const * str) {
    if (str) {
        while (*str != 0) {
            if (!is_safe_ascii(*str))
                return false;
            str++;
        }
    }
    return true;
}
}
