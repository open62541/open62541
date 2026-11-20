import Corpus

set_option autoImplicit false

namespace SessionLifecycle

def opLiteral : Operation → String
  | .create n => s!".create {n}"
  | .activate n => s!".activate {n}"
  | .close n => s!".close {n}"
  | .advance n => s!".advance {n}"

def statusLiteral : Status → String
  | .good => ".good"
  | .invalidSession => ".invalidSession"
  | .tooManySessions => ".tooManySessions"

def scenarioLiteral (s : Scenario) : String :=
  "{ name := \"" ++ s.name ++ "\", maximum := " ++ toString s.maximum ++
  ", tcp := " ++ toString s.tcp ++ ", ops := [" ++
  String.intercalate ", " (s.ops.map opLiteral) ++ "] }"

def observationLiteral (o : Observation) : String :=
  "{ status := " ++ statusLiteral o.status ++
  s!", liveMask := {o.liveMask}, activeMask := {o.activeMask}, count := {o.count}, activeCount := {o.activeCount}" ++ " }"

def certificate (index : Nat) (s : Scenario) : String :=
  s!"theorem trace_{index} : observe " ++ scenarioLiteral s ++ " = [" ++
  String.intercalate ", " ((observe s).map observationLiteral) ++ "] := by rfl\n" ++
  s!"#print axioms trace_{index}\n"

def opJson (op : Operation) : String :=
  let (kind, arg) := match op with
    | .create n => ("create", n)
    | .activate n => ("activate", n)
    | .close n => ("close", n)
    | .advance n => ("advance", n)
  "{\"kind\":\"" ++ kind ++ "\",\"arg\":" ++ toString arg ++ "}"

def statusJson : Status → String
  | .good => "good"
  | .invalidSession => "invalidSession"
  | .tooManySessions => "tooManySessions"

def observationJson (o : Observation) : String :=
  "{\"status\":\"" ++ statusJson o.status ++ "\",\"liveMask\":" ++ toString o.liveMask ++
  ",\"activeMask\":" ++ toString o.activeMask ++ ",\"count\":" ++ toString o.count ++
  ",\"activeCount\":" ++ toString o.activeCount ++ "}"

def scenarioJson (s : Scenario) : String :=
  "{\"name\":\"" ++ s.name ++ "\",\"maximum\":" ++ toString s.maximum ++
  ",\"tcp\":" ++ toString s.tcp ++ ",\"ops\":[" ++
  String.intercalate "," (s.ops.map opJson) ++ "],\"observations\":[" ++
  String.intercalate "," ((observe s).map observationJson) ++ "]}\n"

end SessionLifecycle

def main (args : List String) : IO Unit := do
  match args with
  | ["json"] => IO.print (String.join (SessionLifecycle.corpus.map SessionLifecycle.scenarioJson))
  | ["lean", start, count] =>
      let some start := start.toNat? | throw (IO.userError "Invalid start")
      let some count := count.toNat? | throw (IO.userError "Invalid count")
      IO.print ("import Corpus\nnamespace SessionLifecycle\n" ++
        String.join (((SessionLifecycle.corpus.zipIdx).drop start |>.take count).map
          fun (s, i) => SessionLifecycle.certificate i s) ++ "end SessionLifecycle\n")
  | _ => throw (IO.userError "Expected json or lean START COUNT")
