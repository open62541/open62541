#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2019 (c) Kalycito Infotech Private Limited
# Modified 2025 (c) Construction Future Lab
# Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)

import netifaces
import sys
import os
import socket
import argparse
import subprocess

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

parser.add_argument('-e', '--ecc',
                     metavar="<EccCurve>",
                     type=str,
                     default="",
                     dest="ecc",
                     help="Generate an ECC certificate for the given curve. "
                          "Supported: prime256v1, secp384r1, brainpoolP256r1, "
                          "brainpoolP384r1, ed25519, ed448")

parser.add_argument('--ecc-all',
                     action="store_true",
                     default=False,
                     dest="ecc_all",
                     help="Generate ECC certificates for all six supported curves "
                          "(plus an RSA certificate). Output names follow the "
                          "server_c_<curve> convention.")

args = parser.parse_args()

if not os.path.exists(args.outdir):
    sys.exit('ERROR: Directory %s was not found!' % args.outdir)

keysize = 2048

if args.keysize:
    keysize = args.keysize

if args.uri == "":
    args.uri = "urn:open62541.unconfigured.application"
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

# Mapping from friendly curve name to (openssl curve / algorithm, is_eddsa, digest)
ECC_CURVES = {
    "prime256v1":      ("prime256v1",      False, "sha256"),
    "nistP256":        ("prime256v1",      False, "sha256"),
    "secp384r1":       ("secp384r1",       False, "sha384"),
    "nistP384":        ("secp384r1",       False, "sha384"),
    "brainpoolP256r1": ("brainpoolP256r1", False, "sha256"),
    "brainpoolP384r1": ("brainpoolP384r1", False, "sha384"),
    "ed25519":         ("ed25519",         True,  None),
    "curve25519":      ("ed25519",         True,  None),
    "ed448":           ("ed448",           True,  None),
    "curve448":        ("ed448",           True,  None),
}

# Map curve names to the server_c_<name> naming convention used by the examples
ECC_ALL_CURVES = [
    ("nistP256",        "prime256v1"),
    ("nistP384",        "secp384r1"),
    ("brainpoolP256r1", "brainpoolP256r1"),
    ("brainpoolP384r1", "brainpoolP384r1"),
    ("curve25519",      "ed25519"),
    ("curve448",        "ed448"),
]

def generate_rsa_cert(certname, keysize_bits, conf, subject):
    """Generate an RSA self-signed certificate in DER format."""
    subprocess.run([
        "openssl", "req",
        "-config", conf,
        "-new", "-nodes", "-x509", "-sha256",
        "-newkey", f"rsa:{keysize_bits}",
        "-keyout", "localhost.key",
        "-days", "365",
        "-subj", subject,
        "-out", "localhost.crt"
    ], check=True)

    subprocess.run([
        "openssl", "x509",
        "-in", "localhost.crt",
        "-outform", "der",
        "-out", f"{certname}.cert.der"
    ], check=True)

    subprocess.run([
        "openssl", "rsa",
        "-inform", "PEM",
        "-in", "localhost.key",
        "-outform", "DER",
        "-out", f"{certname}.key.der"
    ], check=True)

    print(f"RSA certificate: {certname}.cert.der / {certname}.key.der")

def generate_ecc_cert(certname, curve, conf, subject):
    """Generate an ECC self-signed certificate in DER format."""
    if curve not in ECC_CURVES:
        sys.exit(f"ERROR: Unknown ECC curve '{curve}'. "
                 f"Supported: {', '.join(ECC_CURVES.keys())}")

    openssl_curve, is_eddsa, digest = ECC_CURVES[curve]
    ecc_conf = os.path.join(os.path.dirname(conf), "localhost_ecc.cnf")

    # Generate the key
    if is_eddsa:
        subprocess.run([
            "openssl", "genpkey",
            "-algorithm", openssl_curve,
            "-out", "localhost.key"
        ], check=True)
    else:
        subprocess.run([
            "openssl", "ecparam",
            "-name", openssl_curve,
            "-genkey", "-noout",
            "-out", "localhost.key"
        ], check=True)

    # Build the certificate command
    req_cmd = [
        "openssl", "req",
        "-config", ecc_conf,
        "-new", "-nodes", "-x509",
        "-key", "localhost.key",
        "-days", "365",
        "-subj", subject,
        "-out", "localhost.crt"
    ]
    if digest:
        req_cmd.insert(req_cmd.index("-new"), f"-{digest}")
    subprocess.run(req_cmd, check=True)

    # Convert to DER
    subprocess.run([
        "openssl", "x509",
        "-in", "localhost.crt",
        "-outform", "der",
        "-out", f"{certname}.cert.der"
    ], check=True)

    subprocess.run([
        "openssl", "pkey",
        "-inform", "PEM",
        "-in", "localhost.key",
        "-outform", "DER",
        "-out", f"{certname}.key.der"
    ], check=True)

    print(f"ECC ({curve}) certificate: {certname}.cert.der / {certname}.key.der")

subject = "/C=DE/L=Here/O=open62541/CN=open62541Server@localhost"

# Use subprocess instead of os.system for better error handling
try:
    if args.ecc_all:
        # Generate RSA + all 6 ECC curves
        generate_rsa_cert(certificatename, keysize, openssl_conf, subject)
        for policy_name, curve in ECC_ALL_CURVES:
            generate_ecc_cert(f"server_c_{policy_name}", curve,
                              openssl_conf, subject)
    elif args.ecc:
        generate_ecc_cert(certificatename, args.ecc, openssl_conf, subject)
    else:
        generate_rsa_cert(certificatename, keysize, openssl_conf, subject)
    
except subprocess.CalledProcessError as e:
    sys.exit(f'ERROR: OpenSSL command failed: {e}')

# Clean up temp files
if os.path.exists("localhost.key"):
    os.remove("localhost.key")
if os.path.exists("localhost.crt"):
    os.remove("localhost.crt")

print("Certificates generated in " + args.outdir)
