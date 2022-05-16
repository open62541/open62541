#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2019 (c) Kalycito Infotech Private Limited
#

import netifaces
import sys
import os
import socket
import argparse

parser = argparse.ArgumentParser()

parser.add_argument('outdir',
                    type=str,
                    nargs='?',
                    default=os.getcwd(),
                    metavar='<OutputDirectory>')

parser.add_argument('-u', '--uri',
                    metavar="<ApplicationUri>",
                    type=str,
                    default="",
                    dest="uri")

parser.add_argument('-k', '--keysize',
                    metavar="<KeySize>",
                    type=int,
                    dest="keysize")

parser.add_argument('-c', '--certificatename',
                     metavar="<CertificateName>",
                     type=str,
                     default="",
                     dest="certificatename")

args = parser.parse_args()

if not os.path.exists(args.outdir):
    sys.exit('ERROR: Directory %s was not found!' % args.outdir)

keysize = 2048

if args.keysize:
    keysize = args.keysize

if args.uri == "":
    args.uri = "urn:open62541.server.application"
    print("No ApplicationUri given for the certificate. Setting to %s" % args.uri)
os.environ['URI1'] = args.uri

if args.certificatename == "":
    certificatename = "server"
    print("No Certificate name provided. Setting to %s" % certificatename)

if args.certificatename:
     certificatename = args.certificatename

certsdir = os.path.dirname(os.path.abspath(__file__))

# Function return TRUE (1) when an IP address is associated with the
# given interface
def is_interface_up(interface):
    addr = netifaces.ifaddresses(interface)
    return netifaces.AF_INET in addr

# Initialize looping variables
interfaceNum = 0
iteratorValue = 0

# Read the number of interfaces available
numberOfInterfaces = int(format(len(netifaces.interfaces())))

# Traverse through the available network interfaces and store the
# corresponding IP addresses of the network interface in a variable
for interfaceNum in range(0, numberOfInterfaces):
    # Function call which returns whether the given
    # interface is up or not
    check = is_interface_up(netifaces.interfaces()[interfaceNum])

    # Check if the interface is up and not the loopback one
    # If yes set the IP Address for the environmental variables
    if check != 0 and netifaces.interfaces()[interfaceNum] != 'lo':
        if iteratorValue == 0:
            os.environ['IPADDRESS1'] = netifaces.ifaddresses(netifaces.interfaces()[interfaceNum])[netifaces.AF_INET][0]['addr']
        if iteratorValue == 1:
            os.environ['IPADDRESS2'] = netifaces.ifaddresses(netifaces.interfaces()[interfaceNum])[netifaces.AF_INET][0]['addr']
        iteratorValue = iteratorValue + 1
        if iteratorValue == 2:
            break

# If there is only one interface available then set the second
# IP address as loopback IP
if iteratorValue < 2:
    os.environ['IPADDRESS2'] = "127.0.0.1"

os.environ['HOSTNAME'] = socket.gethostname()
openssl_conf = os.path.join(certsdir, "localhost.cnf")

os.chdir(os.path.abspath(args.outdir))

os.system("""openssl req \
     -config {} \
     -new \
     -nodes \
     -x509 -sha256  \
     -newkey rsa:{} \
     -keyout localhost.key -days 365 \
     -subj "/C=DE/L=Here/O=open62541/CN=open62541Server@localhost"\
     -out localhost.crt""".format(openssl_conf, keysize))
os.system("openssl x509 -in localhost.crt -outform der -out %s_cert.der" % (certificatename))
os.system("openssl rsa -inform PEM -in localhost.key -outform DER -out %s_key.der"% (certificatename))

os.remove("localhost.key")
os.remove("localhost.crt")

print("Certificates generated in " + args.outdir)
