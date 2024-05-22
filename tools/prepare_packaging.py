#!/usr/bin/env python3

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import subprocess
import os
import re
import shutil

# Get the base filesystem paths.
# The current path is used as the "build environment".
dirpath = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)),".."))
debian_path = os.path.join(dirpath, "tools/packaging/debian")
target_debian_path = os.path.join(dirpath, "debian")
shutil.copytree(debian_path, target_debian_path, ignore=shutil.ignore_patterns('*-template'))

# v1.2
# v1.2.3
# v1.2.3-rc1
# v1.2.3-rc1-dirty
# v1.2.3-5-g4538abcd
# v1.2.3-5-g4538abcd-dirty
# git_describe_version = "v1.2.3"

git_describe_version = subprocess.check_output(["git", "describe", "--tags", "--dirty", "--match", "v*"]).decode('utf-8').strip()
m = re.match(r"^v([0-9]{1,4})(\.[0-9]{1,4})?(\.[0-9]{1,4})?(-(.*){1,100})?$", git_describe_version)
version_major = m.group(1) if m.group(1) is not None else "0"
version_minor = m.group(2).replace(".", "") if m.group(2) is not None else "0"
version_patch = m.group(3).replace(".", "") if m.group(3) is not None else "0"
version_label = m.group(4) if m.group(4) is not None else ""

# Create control file and replace template variables
control_file_template = os.path.join(debian_path, "control-template")
control_file = os.path.join(target_debian_path, "control")
shutil.copy(control_file_template, control_file)
with open(control_file, 'r+') as f:
    content = f.read()
    f.seek(0)
    f.truncate()
    f.write(content.replace('<soname>', "{}.{}".format(version_major, version_minor)))

# Rename the install template to match the soname
install_file_template = os.path.join(debian_path, "libopen62541.install-template")
install_file = os.path.join(target_debian_path, "libopen62541-{}.{}.install".format(version_major, version_minor))
shutil.copy(install_file_template, install_file)

install_file_template = os.path.join(debian_path, "libopen62541-dev.install-template")
install_file = os.path.join(target_debian_path, "libopen62541-{}.{}-dev.install".format(version_major, version_minor))
shutil.copy(install_file_template, install_file)

install_file_template = os.path.join(debian_path, "libopen62541-tools.install-template")
install_file = os.path.join(target_debian_path, "libopen62541-{}.{}-tools.install".format(version_major, version_minor))
shutil.copy(install_file_template, install_file)

# Update CMakeLists.txt to include full version string
with open(os.path.join(dirpath,"CMakeLists.txt"), 'r+') as f:
    lines = f.readlines()
    f.seek(0)
    f.truncate()
    for idx, line in enumerate(lines):
        if idx == 1:
            f.write('set(OPEN62541_VERSION "{}")\n'.format(git_describe_version))
        f.write(line)
