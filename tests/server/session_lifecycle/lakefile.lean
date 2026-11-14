import Lake
open Lake DSL

package sessionLifecycle

@[test_driver]
script verify (args) do
  let child <- IO.Process.spawn {
    cmd := "node"
    args := #["verify.mjs"] ++ args.toArray
  }
  child.wait
