import Int.
(* import("tactic.lua") *)
variable f : Int -> Int -> Int

(*
refl_tac           = apply_tac("refl")
congr_tac          = apply_tac("congr")
symm_tac           = apply_tac("symm")
trans_tac          = apply_tac("trans")
auto = Repeat(OrElse(refl_tac, congr_tac, assumption_tac(), Then(symm_tac, assumption_tac(), now_tac())))
*)

exit -- Temporarily disable the following tests

theorem T1 (a b c : Int) (H1 : a = b) (H2 : a = c) : (f (f a a) b) = (f (f b c) a).
   auto.
   done.

theorem T2 (a b c : Int) (H1 : a = b) (H2 : a = c) : (f (f a c)) = (f (f b a)).
   auto.
   done.

print environment 2.