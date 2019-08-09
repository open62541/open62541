#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import subprocess
import os
import re
from email.utils import formatdate

git_describe_version = subprocess.check_output(["git", "describe", "--tags", "--dirty", "--match", "v*"]).decode('utf-8').strip()

# v1.2
# v1.2.3
# v1.2.3-rc1
# v1.2.3-rc1-dirty
# v1.2.3-5-g4538abcd
# v1.2.3-5-g4538abcd-dirty
# git_describe_version = "v1.2.3"

m = re.match(r"^v([0-9]+)(\.[0-9]+)?(\.[0-9]+)?(-(.*)+)?$", git_describe_version)
version_major = m.group(1) if m.group(1) is not None else "0"
version_minor = m.group(2).replace(".", "") if m.group(2) is not None else "0"
version_patch = m.group(3).replace(".", "") if m.group(3) is not None else "0"
version_label = m.group(4) if m.group(4) is not None else ""
#print("major {} minor {} patch {} label {}".format(version_major, version_minor, version_patch, version_label))

debian_distribution = "unstable"
if version_label is not "":
    debian_distribution = "UNRELEASED"

dirpath = os.path.dirname(os.path.realpath(__file__))
changelog_file = os.path.join(dirpath, "changelog")

# remove leading 'v'
changelog_version = git_describe_version[1:] if git_describe_version[0] == 'v' else git_describe_version

with open(changelog_file, 'r') as original: data = original.read()
with open(changelog_file, 'w') as modified:
    new_entry = """open62541 ({version}) {distribution}; urgency=medium

  * Full changelog is available here: https://github.com/open62541/open62541/blob/master/CHANGELOG

 -- open62541 Team <open62541-core@googlegroups.com>  {time}
""".format(version=changelog_version, time=formatdate(), distribution = debian_distribution)

    modified.write(new_entry + "\n" + data)
