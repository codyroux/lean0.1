(* import("tactic.lua") *)
variables p q r : Bool

theorem T1 : p → q → p /\ q :=
  (fun H1 H2,
     let H1 : p := _,
         H2 : q := _
     in and_intro H1 H2
   ).
    exact -- solve first metavar
    done
    exact -- solve second metavar
    done

(*
simple_tac = Repeat(conj_tac() ^ assumption_tac())
*)

theorem T2 : p → q → p /\ q /\ p := _.
    simple_tac
    done

print environment 1

theorem T3 : p → p /\ q → r → q /\ r /\ p := _.
    (* Repeat(OrElse(conj_tac(), conj_hyp_tac(), assumption_tac())) *)
    done

-- Display proof term generated by previous tac
print environment 1

theorem T4 : p → p /\ q → r → q /\ r /\ p := _.
    Repeat (OrElse (apply and_intro) conj_hyp exact)
    done

-- Display proof term generated by previous tac --
print environment 1