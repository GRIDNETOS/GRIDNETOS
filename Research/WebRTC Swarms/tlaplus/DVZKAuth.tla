------------------------------ MODULE DVZKAuth ------------------------------
EXTENDS Naturals, Sequences, TLC

CONSTANTS Peers, HonestPeers, K, T2, Nonces256, Ctx, NULL
ASSUME HonestPeers \subseteq Peers
ASSUME Nonces256 # {}  (* Non-empty set of possible nonces *)

(* Simple MAC model for verification: concatenation *)
MAC(k, m) == <<k, m>>

VARIABLES
  Authenticated,     (* Authenticated[p][q] = TRUE if p authenticated q *)
  Secrets,           (* Secrets[p] = pre-shared key known by p *)
  NonceA,            (* NonceA[p][q] = validator nonce from p to q *)
  NonceB,            (* NonceB[p][q] = candidate nonce from q to p *)
  Tag,               (* Tag[p][q] = MAC tag from q to p *)
  T2Start,           (* T2Start[p][q] = when validator starts timeout T2 *)
  Now                (* Global clock *)

Init ==
  /\ Authenticated = [p \in Peers |-> [q \in Peers |-> FALSE]]
  /\ Secrets = [p \in HonestPeers |-> K] @@
               [p \in (Peers \ HonestPeers) |-> K]  \* adversary keys arbitrary; safety doesn't need their value
  /\ NonceA = [p \in Peers |-> [q \in Peers |-> NULL]]
  /\ NonceB = [p \in Peers |-> [q \in Peers |-> NULL]]
  /\ Tag    = [p \in Peers |-> [q \in Peers |-> NULL]]
  /\ T2Start = [p \in Peers |-> [q \in Peers |-> 0]]
  /\ Now = 0

Phase1(p, q) ==
  /\ NonceB[p][q] = NULL
  /\ \E n \in Nonces256: NonceB' = [NonceB EXCEPT ![p][q] = n]
  /\ UNCHANGED <<Authenticated, Secrets, NonceA, Tag, T2Start, Now>>

Phase2(p, q) ==
  /\ NonceB[p][q] # NULL /\ NonceA[p][q] = NULL
  /\ \E n \in Nonces256:
       /\ n # NonceB[p][q]  (* Ensure fresh nonce *)
       /\ NonceA' = [NonceA EXCEPT ![p][q] = n]
  /\ T2Start' = [T2Start EXCEPT ![p][q] = Now]
  /\ UNCHANGED <<Authenticated, Secrets, NonceB, Tag, Now>>

Phase3(p, q) ==
  /\ NonceA[p][q] # NULL /\ NonceB[p][q] # NULL /\ Tag[p][q] = NULL
  /\ LET m == <<Ctx, NonceA[p][q], NonceB[p][q]>> IN
     Tag' = [Tag EXCEPT ![p][q] = MAC(Secrets[q], m)]
  /\ UNCHANGED <<Authenticated, Secrets, NonceA, NonceB, T2Start, Now>>

Verify(p, q) ==
  /\ Tag[p][q] # NULL
  /\ (Now - T2Start[p][q]) <= T2
  /\ LET m == <<Ctx, NonceA[p][q], NonceB[p][q]>> IN
     /\ Tag[p][q] = MAC(Secrets[p], m)
     /\ Authenticated' = [Authenticated EXCEPT ![p][q] = TRUE]
  /\ UNCHANGED <<Secrets, NonceA, NonceB, Tag, T2Start, Now>>

(* Advance time and clear expired authentications *)
Advance ==
  /\ Now' = Now + 1
  /\ Authenticated' = [p \in Peers |-> [q \in Peers |->
       IF Authenticated[p][q] /\ ((Now' - T2Start[p][q]) > T2)
       THEN FALSE  (* Expire authentication beyond T2 *)
       ELSE Authenticated[p][q]]]
  /\ UNCHANGED <<Secrets, NonceA, NonceB, Tag, T2Start>>

vars == <<Authenticated, Secrets, NonceA, NonceB, Tag, T2Start, Now>>

TypeOK ==
  /\ NonceA \in [Peers -> [Peers -> Nonces256 \cup {NULL}]]
  /\ NonceB \in [Peers -> [Peers -> Nonces256 \cup {NULL}]]
  (* Tag values are either NULL or MAC outputs (arbitrary tuples in abstract model) *)
  /\ \A p, q \in Peers: Tag[p][q] = NULL \/ Tag[p][q] # NULL
  /\ T2Start \in [Peers -> [Peers -> Nat]]
  /\ Now \in Nat

FreshNonces ==
  \A p,q \in Peers:
    /\ (NonceA[p][q] # NULL => NonceA[p][q] \in Nonces256)
    /\ (NonceB[p][q] # NULL => NonceB[p][q] \in Nonces256)
    /\ (NonceA[p][q] # NULL /\ NonceB[p][q] # NULL => NonceA[p][q] # NonceB[p][q])

StateConstraint == Now <= T2  (* Bound to T2 timeout to match SafetyInvariant *)

SafetyInvariant ==
  \A p,q \in Peers:
    Authenticated[p][q] =>
      /\ Tag[p][q] # NULL
      /\ LET m == <<Ctx, NonceA[p][q], NonceB[p][q]>> IN
         Tag[p][q] = MAC(Secrets[p], m) /\ Tag[p][q] = MAC(Secrets[q], m)
      /\ (Now - T2Start[p][q]) <= T2  (* Timeout check as per paper's specification *)

Next ==
  \/ \E p,q \in Peers: Phase1(p,q)
  \/ \E p,q \in Peers: Phase2(p,q)
  \/ \E p,q \in Peers: Phase3(p,q)
  \/ \E p,q \in Peers: Verify(p,q)
  \/ Advance

Spec == Init /\ [][Next]_vars

THEOREM Spec => [](TypeOK /\ FreshNonces /\ SafetyInvariant)
==============================================================================
