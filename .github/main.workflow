workflow "Build and Deploy" {
    on = "push"
    resolves = ["Build"]
}


action "Init: Submodules" {
    uses = "./.github/actions/build-base"
    runs = "sh -c"
    args = ["git submodule sync && git submodule update --init --recursive"]
}

action "Build: Full NS0 (Python2)" {
    needs = "Init: Submodules"
    uses = "./.github/actions/build-base"
    runs = "sh -c"
    args = ["export PYTHON=python2 && ./.github/actions/build_full_ns0.sh"]
}

action "Build: Full NS0" {
    needs = "Init: Submodules"
    uses = "./.github/actions/build-base"
    runs = "sh -c"
    args = ["export PYTHON=python3 && ./.github/actions/build_full_ns0.sh"]
}

action "Test: Full NS0" {
    needs = ["Build: Full NS0 (Python2)", "Build: Full NS0"]
    uses = "./.github/actions/build-base"
    runs = "sh -c"
    args = ["cd build-full-ns0-python3 && make test"]
}

action "Test: Debian Packaging" {
    needs = "Init: Submodules"
    uses = "./.github/actions/build-base"
    runs = "sh -c"
    args = ["export PYTHON=python2 && ./.github/actions/build_debian_package.sh"]
}

action "Build" {
    uses = "./.github/actions/build-base"
    needs = ["Test: Full NS0"]
}
