import Model

set_option autoImplicit false

namespace SessionLifecycle

structure Scenario where
  name : String
  maximum : Nat
  tcp : Bool
  ops : List Operation
  deriving DecidableEq, Repr

structure Observation where
  status : Status
  liveMask : Nat
  activeMask : Nat
  count : Nat
  activeCount : Nat
  deriving DecidableEq, Repr

def mask (q : List Session) : Nat :=
  q.foldl (fun acc s => acc ||| 2 ^ s.id) 0

def observations (maximum : Nat) : List Operation → State → List Observation
  | [], _ => []
  | op :: ops, st =>
      let result := execute maximum op st
      { status := result.status, liveMask := mask result.state.sessions,
        activeMask := mask (active result.state.sessions),
        count := result.state.sessions.length,
        activeCount := (active result.state.sessions).length } ::
      observations maximum ops result.state

def observe (s : Scenario) : List Observation := observations s.maximum s.ops {}

-- A bounded enumeration over this declared alphabet, not full OPC UA coverage.
def alphabet : List Operation :=
  [.create 20, .activate 0, .activate 1, .activate 2,
   .close 0, .close 1, .close 2, .advance 1, .advance 20]

def setupOps (population activeMask : Nat) : List Operation :=
  (List.range population).flatMap fun id =>
    [.create 20] ++ if (activeMask / 2 ^ id) % 2 = 1 then [.activate id] else []

def enumerated : List Scenario :=
  [1, 2, 3].flatMap fun maximum =>
    (List.range (maximum + 1)).flatMap fun population =>
      (List.range (2 ^ population)).flatMap fun activeMask =>
        (alphabet.zipIdx).flatMap fun (first, a) =>
          (alphabet.zipIdx).map fun (second, b) =>
            { name := s!"c{maximum}_n{population}_m{activeMask}_w{a}_{b}",
              maximum := maximum, tcp := false,
              ops := setupOps population activeMask ++ [first, second] }

def curated : List Scenario :=
  [{ name := "activation_refusal_close_readmission", maximum := 2, tcp := true,
     ops := [.create 20000, .create 20000, .activate 0, .create 20000,
             .activate 2, .create 20000, .close 0, .create 20000] },
   { name := "repeated_activation_changes_victim", maximum := 2, tcp := true,
     ops := [.create 20000, .create 20000, .create 20000, .activate 1,
             .create 20000, .activate 3, .create 20000, .close 1, .create 20000] },
   { name := "creation_order_not_timeout_order", maximum := 2, tcp := true,
     ops := [.create 60000, .create 20000, .create 60000, .activate 1,
             .create 60000, .activate 3] },
   { name := "unlimited_capacity", maximum := 0, tcp := true,
     ops := [.create 20000, .activate 0, .create 20000, .create 20000,
             .create 20000, .close 1, .create 20000] },
   { name := "exact_expiry_boundary", maximum := 1, tcp := false,
     ops := [.create 10, .advance 10, .advance 1, .activate 0, .close 0,
             .create 10] },
   { name := "activation_refreshes_deadline", maximum := 1, tcp := false,
     ops := [.create 10, .advance 9, .activate 0, .advance 1, .advance 9,
             .advance 1, .activate 0] },
   { name := "expiry_frees_capacity", maximum := 2, tcp := false,
     ops := [.create 20, .create 10, .activate 0, .advance 10, .advance 1,
             .create 20, .activate 2, .close 0, .create 20] },
   { name := "reactivation_refreshes_deadline", maximum := 1, tcp := false,
     ops := [.create 10, .activate 0, .advance 9, .activate 0,
             .advance 10, .advance 1] }]

def corpus : List Scenario := enumerated ++ curated

theorem one_observation_per_operation (maximum : Nat) (ops : List Operation) (st : State) :
    (observations maximum ops st).length = ops.length := by
  induction ops generalizing st with
  | nil => rfl
  | cons op ops ih => simp [observations, ih]
#print axioms one_observation_per_operation

end SessionLifecycle
