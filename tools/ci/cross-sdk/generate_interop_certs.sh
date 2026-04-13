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
echo "  Trusted certs: $(ls "$DOTNET_PKI/trusted/certs/" | wc -l) files"

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

echo ""
echo "=== Certificate generation complete ==="
echo ""
echo "Files generated:"
ls -la "$OUTDIR"/*.pem "$OUTDIR"/*.der 2>/dev/null | awk '{print "  " $NF}'
