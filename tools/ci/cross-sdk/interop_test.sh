#!/usr/bin/env bash
# Cross-SDK interoperability test orchestration script.
#
# Every scenario involves the C (open62541) SDK as server or client.
#
#   Scenario A: C server        <-->  C client  (self-test)
#   Scenario A: C server        <-->  .NET client
#   Scenario H1: C HTTPS server <-->  .NET HTTPS client
#   Scenario A: C server        <-->  node-opcua client
#   Scenario B: .NET server     <-->  C client
#   Scenario H2: .NET HTTPS srv <-->  C HTTPS client
#   Scenario C: node-opcua srv  <-->  C client
#
# Prerequisites:
#   - C SDK built with encryption (interop_server, check_interop_client)
#   - .NET SDK restored and built
#   - Certificates generated (via generate_interop_certs.sh)
#
# Usage: ./interop_test.sh <c_build_dir> <dotnet_sdk_dir> <cert_dir>
#
# Set INTEROP_ENABLE_WSS=1 to repeat the existing C and .NET client suites
# over opc.wss. This remains opt-in until a released .NET SDK supports WSS.
# Set INTEROP_ENABLE_NODE_WSS=1 to run the node-opcua client suite over WSS.

set -euo pipefail

C_BUILD_DIR="${1:?Usage: $0 <c_build_dir> <dotnet_sdk_dir> <cert_dir>}"
DOTNET_SDK_DIR="${2:?Usage: $0 <c_build_dir> <dotnet_sdk_dir> <cert_dir>}"
CERT_DIR="${3:?Usage: $0 <c_build_dir> <dotnet_sdk_dir> <cert_dir>}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

CI_SERVER="$C_BUILD_DIR/bin/tests/interop_server"
INTEROP_CLIENT="$C_BUILD_DIR/bin/tests/check_interop_client"
CI_WSS_SERVER="$C_BUILD_DIR/bin/tests/interop_server_wss"
INTEROP_WSS_CLIENT="$C_BUILD_DIR/bin/tests/check_interop_client_wss"
CI_HTTPS_SERVER="$C_BUILD_DIR/bin/tests/interop_server_https"
INTEROP_HTTPS_CLIENT="$C_BUILD_DIR/bin/tests/check_interop_client_https"
DOTNET_INTEROP_PROJECT="$REPO_ROOT/tests/interop/dotnet/Opc.Ua.Interop.Tests.csproj"
DOTNET_SERVER_PROJECT="$DOTNET_SDK_DIR/Applications/ConsoleReferenceServer/ConsoleReferenceServer.csproj"
if [[ ! -f "$DOTNET_SERVER_PROJECT" ]]; then
    DOTNET_SERVER_PROJECT="$DOTNET_SDK_DIR/samples/ConsoleReferenceServer/ConsoleReferenceServer.csproj"
fi
NODEOPCUA_CLIENT_DIR="$REPO_ROOT/tests/interop/node-opcua"
NODEOPCUA_WSS_CLIENT="$NODEOPCUA_CLIENT_DIR/websocket/client_wss.mjs"
INTEROP_ENABLE_WSS="${INTEROP_ENABLE_WSS:-0}"
INTEROP_ENABLE_NODE_WSS="${INTEROP_ENABLE_NODE_WSS:-0}"

# Local interoperability traffic must not be routed through a CI proxy.
export NO_PROXY="${NO_PROXY:+$NO_PROXY,}localhost,127.0.0.1,::1"
export no_proxy="${no_proxy:+$no_proxy,}localhost,127.0.0.1,::1"

RESULT=0
C_SERVER_PID=""
C_WSS_SERVER_PID=""
C_HTTPS_SERVER_PID=""
DOTNET_SERVER_PID=""
NODEOPCUA_SERVER_PID=""

# Send SIGTERM, wait up to 5 s, then SIGKILL.  Also kills child processes
# (handles the subshell + dotnet-run process tree).
stop_server() {
    local pid="$1"
    local label="${2:-server}"
    if [[ -z "$pid" ]] || ! kill -0 "$pid" 2>/dev/null; then
        return 0
    fi
    echo "  Stopping $label (PID $pid)"
    # Kill child processes first (e.g. dotnet exec spawned by dotnet run)
    pkill -P "$pid" 2>/dev/null || true
    kill "$pid" 2>/dev/null || true
    # Wait up to 5 s for graceful shutdown
    local i
    for i in 1 2 3 4 5; do
        kill -0 "$pid" 2>/dev/null || break
        sleep 1
    done
    # Force-kill if still alive
    if kill -0 "$pid" 2>/dev/null; then
        echo "  Force-killing $label (PID $pid)"
        pkill -9 -P "$pid" 2>/dev/null || true
        kill -9 "$pid" 2>/dev/null || true
    fi
    wait "$pid" 2>/dev/null || true
}

cleanup() {
    echo ""
    echo "=== Cleaning up ==="
    stop_server "$C_SERVER_PID" "C server"
    stop_server "$C_WSS_SERVER_PID" "C WSS server"
    stop_server "$C_HTTPS_SERVER_PID" "C HTTPS server"
    stop_server "$DOTNET_SERVER_PID" ".NET server"
    stop_server "$NODEOPCUA_SERVER_PID" "node-opcua server"
}
trap cleanup EXIT

wait_for_server() {
    local url="$1"
    local timeout="${2:-30}"
    local port="${url##*:}"
    local host="${url%:*}"
    local start=$SECONDS
    echo "  Waiting for server at $url (timeout: ${timeout}s)..."
    while (( SECONDS - start < timeout )); do
        if nc -z "$host" "$port" 2>/dev/null; then
            echo "  Server is ready (took $(( SECONDS - start ))s)"
            return 0
        fi
        sleep 1
    done
    echo "  ERROR: Server did not start within ${timeout}s"
    return 1
}

# Wait for a background process to print a specific marker on stdout.
# The process output is redirected to a log file which is polled.
wait_for_log_marker() {
    local logfile="$1"
    local marker="$2"
    local timeout="${3:-60}"
    local start=$SECONDS
    echo "  Waiting for \"$marker\" in server output (timeout: ${timeout}s)..."
    while (( SECONDS - start < timeout )); do
        if grep -qF "$marker" "$logfile" 2>/dev/null; then
            echo "  Server is ready (took $(( SECONDS - start ))s)"
            return 0
        fi
        sleep 1
    done
    echo "  ERROR: Marker not found within ${timeout}s"
    echo "  Server output so far:"
    cat "$logfile" 2>/dev/null || true
    return 1
}

# ============================================================
# Verify prerequisites
# ============================================================

echo "=== Cross-SDK Interoperability Tests ==="
echo "  C build dir:    $C_BUILD_DIR"
echo "  .NET SDK dir:   $DOTNET_SDK_DIR"
echo "  Cert dir:       $CERT_DIR"
echo ""

for f in "$CI_SERVER" "$INTEROP_CLIENT" \
         "$CI_HTTPS_SERVER" "$INTEROP_HTTPS_CLIENT"; do
    if [[ ! -x "$f" ]]; then
        echo "ERROR: Missing executable: $f"
        exit 1
    fi
done
if [[ "$INTEROP_ENABLE_WSS" != "0" || "$INTEROP_ENABLE_NODE_WSS" != "0" ]]; then
    if [[ ! -x "$CI_WSS_SERVER" ]]; then
        echo "ERROR: Missing WSS executable: $CI_WSS_SERVER"
        exit 1
    fi
fi
if [[ "$INTEROP_ENABLE_WSS" != "0" && ! -x "$INTEROP_WSS_CLIENT" ]]; then
    echo "ERROR: Missing WSS executable: $INTEROP_WSS_CLIENT"
    exit 1
fi
if [[ "$INTEROP_ENABLE_NODE_WSS" != "0" && ! -f "$NODEOPCUA_WSS_CLIENT" ]]; then
    echo "ERROR: Missing node-opcua WSS client: $NODEOPCUA_WSS_CLIENT"
    exit 1
fi

REQUIRED_CERTS=(
    server_c.cert.der server_c.key.der
    client_c.cert.der client_c.key.der client_c.cert.pem client_c.key.pem
    client_dotnet.cert.der
    client_nodeopcua.cert.der
    server_dotnet.cert.der
    server_nodeopcua.cert.pem server_nodeopcua.key.pem server_nodeopcua.cert.der
    client_nodeopcua.cert.pem client_nodeopcua.key.pem
    server_c.cert.pem
)
for f in "${REQUIRED_CERTS[@]}"; do
    if [[ ! -f "$CERT_DIR/$f" ]]; then
        echo "ERROR: Missing certificate: $CERT_DIR/$f"
        exit 1
    fi
done

# Export ECC certificate directory for C server & client (both read this env var)
if [[ -d "$CERT_DIR/ecc" ]]; then
    export INTEROP_ECC_CERT_DIR="$CERT_DIR/ecc"
    echo "  ECC cert dir:   $INTEROP_ECC_CERT_DIR"
fi

# ============================================================
# Scenario A: C server <--> C, .NET & node-opcua clients
# ============================================================

echo ""
echo "=========================================="
echo "  Scenario A: C server <--> C, .NET & node-opcua clients"
echo "=========================================="
echo ""

C_PORT=4840
C_WSS_PORT=4843
C_WSS_URL="opc.wss://localhost:$C_WSS_PORT/interop"
C_LOG="$(mktemp)"
echo "Starting C server on port $C_PORT..."
"$CI_SERVER" "$C_PORT" \
    "$CERT_DIR/server_c.cert.der" \
    "$CERT_DIR/server_c.key.der" \
    "$CERT_DIR/client_c.cert.der" \
    "$CERT_DIR/client_dotnet.cert.der" \
    "$CERT_DIR/client_nodeopcua.cert.der" > >(tee "$C_LOG") 2>&1 &
C_SERVER_PID=$!

if ! wait_for_server "localhost:$C_PORT"; then
    echo "FAIL: C server did not start"
    RESULT=1
else
    echo "Running C interop client against C server (self-test)..."
    # Set when the C server offers ECC. Honoured by both the C and .NET
    # clients so neither side can silently skip all ECC tests.
    if grep -q "Added ECC policy" "$C_LOG"; then
        export INTEROP_REQUIRE_ECC=1
    fi
    if "$INTEROP_CLIENT" \
        "opc.tcp://localhost:$C_PORT" \
        "$CERT_DIR/client_c.cert.der" \
        "$CERT_DIR/client_c.key.der" \
        "$CERT_DIR/server_c.cert.der" 2>&1; then
        echo "PASS: Scenario A - C client self-test passed"
    else
        echo "FAIL: Scenario A - C client self-test failed"
        RESULT=1
    fi

    echo ""
    echo "Running .NET interop tests against C server..."
    export OPCUA_INTEROP_SERVER_URL="opc.tcp://localhost:$C_PORT"
    export OPCUA_INTEROP_CERT_DIR="$CERT_DIR"
    if dotnet test "$DOTNET_INTEROP_PROJECT" --no-build --verbosity normal \
         --configuration "${DOTNET_CONFIG:-Debug}" \
         --filter "Category=Interop" 2>&1; then
        echo "PASS: Scenario A - .NET client tests passed"
    else
        echo "FAIL: Scenario A - .NET client tests failed"
        RESULT=1
    fi
    unset OPCUA_INTEROP_SERVER_URL
    unset OPCUA_INTEROP_CERT_DIR
    unset INTEROP_REQUIRE_ECC 2>/dev/null || true

    if [[ "$INTEROP_ENABLE_WSS" != "0" || "$INTEROP_ENABLE_NODE_WSS" != "0" ]]; then
        echo ""
        C_WSS_LOG="$(mktemp)"
        echo "Starting C WSS server on port $C_WSS_PORT..."
        "$CI_WSS_SERVER" "$C_WSS_PORT" \
            "$CERT_DIR/server_c.cert.der" \
            "$CERT_DIR/server_c.key.der" \
            "$CERT_DIR/client_c.cert.der" \
            "$CERT_DIR/client_dotnet.cert.der" \
            "$CERT_DIR/client_nodeopcua.cert.der" > >(tee "$C_WSS_LOG") 2>&1 &
        C_WSS_SERVER_PID=$!

        if ! wait_for_server "localhost:$C_WSS_PORT"; then
            echo "FAIL: C WSS server did not start"
            RESULT=1
        else
            if [[ "$INTEROP_ENABLE_WSS" != "0" ]]; then
                echo "Running WSS C interop client tests..."
                if "$INTEROP_WSS_CLIENT" \
                    "$C_WSS_URL" \
                    "$CERT_DIR/client_c.cert.der" \
                    "$CERT_DIR/client_c.key.der" \
                    "$CERT_DIR/server_c.cert.der" 2>&1; then
                    echo "PASS: Scenario A - C client WSS tests passed"
                else
                    echo "FAIL: Scenario A - C client WSS tests failed"
                    RESULT=1
                fi

                echo ""
                echo "Running experimental .NET interop client tests over WSS..."
                export OPCUA_INTEROP_WSS_SERVER_URL="$C_WSS_URL"
                export OPCUA_INTEROP_CERT_DIR="$CERT_DIR"
                if dotnet test "$DOTNET_INTEROP_PROJECT" --no-build --verbosity normal \
                     --configuration "${DOTNET_CONFIG:-Debug}" \
                     --filter "FullyQualifiedName~ExperimentalWssInteropClientTest" \
                     2>&1; then
                    echo "PASS: Scenario A - .NET client WSS tests passed"
                else
                    echo "FAIL: Scenario A - .NET client WSS tests failed"
                    RESULT=1
                fi
                unset OPCUA_INTEROP_WSS_SERVER_URL
                unset OPCUA_INTEROP_CERT_DIR
            fi

            if [[ "$INTEROP_ENABLE_NODE_WSS" != "0" ]]; then
                echo ""
                echo "Running node-opcua interop client tests over WSS..."
                if node "$NODEOPCUA_WSS_CLIENT" \
                    "$C_WSS_URL" \
                    "$CERT_DIR/client_nodeopcua.cert.pem" \
                    "$CERT_DIR/client_nodeopcua.key.pem" \
                    "$CERT_DIR/server_c.cert.pem" 2>&1; then
                    echo "PASS: Scenario A - node-opcua client WSS tests passed"
                else
                    echo "FAIL: Scenario A - node-opcua client WSS tests failed"
                    RESULT=1
                fi
            fi
        fi
        stop_server "$C_WSS_SERVER_PID" "C WSS server"
        C_WSS_SERVER_PID=""
    fi

    echo ""
    echo "Running node-opcua interop client against C server..."
    if node "$NODEOPCUA_CLIENT_DIR/client.mjs" \
        "opc.tcp://localhost:$C_PORT" \
        "$CERT_DIR/client_nodeopcua.cert.pem" \
        "$CERT_DIR/client_nodeopcua.key.pem" \
        "$CERT_DIR/server_c.cert.pem" 2>&1; then
        echo "PASS: Scenario A - node-opcua client tests passed"
    else
        echo "FAIL: Scenario A - node-opcua client tests failed"
        RESULT=1
    fi
fi

# Stop C server
stop_server "$C_SERVER_PID" "C server"
C_SERVER_PID=""

# ============================================================
# Scenario H1: C HTTPS server <--> .NET HTTPS client
# ============================================================

echo ""
echo "=========================================="
echo "  Scenario H1: C HTTPS server <--> .NET HTTPS client"
echo "=========================================="
echo ""

C_HTTPS_PORT=4844
C_HTTPS_URL="opc.https://localhost:$C_HTTPS_PORT/interop"
C_HTTPS_LOG="$(mktemp)"
echo "Starting C HTTPS server on port $C_HTTPS_PORT..."
"$CI_HTTPS_SERVER" "$C_HTTPS_PORT" \
    "$CERT_DIR/server_c.cert.der" \
    "$CERT_DIR/server_c.key.der" \
    "$CERT_DIR/client_c.cert.der" \
    "$CERT_DIR/client_dotnet.cert.der" > >(tee "$C_HTTPS_LOG") 2>&1 &
C_HTTPS_SERVER_PID=$!

if ! wait_for_server "localhost:$C_HTTPS_PORT"; then
    echo "FAIL: C HTTPS server did not start"
    RESULT=1
else
    echo "Running .NET HTTPS anonymous/username subscription matrix..."
    export OPCUA_INTEROP_HTTPS_SERVER_URL="$C_HTTPS_URL"
    if dotnet test "$DOTNET_INTEROP_PROJECT" --no-build --verbosity normal \
         --configuration "${DOTNET_CONFIG:-Debug}" \
         --filter "Category=HttpInterop" 2>&1; then
        echo "PASS: Scenario H1 - .NET HTTPS client matrix passed"
    else
        echo "FAIL: Scenario H1 - .NET HTTPS client matrix failed"
        RESULT=1
    fi
    unset OPCUA_INTEROP_HTTPS_SERVER_URL
fi
stop_server "$C_HTTPS_SERVER_PID" "C HTTPS server"
C_HTTPS_SERVER_PID=""

# ============================================================
# Scenario B: .NET server <--> C client
# ============================================================

echo ""
echo "=========================================="
echo "  Scenario B: .NET server <--> C client"
echo "=========================================="
echo ""

DOTNET_PORT=62541
DOTNET_LOG="$(mktemp)"
echo "Starting .NET Reference Server on port $DOTNET_PORT..."
DOTNET_SERVER_DIR="$(dirname "$DOTNET_SERVER_PROJECT")"
(cd "$DOTNET_SERVER_DIR" && dotnet run --project "$DOTNET_SERVER_PROJECT" --no-build \
    --framework net9.0 \
    --configuration "${DOTNET_CONFIG:-Debug}" -- -a -c) >"$DOTNET_LOG" 2>&1 &
DOTNET_SERVER_PID=$!

# The .NET Reference Server opens its TCP port well before the OPC UA stack
# is fully initialised (BadServerHalted during early requests).  Wait for the
# "Server started" message which is printed after full initialisation.
if ! wait_for_log_marker "$DOTNET_LOG" "Server started" 60; then
    echo "FAIL: .NET server did not start"
    RESULT=1
else
    echo "Running C interop client against .NET server..."
    # The .NET Reference Server advertises ECC (nistP256, nistP384,
    # brainpoolP256r1, brainpoolP384r1) but its P384 endpoints reject
    # connections with BadSecurityChecksFailed.  Do NOT set
    # INTEROP_REQUIRE_ECC here – P256 tests still pass and P384 will
    # gracefully skip instead of failing the CI.
    # C client auto-tests all policies when certs are provided
    if "$INTEROP_CLIENT" \
        "opc.tcp://localhost:$DOTNET_PORT" \
        "$CERT_DIR/client_c.cert.der" \
        "$CERT_DIR/client_c.key.der" \
        "$CERT_DIR/server_dotnet.cert.der" 2>&1; then
        echo "PASS: Scenario B - C client tests passed"
    else
        echo "FAIL: Scenario B - C client tests failed"
        RESULT=1
    fi

    echo ""
    echo "Running C HTTPS anonymous/username subscription matrix..."
    DOTNET_HTTPS_URL="$(grep -m1 '^opc\.https://' "$DOTNET_LOG" | \
        sed 's:/*$::' || true)"
    DOTNET_TLS_CERT="$CERT_DIR/server_dotnet_tls.cert.der"
    if [[ -z "$DOTNET_HTTPS_URL" ]]; then
        echo "FAIL: .NET server did not advertise an HTTPS endpoint"
        RESULT=1
    else
        DOTNET_HTTPS_AUTHORITY="${DOTNET_HTTPS_URL#opc.https://}"
        DOTNET_HTTPS_AUTHORITY="${DOTNET_HTTPS_AUTHORITY%%/*}"
        DOTNET_HTTPS_HOST="${DOTNET_HTTPS_AUTHORITY%:*}"
        if ! wait_for_server "$DOTNET_HTTPS_AUTHORITY"; then
            echo "FAIL: .NET HTTPS server did not start"
            RESULT=1
        elif ! openssl s_client -connect "$DOTNET_HTTPS_AUTHORITY" \
                 -servername "$DOTNET_HTTPS_HOST" \
                 -cert "$CERT_DIR/client_c.cert.pem" \
                 -key "$CERT_DIR/client_c.key.pem" </dev/null 2>/dev/null | \
                 openssl x509 -outform DER -out "$DOTNET_TLS_CERT"; then
            echo "FAIL: Could not obtain the .NET HTTPS TLS certificate"
            RESULT=1
        elif "$INTEROP_HTTPS_CLIENT" \
            "$DOTNET_HTTPS_URL" \
            "$CERT_DIR/client_c.cert.der" \
            "$CERT_DIR/client_c.key.der" \
            "$DOTNET_TLS_CERT" 2>&1; then
            echo "PASS: Scenario H2 - C HTTPS client matrix passed"
        else
            echo "FAIL: Scenario H2 - C HTTPS client matrix failed"
            RESULT=1
        fi
    fi

    if [[ "$INTEROP_ENABLE_WSS" != "0" ]]; then
        echo ""
        echo "Running existing C interop client tests against .NET over WSS..."
        DOTNET_WSS_URL="opc.wss://localhost:62543/Quickstarts/ReferenceServer"
        if ! wait_for_server "localhost:62543"; then
            echo "FAIL: .NET WSS server did not start"
            RESULT=1
        elif "$INTEROP_WSS_CLIENT" \
            "$DOTNET_WSS_URL" \
            "$CERT_DIR/client_c.cert.der" \
            "$CERT_DIR/client_c.key.der" \
            "$CERT_DIR/server_dotnet.cert.der" 2>&1; then
            echo "PASS: Scenario B - C client WSS tests passed"
        else
            echo "FAIL: Scenario B - C client WSS tests failed"
            RESULT=1
        fi
    fi
fi

# Stop .NET server
stop_server "$DOTNET_SERVER_PID" ".NET server"
DOTNET_SERVER_PID=""

# ============================================================
# Scenario C: node-opcua server <--> C client
# ============================================================

echo ""
echo "=========================================="
echo "  Scenario C: node-opcua server <--> C client"
echo "=========================================="
echo ""

NODEOPCUA_PORT=62542
NODEOPCUA_SERVER_DIR="$REPO_ROOT/tests/interop/node-opcua"
NODE_PKI="$CERT_DIR/node_pki"
NODEOPCUA_LOG="$(mktemp)"
echo "Starting node-opcua server on port $NODEOPCUA_PORT..."
node "$NODEOPCUA_SERVER_DIR/server.mjs" \
    "$NODEOPCUA_PORT" \
    "$CERT_DIR/server_nodeopcua.cert.pem" \
    "$CERT_DIR/server_nodeopcua.key.pem" \
    "$NODE_PKI" >"$NODEOPCUA_LOG" 2>&1 &
NODEOPCUA_SERVER_PID=$!

if ! wait_for_log_marker "$NODEOPCUA_LOG" "node-opcua interop server listening" 30; then
    echo "FAIL: node-opcua server did not start"
    echo "  Server output:"
    cat "$NODEOPCUA_LOG" 2>/dev/null || true
    RESULT=1
else
    echo "Running C interop client against node-opcua server..."
    # Policies not offered by node-opcua (e.g. Aes128/Aes256_RsaPss)
    # are automatically SKIP-ped by the C client (BadSecurityPolicyRejected).
    if "$INTEROP_CLIENT" \
        "opc.tcp://localhost:$NODEOPCUA_PORT" \
        "$CERT_DIR/client_c.cert.der" \
        "$CERT_DIR/client_c.key.der" \
        "$CERT_DIR/server_nodeopcua.cert.der" 2>&1; then
        echo "PASS: Scenario C - C client tests passed"
    else
        echo "FAIL: Scenario C - C client tests failed"
        RESULT=1
    fi
fi

# Stop node-opcua server
stop_server "$NODEOPCUA_SERVER_PID" "node-opcua server"
NODEOPCUA_SERVER_PID=""

# ============================================================
# Summary
# ============================================================

echo ""
echo "=========================================="
if [[ $RESULT -eq 0 ]]; then
    echo "  All interop tests PASSED"
else
    echo "  Some interop tests FAILED"
fi
echo "=========================================="

exit $RESULT
