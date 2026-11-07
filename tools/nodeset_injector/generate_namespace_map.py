#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Print a shared CMake namespace map for the bundled information models."""
from pathlib import Path
import sys
import xml.etree.ElementTree as ET


def namespace_map(directory):
    uris = set()
    for path in sorted(directory.rglob('*.xml')):
        with path.open('rb') as source:
            elements = ET.iterparse(source, events=('start', 'end'))
            _, root = next(elements)
            if root.tag.rsplit('}', 1)[-1] != 'UANodeSet':
                continue
            for event, element in elements:
                if event == 'end' and element.tag.rsplit('}', 1)[-1] == 'NamespaceUris':
                    uris.update(child.text for child in element if child.text)
                    break
    uris.discard('http://opcfoundation.org/UA/')
    # Namespace 1 is reserved for the server's application namespace.
    return ['0:http://opcfoundation.org/UA/'] + [
        '%d:%s' % (index, uri) for index, uri in enumerate(sorted(uris), 2)]


if __name__ == '__main__':
    print(';'.join(namespace_map(Path(sys.argv[1]))))
