# Lua API documentation

We the [Lua](http://www.lua.org) script language to customize and
extend [Lean](http://leanprover.net). This
[link](http://www.lua.org/docs.html) is a good starting point for
learning Lua. In this document, we focus on the Lean specific APIs.
Most of Lean components are exposed in the Lua API.

**Remark:** the script [md2lua.sh](md2lua.sh) extracts the Lua code
examples from the `*.md` documentation files in this directory.

## Hierarchical names

In Lean, we use _hierarchical names_ for identifying configuration
options, constants, and kernel objects. A hierarchical name is
essentially a list of strings and integers.
The following example demonstrates how to create and manipulate
hierarchical names using the Lua API.

```lua
local x = name("x")    -- create a simple hierarchical name
local y = name("y")
-- In Lua, 'assert(p)' succeeds if 'p' does not evaluate to false (or nil)
assert(x == name("x")) -- test if 'x' is equal to 'name("x")'
assert(x ~= y)         -- '~=' is the not equal operator in Lua
assert(x ~= "x")

assert(is_name(x)) -- test whether argument is a hierarchical name or not.
assert(not is_name("x"))

local x1 = name(x, 1) -- x1 is a name composed of the string "x" and number 1
assert(tostring(x1) == "x::1")
assert(x1 ~= name("x::1"))  -- x1 is not equal to the string x::1
assert(x1 == name("x", 1))

local o = name("lean", "pp", "colors")
-- The previous construct is syntax sugar for the following expression
assert(o == name(name(name("lean"), "pp"), "colors"))

assert(x < y) -- '<' is a total order on hierarchical names

local h = x:hash() -- retrieve the hash code for 'x'
assert(h ~= y:hash())
```

## Lua tables

Tables are the only mutable, non-atomic type of data in Lua.  Tables
are used to implement mappings, arrays, lists, objects, etc. Here is a
small examples demonstrating how to use Lua tables:

```lua
local t = {}    -- create an empty table
t["x"]  = 10    -- insert the entry "x" -> 10
t.x     = 20    -- syntax-sugar for t["x"] = 20
t["y"]  = 30    -- insert the entry "y" -> 30
assert(t["x"] == 20)
local p = { x = 10, y = 20 } -- create a table with two entries
assert(p.x == 10)
assert(p["x"] == 10)
assert(p.y == 20)
assert(p["y"] == 20)
```

Arrays are implemented by indexing tables with integers.
The arrays don't have a fixed size and grow dynamically.
The arrays in Lua usually start at index 1. The Lua libraries
use this convention. The following example initializes
an array with 100 elements.

```lua
local a = {}    -- new array
for i=1, 100 do
    a[i] = 0
end
local b = {2, 4, 6, 8, 10} -- create an array with 5 elements
assert(#b == 5)    -- array has 5 elements
assert(b[1] == 2)
assert(b[2] == 4)
```
In Lua, we cannot provide custom hash and equality functions to tables.
So, we cannot effectively use Lua tables to implement mappings where
the keys are Lean hierarchical names, expressions, etc.
The following example demonstrates the issue.

```lua
local m  = {} -- create empty table
local a  = name("a")
m[a]     = 10 -- map the hierarchical name 'a' to 10
assert(m[a] == 10)
local a1 = name("a")
assert(a == a1)      -- 'a' and 'a1' are the same hierarchical name
assert(m[a1] == nil) -- 'a1' is not a key in the mapping 'm'
assert(m[a]  == 10)
-- 'a' and 'a1' are two instances of the same object
-- Lua tables assume that different instances are not equal
```

## Splay maps

In Lean, we provide splay maps for implementing mappings where the keys are
Lean objects such as hierarchical names. A splay map is implemented using
a [splay tree](http://en.wikipedia.org/wiki/Splay_tree), a self-adjusting binary
search tree. We can also use Lua atomic data types
as keys in splay maps. However, we should not mix different types in the
same splay map. The Lean splay map assumes that `<` is a total order on the
keys inserted in the map.

```lua
local m = splay_map() -- create an empty splay map
assert(is_splay_map(m))
assert(#m == 0)
local a  = name("a", 1)
local a1 = name("a", 1)
m:insert(a, 10)          -- add the entry 'a::1' -> 10
assert(m:find(a1) == 10)
m:insert(name("b"), 20)
assert(#m == 2)
m:erase(a)               -- remove entry with key 'a::1'
assert(m:find(a) == nil)
assert(not m:contains(a))
assert(#m == 1)
m:insert(name("c"), 30)
m:for_each(              -- traverse the entries in the splay map
    function(k, v)       -- executing the given function
        print(k, v)
    end
)
local m2 = m:copy()      -- the splay maps are copied in constant time
assert(#m2 == #m)
m2:insert(name("b", 2), 40)
assert(#m2 == #m + 1)
```

## Multiple precision numerals

We expose [GMP](http://gmplib.org/) (GNU Multiple precision arithmetic
library) in Lua. Internally, Lean uses multiple precision numerals for
representing expressing containing numerals.
In Lua, we can create multiple precision integers (_mpz_) and multiple
precision rationals (_mpq_). The following example demonstrates how to
use `mpz` and `mpq` numerals.

```lua
local ten = mpz(10) -- create the mpz numeral 10.
assert(is_mpz(ten)) -- test whether 'ten' is a mpz numeral or not
assert(not is_mpz(10))
local big = mpz("100000000000000000000000") -- create a mpz numeral from a string
-- The operators +, -, *, /, ^, <, <=, >, >=, ==, ~=
-- The operators +, -, *, /, ^ accept mixed mpz and Lua native types
assert(ten + 1 == mpz(11))
-- In Lua, objects of different types are always different
-- So, the mpz number 10 is different from the native Lua numeral 10
assert(mpz(10) ~= 10)
assert(mpz(3) / 2 == mpz(1))
-- The second argument of ^ must be a non-negative Lua numeral
assert(mpz(10) ^ 100 > mpz("1000000000000000000000000"))
assert(mpz(3) * mpz("1000000000000000000000") == mpz("3000000000000000000000"))
assert(mpz(4) + "10" == mpz(14))
local q1 = mpq(10) -- create the mpq numeral 10
local q2 = q1 / 3  -- create the mpq numeral 10/3
assert(q2 == mpq("10/3"))
local q3 = mpq(0.25) -- create the mpq numeral 1/4
assert(q3 == mpq(1)/4)
assert(is_mpq(q3)) -- test whether 'q3' is a mpq numeral or not
assert(not is_mpq(mpz(10))) -- mpz numerals are not mpq
assert(ten == mpz(10))
local q4 = mpq(ten) -- convert the mpz numeral 'ten' into a mpq numeral
assert(is_mpq(q4))
assert(mpq(1)/3 + mpq(2)/3 == mpq(1))
assert(mpq(0.5)^5 == mpq("1/32"))
assert(mpq(1)/2 > mpq("1/3"))
```

