set::option pp::colors false
variable N : Type
variables a b c : N
variables P : N -> N -> N -> Bool

set::opaque forall  false.
set::opaque exists  false.
set::opaque not     false.

theorem T1 (f : N -> N) (H : P (f a) b (f (f c))) : exists x y z, P x y z := exists::intro _ (exists::intro _ (exists::intro _ H))

print environment 1.