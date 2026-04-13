#!/usr/bin/env bash
# Generate certificates for cross-SDK interoperability tests.
# Creates RSA certs for both C and .NET servers, and sets up
# mutual trust so each SDK trusts the other's certificate.
#
# Usage: ./generate_interop_certs.sh <output_dir>

set -euo pipefail

OUTDIR="${1:?Usage: $0 <output_dir>}"
mkdir -p "$OUTDIR"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# --- Helper: create a self-signed RSA cert + key ---
# Args: <name> <CN> <ApplicationUri> <outdir>
generate_rsa_cert() {
    local name="$1" cn="$2" uri="$3" dir="$4"
    local keyfile="$dir/${name}.key.pem"
    local certfile="$dir/${name}.cert.pem"

    # Generate key + self-signed cert (PEM)
    openssl req -x509 -newkey rsa:2048 -sha256 -nodes \
        -keyout "$keyfile" \
        -out "$certfile" \
        -days 365 \
        -subj "/C=DE/O=open62541/CN=${cn}" \
        -addext "subjectAltName=URI:${uri},DNS:localhost,IP:127.0.0.1" \
        -addext "basicConstraints=critical,CA:FALSE" \
        -addext "keyUsage=critical,digitalSignature,nonRepudiation,keyEncipherment,dataEncipherment" \
        -addext "extendedKeyUsage=serverAuth,clientAuth" \
        2>/dev/null

    # Convert to DER (for C SDK)
    openssl x509 -in "$certfile" -outform DER -out "$dir/${name}.cert.der"
    openssl rsa  -in "$keyfile"  -outform DER -out "$dir/${name}.key.der" 2>/dev/null

    echo "  Generated: ${name} (PEM + DER)"
}

echo "=== Generating interop test certificates ==="
echo "Output directory: $OUTDIR"

# --- C SDK server certificate ---
generate_rsa_cert "server_c" \
    "open62541 CI Server" \
    "urn:open62541.unconfigured.application" \
    "$OUTDIR"

# --- C SDK client certificate ---
generate_rsa_cert "client_c" \
    "open62541 CI Client" \
    "urn:open62541.client.application" \
    "$OUTDIR"

# --- .NET SDK server certificate ---
generate_rsa_cert "server_dotnet" \
    "OPC UA .NET Reference Server" \
    "urn:localhost:UA:ReferenceServer" \
    "$OUTDIR"

# --- .NET SDK client certificate ---
generate_rsa_cert "client_dotnet" \
    "OPC UA .NET Interop Client" \
    "urn:localhost:UA:InteropClient" \
    "$OUTDIR"

# --- node-opcua SDK server certificate ---
generate_rsa_cert "server_nodeopcua" \
    "node-opcua Interop Server" \
    "urn:localhost:node-opcua:interop" \
    "$OUTDIR"

# --- node-opcua SDK client certificate ---
generate_rsa_cert "client_nodeopcua" \
    "node-opcua Interop Client" \
    "urn:localhost:node-opcua:client" \
    "$OUTDIR"

# --- Set up trust directories ---
echo ""
echo "=== Setting up trust stores ==="

# The C SDK interop_server accepts trust list files as extra CLI args.
# The .NET SDK reads from a PKI directory structure.

# Create .NET PKI directories
DOTNET_PKI="$OUTDIR/dotnet_pki"
mkdir -p "$DOTNET_PKI/trusted/certs"
mkdir -p "$DOTNET_PKI/rejected/certs"
mkdir -p "$DOTNET_PKI/issuer/certs"
mkdir -p "$DOTNET_PKI/own/certs"
mkdir -p "$DOTNET_PKI/own/private"

# Copy .NET server's own cert + key
cp "$OUTDIR/server_dotnet.cert.der" "$DOTNET_PKI/own/certs/"
cp "$OUTDIR/server_dotnet.cert.pem" "$DOTNET_PKI/own/certs/"
cp "$OUTDIR/server_dotnet.key.pem"  "$DOTNET_PKI/own/private/"

# Trust the C SDK certificates
cp "$OUTDIR/server_c.cert.der" "$DOTNET_PKI/trusted/certs/"
cp "$OUTDIR/client_c.cert.der" "$DOTNET_PKI/trusted/certs/"

# Trust the .NET SDK's own certs (self-trust)
cp "$OUTDIR/server_dotnet.cert.der" "$DOTNET_PKI/trusted/certs/"
cp "$OUTDIR/client_dotnet.cert.der" "$DOTNET_PKI/trusted/certs/"

echo "  .NET PKI trust store: $DOTNET_PKI"
echo "  Trusted certs (RSA): $(ls "$DOTNET_PKI/trusted/certs/" | wc -l) files"

# ---- node-opcua PKI directory structure ----
NODE_PKI="$OUTDIR/node_pki"
mkdir -p "$NODE_PKI/trusted/certs"
mkdir -p "$NODE_PKI/rejected/certs"
mkdir -p "$NODE_PKI/own/certs"
mkdir -p "$NODE_PKI/own/private"

# node-opcua server's own certificate and key
cp "$OUTDIR/server_nodeopcua.cert.pem" "$NODE_PKI/own/certs/"
cp "$OUTDIR/server_nodeopcua.key.pem"  "$NODE_PKI/own/private/"

# Trust the C SDK client certificate so the server accepts T-9 X509 auth
cp "$OUTDIR/client_c.cert.der" "$NODE_PKI/trusted/certs/"
cp "$OUTDIR/client_c.cert.pem" "$NODE_PKI/trusted/certs/"

echo "  node-opcua PKI trust store: $NODE_PKI"

# ============================================================
# ECC certificates (for ECC security policy interop tests)
# ============================================================

echo ""
echo "=== Generating ECC certificates ==="

ECC_DIR="$OUTDIR/ecc"
mkdir -p "$ECC_DIR"

# --- Helper: create a self-signed ECC cert (ECDSA curves) ---
# Args: <name> <curve> <sha_digest> <CN> <ApplicationUri>
#   sha_digest: sha256 for P256 curves, sha384 for P384 curves
generate_ecc_cert() {
    local name="$1" curve="$2" sha="$3" cn="$4" uri="$5"
    local keyfile="$ECC_DIR/${name}.key.pem"
    local certfile="$ECC_DIR/${name}.cert.pem"

    openssl ecparam -name "$curve" -genkey -noout -out "$keyfile" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  SKIP: $name ($curve not supported by this OpenSSL)"
        return 1
    fi

    openssl req -new -x509 -key "$keyfile" \
        -out "$certfile" \
        -days 365 -"$sha" \
        -subj "/C=DE/O=open62541/CN=${cn}" \
        -addext "subjectAltName=URI:${uri},DNS:localhost,IP:127.0.0.1" \
        -addext "basicConstraints=critical,CA:TRUE" \
        -addext "keyUsage=critical,digitalSignature,nonRepudiation,keyAgreement,keyCertSign" \
        -addext "extendedKeyUsage=serverAuth,clientAuth" \
        2>/dev/null

    openssl x509 -in "$certfile" -outform DER -out "$ECC_DIR/${name}.cert.der"
    openssl ec -in "$keyfile" -outform DER -out "$ECC_DIR/${name}.key.der" 2>/dev/null

    echo "  Generated: ${name} ($curve, $sha)"
    return 0
}

# --- Helper: create a self-signed EdDSA cert (Ed25519/Ed448) ---
# Args: <name> <algorithm> <CN> <ApplicationUri>
generate_eddsa_cert() {
    local name="$1" algo="$2" cn="$3" uri="$4"
    local keyfile="$ECC_DIR/${name}.key.pem"
    local certfile="$ECC_DIR/${name}.cert.pem"

    openssl genpkey -algorithm "$algo" -out "$keyfile" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  SKIP: $name ($algo not supported by this OpenSSL)"
        return 1
    fi

    openssl req -new -x509 -key "$keyfile" \
        -out "$certfile" \
        -days 365 \
        -subj "/C=DE/O=open62541/CN=${cn}" \
        -addext "subjectAltName=URI:${uri},DNS:localhost,IP:127.0.0.1" \
        2>/dev/null

    openssl x509 -in "$certfile" -outform DER -out "$ECC_DIR/${name}.cert.der"
    openssl pkey -in "$keyfile" -outform DER -out "$ECC_DIR/${name}.key.der" 2>/dev/null

    echo "  Generated: ${name} ($algo)"
    return 0
}

# ECC_nistP256 (prime256v1 / secp256r1)
# ApplicationUri must match the RSA certs so that the C server/client
# ApplicationDescription.applicationUri stays consistent.
generate_ecc_cert "server_c_nistP256" prime256v1 sha256 \
    "open62541 Server ECC" "urn:open62541.unconfigured.application"
generate_ecc_cert "client_c_nistP256" prime256v1 sha256 \
    "open62541 Client ECC" "urn:open62541.client.application"

# ECC_nistP384 (secp384r1) – must use SHA-384 per OPC UA ECC_nistP384 policy
generate_ecc_cert "server_c_nistP384" secp384r1 sha384 \
    "open62541 Server ECC" "urn:open62541.unconfigured.application"
generate_ecc_cert "client_c_nistP384" secp384r1 sha384 \
    "open62541 Client ECC" "urn:open62541.client.application"

# ECC_brainpoolP256r1
generate_ecc_cert "server_c_brainpoolP256r1" brainpoolP256r1 sha256 \
    "open62541 Server ECC" "urn:open62541.unconfigured.application"
generate_ecc_cert "client_c_brainpoolP256r1" brainpoolP256r1 sha256 \
    "open62541 Client ECC" "urn:open62541.client.application"

# ECC_brainpoolP384r1 – must use SHA-384 per OPC UA ECC_brainpoolP384r1 policy
generate_ecc_cert "server_c_brainpoolP384r1" brainpoolP384r1 sha384 \
    "open62541 Server ECC" "urn:open62541.unconfigured.application"
generate_ecc_cert "client_c_brainpoolP384r1" brainpoolP384r1 sha384 \
    "open62541 Client ECC" "urn:open62541.client.application"

# ECC_curve25519 (Ed25519)
generate_eddsa_cert "server_c_curve25519" Ed25519 \
    "open62541 Server ECC" "urn:open62541.unconfigured.application"
generate_eddsa_cert "client_c_curve25519" Ed25519 \
    "open62541 Client ECC" "urn:open62541.client.application"

# ECC_curve448 (Ed448)
generate_eddsa_cert "server_c_curve448" Ed448 \
    "open62541 Server ECC" "urn:open62541.unconfigured.application"
generate_eddsa_cert "client_c_curve448" Ed448 \
    "open62541 Client ECC" "urn:open62541.client.application"

# --- Copy C client ECC certs into .NET PKI trust store ---
# The .NET Reference Server supports ECC (nistP256, nistP384,
# brainpoolP256r1, brainpoolP384r1) and auto-generates its own ECC
# server certs.  For the C client to be accepted by the .NET server
# in Scenario B, the .NET server must trust the C client's ECC certs.
echo ""
echo "=== Adding ECC certs to .NET PKI trust store ==="
for eccCert in "$ECC_DIR"/client_c_*.cert.der; do
    [ -f "$eccCert" ] || continue
    cp "$eccCert" "$DOTNET_PKI/trusted/certs/"
    echo "  Trusted: $(basename "$eccCert")"
done
echo "  .NET PKI trusted certs total: $(ls "$DOTNET_PKI/trusted/certs/" | wc -l) files"

echo ""
echo "=== Certificate generation complete ==="
echo ""
echo "Files generated:"
ls -la "$OUTDIR"/*.pem "$OUTDIR"/*.der 2>/dev/null | awk '{print "  " $NF}'
echo "ECC certificates:"
ls -la "$ECC_DIR"/ 2>/dev/null | awk '{print "  " $NF}'
