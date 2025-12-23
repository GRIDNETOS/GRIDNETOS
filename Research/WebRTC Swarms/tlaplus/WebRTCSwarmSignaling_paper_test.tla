------------------------------ MODULE WebRTCSwarmSignaling_paper_test ------------------------------
EXTENDS Naturals, Sequences, FiniteSets

CONSTANTS
  Peers, FullNodes, Swarms, SessionIDs, SDPOffers, SDPAnswers,
  MsgIDs, TTLMax, NULL

VARIABLES
  swarmMembers, memberStatus, peerState,
  connectionState, activeOffers, activeAnswers, sessionIDs,
  messagesInTransit, seenMsgIDs, lastJoinTime, Now, usedMsgIDs

TypeOK ==
  /\ swarmMembers \in [FullNodes -> [Swarms -> SUBSET Peers]]
  /\ memberStatus \in [FullNodes -> [Swarms -> [Peers -> {"none","unauthenticated","authenticated"}]]]
  /\ peerState \in [Peers -> {"idle","joining","joinConfirmed","connecting","connected"}]
  /\ connectionState \in [Peers -> [Peers -> {"none","offerSent","answerSent","iceExchange","connected"}]]
  /\ activeOffers \in [Peers -> [Peers -> SDPOffers \cup {NULL}]]
  /\ activeAnswers \in [Peers -> [Peers -> SDPAnswers \cup {NULL}]]
  /\ sessionIDs \in [Peers -> [Peers -> SessionIDs \cup {NULL}]]
  (* messagesInTransit is a sequence of message records *)
  /\ \A i \in 1..Len(messagesInTransit): messagesInTransit[i].msgID \in MsgIDs
  /\ seenMsgIDs \in [FullNodes -> SUBSET MsgIDs]
  /\ lastJoinTime \in [Peers -> Nat]
  /\ Now \in Nat

Init ==
  /\ swarmMembers = [fn \in FullNodes |-> [sw \in Swarms |-> {}]]
  /\ memberStatus = [fn \in FullNodes |-> [sw \in Swarms |-> [p \in Peers |-> "none"]]]
  /\ peerState = [p \in Peers |-> "idle"]
  /\ connectionState = [p \in Peers |-> [q \in Peers |-> "none"]]
  /\ activeOffers = [p \in Peers |-> [q \in Peers |-> NULL]]
  /\ activeAnswers = [p \in Peers |-> [q \in Peers |-> NULL]]
  /\ sessionIDs = [p \in Peers |-> [q \in Peers |-> NULL]]
  /\ messagesInTransit = <<>>
  /\ seenMsgIDs = [fn \in FullNodes |-> {}]
  /\ lastJoinTime = [p \in Peers |-> 0]
  /\ Now = 0
  /\ usedMsgIDs = {}

SendJoin(p, sw, fn) ==
  /\ peerState[p] = "idle"
  /\ (Now - lastJoinTime[p]) >= 5
  /\ peerState' = [peerState EXCEPT ![p] = "joining"]
  /\ lastJoinTime' = [lastJoinTime EXCEPT ![p] = Now]
  /\ swarmMembers' = [swarmMembers EXCEPT ![fn][sw] = @ \cup {p}]
  /\ memberStatus' = [memberStatus EXCEPT ![fn][sw][p] = "unauthenticated"]
  /\ \E mid \in MsgIDs \ usedMsgIDs:
       /\ messagesInTransit' = Append(messagesInTransit, [type |-> "joining", from |-> p, to |-> {fn}, msgID |-> mid, ttl |-> TTLMax, payload |-> <<>>])
       /\ usedMsgIDs' = usedMsgIDs \cup {mid}
  /\ UNCHANGED <<connectionState, activeOffers, activeAnswers, sessionIDs, seenMsgIDs, Now>>

BroadcastGetOffer(p, sw, fn) ==
  /\ memberStatus[fn][sw][p] = "unauthenticated"
  /\ peerState[p] = "joining"
  /\ peerState' = [peerState EXCEPT ![p] = "joinConfirmed"]
  /\ \E mid \in MsgIDs \ usedMsgIDs:
       /\ messagesInTransit' = Append(messagesInTransit, [type |-> "getOffer", from |-> p, to |-> swarmMembers[fn][sw] \ {p}, msgID |-> mid, ttl |-> TTLMax, payload |-> <<>>])
       /\ usedMsgIDs' = usedMsgIDs \cup {mid}
  /\ UNCHANGED <<swarmMembers, memberStatus, connectionState, activeOffers, activeAnswers, sessionIDs, seenMsgIDs, lastJoinTime, Now>>

Forward(n) ==
  /\ Len(messagesInTransit) > 0
  /\ \E i \in 1..Len(messagesInTransit):
       LET m == messagesInTransit[i] IN
         /\ m.type \in {"joining","getOffer"}
         /\ m.ttl > 0
         /\ m.msgID \notin seenMsgIDs[n]
         /\ seenMsgIDs' = [seenMsgIDs EXCEPT ![n] = @ \cup {m.msgID}]
         (* Remove message from queue after processing - no re-adding *)
         /\ messagesInTransit' = SubSeq(messagesInTransit,1,i-1) \o SubSeq(messagesInTransit,i+1,Len(messagesInTransit))
         /\ UNCHANGED <<swarmMembers, memberStatus, peerState, connectionState, activeOffers, activeAnswers, sessionIDs, lastJoinTime, Now, usedMsgIDs>>

AdvanceTime(delta) ==
  /\ delta > 0
  /\ Now' = Now + delta
  /\ UNCHANGED <<swarmMembers, memberStatus, peerState, connectionState, activeOffers, activeAnswers, sessionIDs, messagesInTransit, seenMsgIDs, lastJoinTime, usedMsgIDs>>

Invariant_NoDupForward ==
  (* No duplicate message IDs in the message queue *)
  \A mid \in MsgIDs: Cardinality({ i \in 1..Len(messagesInTransit) : messagesInTransit[i].msgID = mid }) <= 1

Invariant_TTLNonNegative ==
  \A i \in 1..Len(messagesInTransit): messagesInTransit[i].ttl \in Nat

Invariant1 ==
  \A p,q \in Peers:
    (p # q) =>
      /\ ~(connectionState[p][q] = "offerSent" /\ connectionState[q][p] = "offerSent")
      /\ ~(activeOffers[p][q] # NULL /\ activeOffers[q][p] # NULL)

Invariant2 ==
  \A sw \in Swarms, fn1, fn2 \in FullNodes, p \in Peers:
    (p \in swarmMembers[fn1][sw] /\ p \in swarmMembers[fn2][sw]) =>
      (memberStatus[fn1][sw][p] = memberStatus[fn2][sw][p])

Next ==
  \/ \E p \in Peers, sw \in Swarms, fn \in FullNodes: SendJoin(p, sw, fn)
  \/ \E p \in Peers, sw \in Swarms, fn \in FullNodes: BroadcastGetOffer(p, sw, fn)
  \/ \E n \in FullNodes: Forward(n)
  \/ \E delta \in {1, 2, 5}: AdvanceTime(delta)  (* Bounded delta for model checking *)

StateConstraint == Now <= 10 /\ Len(messagesInTransit) <= 5  (* Bound state space *)

vars == <<swarmMembers, memberStatus, peerState, connectionState, activeOffers, activeAnswers, sessionIDs, messagesInTransit, seenMsgIDs, lastJoinTime, Now, usedMsgIDs>>

Spec == Init /\ [][Next]_vars

THEOREM Spec => [](TypeOK /\ Invariant1 /\ Invariant2 /\ Invariant_NoDupForward /\ Invariant_TTLNonNegative)
==============================================================================
