import Std

set_option autoImplicit false

namespace SessionLifecycle

/- OPC 10000-4 v1.05.07, 5.7.2.1:
   https://reference.opcfoundation.org/specs/OPC-10000-4/5.7.2.1
   LLM-assisted interpretation, subject to human normative review.
   Creation ordering and at-capacity reclamation are explicit policy choices.
   The strict expiry boundary and zero-as-unlimited convention are source-
   informed contracts. Time is integral milliseconds; callbacks, allocation
   failures, transport, subscriptions and concurrency are outside this model. -/
structure Session where
  id : Nat
  activated : Bool
  timeout : Nat
  deadline : Nat
  deriving DecidableEq, Repr

structure State where
  now : Nat := 0
  next : Nat := 0
  sessions : List Session := []
  deriving DecidableEq, Repr

inductive Operation where
  | create (timeout : Nat)
  | activate (id : Nat)
  | close (id : Nat)
  | advance (milliseconds : Nat)
  deriving DecidableEq, Repr

inductive Status where
  | good | invalidSession | tooManySessions
  deriving DecidableEq, Repr

structure Result where
  status : Status
  state : State
  deriving DecidableEq, Repr

def firstPending : List Session → Option Session
  | [] => none
  | s :: rest => if s.activated then firstPending rest else some s

def evictOldest : List Session → List Session
  | [] => []
  | s :: rest => if s.activated then s :: evictOldest rest else rest

def active (q : List Session) : List Session := q.filter (fun s => s.activated)

theorem eviction_preserves_active (q : List Session) :
    active (evictOldest q) = active q := by
  induction q with
  | nil => rfl
  | cons s rest ih =>
      cases h : s.activated <;> simp [evictOldest, active, h] at *
      exact ih

theorem oldest_removal (q : List Session) (victim : Session)
    (selected : firstPending q = some victim) :
    ∃ before after, q = before ++ victim :: after ∧
      (∀ s ∈ before, s.activated = true) ∧ victim.activated = false ∧
      evictOldest q = before ++ after := by
  induction q with
  | nil => simp [firstPending] at selected
  | cons s rest ih =>
      cases h : s.activated with
      | false =>
          simp [firstPending, h] at selected
          subst victim
          exact ⟨[], rest, rfl, by simp, h, by simp [evictOldest, h]⟩
      | true =>
          have tailSelected : firstPending rest = some victim := by
            simpa [firstPending, h] using selected
          obtain ⟨before, after, eq, activeBefore, pending, remove⟩ := ih tailSelected
          refine ⟨s :: before, after, ?_, ?_, pending, ?_⟩
          · simp [eq]
          · intro x member
            simp only [List.mem_cons] at member
            rcases member with equal | tail
            · simpa [equal] using h
            · exact activeBefore x tail
          · simp [evictOldest, h, remove]

theorem removal_decreases_length (q : List Session) (victim : Session)
    (selected : firstPending q = some victim) :
    (evictOldest q).length + 1 = q.length := by
  obtain ⟨before, after, eq, _, _, remove⟩ := oldest_removal q victim selected
  rw [remove, eq]
  simp only [List.length_append, List.length_cons]
  omega

theorem no_pending_iff_all_active (q : List Session) :
    firstPending q = none ↔ ∀ s ∈ q, s.activated = true := by
  induction q with
  | nil => simp [firstPending]
  | cons s rest ih =>
      cases h : s.activated <;> simp [firstPending, h, ih]

def insert (timeout : Nat) (st : State) (q : List Session) : State :=
  { st with next := st.next + 1, sessions := q ++ [⟨st.next, false, timeout, st.now + timeout⟩] }

def create (maximum timeout : Nat) (st : State) : Result :=
  if maximum = 0 ∨ st.sessions.length < maximum then
    ⟨.good, insert timeout st st.sessions⟩
  else match firstPending st.sessions with
    | none => ⟨.tooManySessions, { st with next := st.next + 1 }⟩
    | some _ => ⟨.good, insert timeout st (evictOldest st.sessions)⟩

def activate (id : Nat) (st : State) : Result :=
  if st.sessions.any (fun s => s.id == id) then
    ⟨.good, { st with sessions := st.sessions.map fun s =>
      if s.id = id then { s with activated := true, deadline := st.now + s.timeout }
      else s }⟩
  else ⟨.invalidSession, st⟩

def close (id : Nat) (st : State) : Result :=
  if st.sessions.any (fun s => s.id == id) then
    ⟨.good, { st with sessions := st.sessions.filter (fun s => s.id != id) }⟩
  else ⟨.invalidSession, st⟩

def advance (dt : Nat) (st : State) : State :=
  { st with now := st.now + dt, sessions := st.sessions.filter (fun s => st.now + dt ≤ s.deadline) }

def execute (maximum : Nat) (op : Operation) (st : State) : Result :=
  match op with
  | .create timeout => create maximum timeout st
  | .activate id => activate id st
  | .close id => close id st
  | .advance dt => ⟨.good, advance dt st⟩

theorem create_preserves_active (maximum timeout : Nat) (st : State) :
    active (create maximum timeout st).state.sessions = active st.sessions := by
  unfold create
  split
  · simp [insert, active, List.filter_append]
  · cases h : firstPending st.sessions <;>
      simp [insert, active, List.filter_append, h, eviction_preserves_active]
    exact eviction_preserves_active st.sessions

theorem unlimited_does_not_evict (timeout : Nat) (st : State) :
    create 0 timeout st = ⟨.good, insert timeout st st.sessions⟩ := by
  simp [create]

theorem capacity_refusal_preserves_sessions (maximum timeout : Nat) (st : State)
    (refused : (create maximum timeout st).status = .tooManySessions) :
    (create maximum timeout st).state.sessions = st.sessions := by
  by_cases room : maximum = 0 ∨ st.sessions.length < maximum
  · simp [create, room] at refused
  · cases h : firstPending st.sessions with
    | none => simp [create, room, h]
    | some victim => simp [create, room, h] at refused

theorem create_respects_capacity (maximum timeout : Nat) (st : State)
    (positive : 0 < maximum) (within : st.sessions.length ≤ maximum) :
    (create maximum timeout st).state.sessions.length ≤ maximum := by
  by_cases room : st.sessions.length < maximum
  · simp [create, room, insert]
    omega
  · have nonzero : maximum ≠ 0 := by omega
    cases h : firstPending st.sessions with
    | none => simp [create, room, nonzero, h, within]
    | some victim =>
        have decreases := removal_decreases_length st.sessions victim h
        simp [create, room, nonzero, h, insert]
        omega

theorem activation_preserves_length (id : Nat) (st : State) :
    (activate id st).state.sessions.length = st.sessions.length := by
  unfold activate
  split <;> simp

theorem activation_makes_matching_session_active (id : Nat) (st : State)
    (present : st.sessions.any (fun s => s.id == id) = true)
    (s : Session) (member : s ∈ (activate id st).state.sessions)
    (matching : s.id = id) : s.activated = true := by
  simp only [activate, present, Bool.true_eq, ↓reduceIte] at member
  obtain ⟨original, _, eq⟩ := List.mem_map.mp member
  by_cases h : original.id = id
  · simp [h] at eq
    subst s
    rfl
  · simp [h] at eq
    subst s
    exact (h matching).elim

theorem close_removes_identity (id : Nat) (st : State)
    (s : Session) (member : s ∈ (close id st).state.sessions) : s.id ≠ id := by
  unfold close at member
  split at member
  · simpa using (List.mem_filter.mp member).2
  · rename_i absent
    have notAny : st.sessions.any (fun x => x.id == id) = false := by
      simpa using absent
    intro matching
    have anyTrue : st.sessions.any (fun x => x.id == id) = true :=
      List.any_eq_true.mpr ⟨s, member, by simp [matching]⟩
    simp [notAny] at anyTrue

theorem close_does_not_grow (id : Nat) (st : State) :
    (close id st).state.sessions.length ≤ st.sessions.length := by
  unfold close
  split
  · exact List.length_filter_le _ _
  · exact Nat.le_refl _

theorem expiry_retains_exact_deadline (dt : Nat) (st : State) (s : Session)
    (member : s ∈ st.sessions) (boundary : st.now + dt = s.deadline) :
    s ∈ (advance dt st).sessions := by
  simp [advance, List.mem_filter, member, boundary]

theorem expiry_removes_past_deadline (dt : Nat) (st : State) (s : Session)
    (past : s.deadline < st.now + dt) : s ∉ (advance dt st).sessions := by
  simp [advance, List.mem_filter, show ¬ st.now + dt ≤ s.deadline by omega]

theorem expiry_does_not_grow (dt : Nat) (st : State) :
    (advance dt st).sessions.length ≤ st.sessions.length :=
  List.length_filter_le _ _

theorem step_respects_capacity (maximum : Nat) (op : Operation) (st : State)
    (positive : 0 < maximum) (within : st.sessions.length ≤ maximum) :
    (execute maximum op st).state.sessions.length ≤ maximum := by
  cases op with
  | create timeout => exact create_respects_capacity maximum timeout st positive within
  | activate id => simpa [execute, activation_preserves_length] using within
  | close id => exact Nat.le_trans (close_does_not_grow id st) within
  | advance dt => exact Nat.le_trans (expiry_does_not_grow dt st) within

def run (maximum : Nat) : List Operation → State → State
  | [], st => st
  | op :: ops, st => run maximum ops (execute maximum op st).state

theorem history_respects_capacity (maximum : Nat) (ops : List Operation) (st : State)
    (positive : 0 < maximum) (within : st.sessions.length ≤ maximum) :
    (run maximum ops st).sessions.length ≤ maximum := by
  induction ops generalizing st with
  | nil => exact within
  | cons op ops ih =>
      exact ih _ (step_respects_capacity maximum op st positive within)

#print axioms eviction_preserves_active
#print axioms oldest_removal
#print axioms removal_decreases_length
#print axioms no_pending_iff_all_active
#print axioms create_preserves_active
#print axioms unlimited_does_not_evict
#print axioms capacity_refusal_preserves_sessions
#print axioms create_respects_capacity
#print axioms activation_preserves_length
#print axioms activation_makes_matching_session_active
#print axioms close_removes_identity
#print axioms close_does_not_grow
#print axioms expiry_retains_exact_deadline
#print axioms expiry_removes_past_deadline
#print axioms expiry_does_not_grow
#print axioms step_respects_capacity
#print axioms history_respects_capacity

end SessionLifecycle
