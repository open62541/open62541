#!/bin/sh
set -e

CERT_DIR="/opt/open62541/pki/created"
CERT_FILE="${CERT_DIR}/server.cert.der"
KEY_FILE="${CERT_DIR}/server.key.der"

# Ensure the output directory exists
mkdir -p "${CERT_DIR}"

if [ ! -f "${CERT_FILE}" ] || [ ! -f "${KEY_FILE}" ]; then
    echo "No existing certificate or private key found. Provisioning a new self-signed certificate..."

    # Use environment variables if provided, otherwise resolve defaults dynamically
    RESOLVED_HOSTNAME="${CERT_HOSTNAME:-$(hostname)}"
    # Resolve first non-loopback IP address
    RESOLVED_IP1="${CERT_IP1:-$(hostname -i | awk '{print $1}')}"
    RESOLVED_IP2="${CERT_IP2:-127.0.0.1}"
    RESOLVED_URI="${CERT_URI:-urn:open62541.unconfigured.application}"

    echo "Using certificate SAN options:"
    echo "  - Hostname/DNS: ${RESOLVED_HOSTNAME}"
    echo "  - IP 1: ${RESOLVED_IP1}"
    echo "  - IP 2: ${RESOLVED_IP2}"
    echo "  - URI: ${RESOLVED_URI}"

    python3 /usr/local/share/open62541/certs/create_self-signed.py \
        -u "${RESOLVED_URI}" \
        --hostname "${RESOLVED_HOSTNAME}" \
        --ipaddress1 "${RESOLVED_IP1}" \
        --ipaddress2 "${RESOLVED_IP2}" \
        "${CERT_DIR}"
else
    echo "Existing certificate and private key found. Skipping provisioning."
fi

# Detect if a GDS-pushed certificate and private key already exist in the persistent File PKI store
GDS_OWN_CERT_DIR="/opt/open62541/pki/ApplCerts/own/certs"
GDS_OWN_KEY_DIR="/opt/open62541/pki/ApplCerts/own/private"

# If we find files inside the GDS 'own' folders, use them instead of the default self-signed certs!
GDS_CERT=$(find "${GDS_OWN_CERT_DIR}" -type f -name "*.der" | head -n 1)
GDS_KEY=$(find "${GDS_OWN_KEY_DIR}" -type f -name "*.der" | head -n 1)

if [ -n "${GDS_CERT}" ] && [ -n "${GDS_KEY}" ] && [ -f "${GDS_CERT}" ] && [ -f "${GDS_KEY}" ]; then
    echo "GDS-pushed persistent certificate and private key detected!"
    echo "  - Loading active cert: ${GDS_CERT}"
    echo "  - Loading active key:  ${GDS_KEY}"
    ACTIVE_CERT_FILE="${GDS_CERT}"
    ACTIVE_KEY_FILE="${GDS_KEY}"
else
    echo "No GDS-pushed certificate detected. Loading default self-signed certificate."
    ACTIVE_CERT_FILE="${CERT_FILE}"
    ACTIVE_KEY_FILE="${KEY_FILE}"
fi

# Execute the main application command (or fallback if empty)
if [ "$#" -eq 0 ]; then
    exec /opt/open62541/examples/ci_server "4840" "${ACTIVE_CERT_FILE}" "${ACTIVE_KEY_FILE}"
else
    exec "$@"
fi
