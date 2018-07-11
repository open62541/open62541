#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this 
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import os
import shutil
import socket

if len(sys.argv) < 2:
    sys.exit('Usage: %s directory to output certificates' % sys.argv[0])

if not os.path.exists(sys.argv[1]):
    sys.exit('ERROR: Directory %s was not found!' % sys.argv[1])

keysize = 2048

if len(sys.argv) == 3:
    keysize = int(sys.argv[2])

certsdir = os.path.dirname(os.path.abspath(__file__))
print(certsdir)
os.environ['HOSTNAME'] = socket.gethostname()
openssl_conf = os.path.join(certsdir, "localhost.cnf")

os.chdir(os.path.abspath(sys.argv[1]))

os.system("""openssl req \
     -config {} \
     -new \
     -nodes \
     -x509 -sha256  \
     -newkey rsa:{} \
     -keyout localhost.key -days 365 \
     -subj "/C=DE/O=open62541/CN=open62541Server@localhost"\
     -out localhost.crt""".format(openssl_conf, keysize))

os.system("openssl x509 -in localhost.crt -outform der -out server_cert.der")
os.system("openssl rsa -inform PEM -in localhost.key -outform DER -out server_key.der")

os.remove("localhost.key")
os.remove("localhost.crt")

print("Certificates generated in " + sys.argv[1])
