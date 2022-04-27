#!/usr/bin/env python3

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import subprocess
import os
import re
from email.utils import formatdate
import datetime
import shutil

# Get the base filesystem paths.
# The current path is used as the "build environment"
dirpath = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)),".."))
debian_path = os.path.join(dirpath, "tools/packaging/debian")
target_debian_path = os.path.join(dirpath, "debian")
shutil.copytree(debian_path, target_debian_path)

# Unpack the library version from the git information
git_describe_version = subprocess.check_output(["git", "describe", "--tags", "--dirty", "--match", "v*"]).decode('utf-8').strip()

# v1.2
# v1.2.3
# v1.2.3-rc1
# v1.2.3-rc1-dirty
# v1.2.3-5-g4538abcd
# v1.2.3-5-g4538abcd-dirty
# git_describe_version = "v1.2.3"

m = re.match(r"^v([0-9]{1,4})(\.[0-9]{1,4}){0,2}(-(.*){1,100})?$", git_describe_version)
version_major = m.group(1) if m.group(1) is not None else "0"
version_minor = m.group(2).replace(".", "") if m.group(2) is not None else "0"
version_patch = m.group(3).replace(".", "") if m.group(3) is not None else "0"
version_label = m.group(4) if m.group(4) is not None else ""
#print("major {} minor {} patch {} label {}".format(version_major, version_minor, version_patch, version_label))

# We can not use unstable for now, because dpkg-buildpackage wants to sign them
# It will fail with gpg: skipped "open62541 Team <open62541-core@googlegroups.com>": No secret key
#debian_distribution = "unstable"
#if version_label is not "":
debian_distribution = "UNRELEASED"

changelog_file = os.path.join(debian_path, "changelog")

# remove leading 'v'
changelog_version = git_describe_version[1:] if git_describe_version[0] == 'v' else git_describe_version
# replace all '-' with '~' in version
changelog_version = changelog_version.replace('-', '~')

# prefix the version string with the current ISO datetime to ensure correct version ordering.
# See https://github.com/open62541/open62541/issues/3140
changelog_version = datetime.datetime.utcnow().replace(microsecond=0).isoformat().replace('-', '').replace(':', '') + '~' + changelog_version

# Create an updated changelog file with the version information
with open(changelog_file, 'r') as original:
    data = original.read()
with open(os.path.join(target_debian_path, "changelog"), 'w') as modified:
    new_entry = """open62541 ({version}) {distribution}; urgency=medium

  * Full changelog is available here:
    https://github.com/open62541/open62541/blob/master/CHANGELOG

 -- open62541 Team <open62541-core@googlegroups.com>  {time}
""".format(version=changelog_version, time=formatdate(), distribution = debian_distribution)

    modified.write(new_entry + "\n" + data)

# Create control file and replace template variables
control_file_template = os.path.join(debian_path, "control-template")
control_file = os.path.join(target_debian_path, "control")
shutil.copy(control_file_template, control_file)
with open(control_file, 'r+') as f:
    content = f.read()
    f.seek(0)
    f.truncate()
    f.write(content.replace('<soname>', "{}.{}".format(version_major, version_minor)))

# rename the install template to match the soname
install_file_template = os.path.join(debian_path, "libopen62541.install-template")
install_file = os.path.join(target_debian_path,
                  "libopen62541-{}.{}.install".format(version_major, version_minor))
shutil.copy(install_file_template, install_file)

install_file_template = os.path.join(debian_path, "libopen62541-dev.install-template")
install_file = os.path.join(target_debian_path,
                  "libopen62541-{}.{}-dev.install".format(version_major, version_minor))
shutil.copy(install_file_template, install_file)

install_file_template = os.path.join(debian_path, "libopen62541-tools.install-template")
install_file = os.path.join(target_debian_path,
                  "libopen62541-{}.{}-tools.install".format(version_major, version_minor))
shutil.copy(install_file_template, install_file)

install_file_template = os.path.join(debian_path, "libopen62541-doc.doc-base-template")
install_file = os.path.join(target_debian_path,
                  "libopen62541-{}.{}-doc.doc-base".format(version_major, version_minor))
shutil.copy(install_file_template, install_file)

install_file_template = os.path.join(debian_path, "libopen62541-doc.install-template")
install_file = os.path.join(target_debian_path,
                  "libopen62541-{}.{}-doc.install".format(version_major, version_minor))
shutil.copy(install_file_template, install_file)

# Create rule file and replace template variables
rule_file_template = os.path.join(debian_path, "rules-template")
rule_file = os.path.join(target_debian_path, "rules")
shutil.copy(rule_file_template, rule_file)
with open(rule_file, 'r+') as f:
    content = f.read()
    f.seek(0)
    f.truncate()
    content = content.replace('<srcdir>', "{}".format(dirpath))
    f.write(content)

# Update CMakeLists.txt to include full version string
with open(os.path.join(dirpath,"CMakeLists.txt"), 'r+') as f:
    lines = f.readlines()
    f.seek(0)
    f.truncate()
    for idx, line in enumerate(lines):
        if idx == 2:
            f.write('set(OPEN62541_VERSION "{}")\n'.format(git_describe_version))
        f.write(line)
