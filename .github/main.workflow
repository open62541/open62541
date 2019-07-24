workflow "Build and Deploy" {
  on = "push"
  resolves = ["Build"]
}


action "Init Submodules" {
  needs = "Init Submodules"
  uses = "./.github/actions/build-base"
  runs = ["git submodule sync && git submodule update --init --recursive"]
}

action "Build Full NS0" {
 needs = "Init Submodules"
 uses = "./.github/actions/build-base"
 runs = ["./.github/actions/build_full_ns0.sh"]
}

action "Unit Test Full NS0" {
 needs = "Build Full NS0"
 uses = "./.github/actions/build-base"
 runs = ["cd build && make test"]
}

action "Build" {
  needs = ["Unit Test Full NS0"]
}
