# OPC UA Cross-SDK Interoperability Test Specification

## Purpose

Validate that the open62541 C SDK can communicate correctly with other OPC UA
SDK implementations. The test suite covers transport-level security, user
authentication, data access, and method invocation.

The specification is SDK-agnostic: any conformant OPC UA server or client can
be substituted. The current reference implementation tests against the
OPC Foundation .NET Standard SDK.

## Test Server Requirements

The server under test **must** provide:

| Item | Details |
|------|---------|
| Endpoint | `opc.tcp://<host>:<port>` |
| SecurityPolicies | `None`, `Basic256Sha256`, `Aes128_Sha256_RsaOaep`, `Aes256_Sha256_RsaPss` |
| SecurityModes | `None` (only with policy None), `Sign`, `SignAndEncrypt` |
| Anonymous access | Enabled |
| Username/Password | `user1` / `password` |
| X509 certificate auth | Enabled; trust store must contain the client auth certificate |
| Variable | `ns=1;s=the.answer` — `Int32`, value `43`, readable |
| Method | `ns=1;i=62541` under `ns=1;i=1000` (InteropTests) — HelloWorld(String) → String |

## Test Matrix

### T-1: Anonymous Connection (No Security)

| # | Step | Expected |
|---|------|----------|
| 1 | Connect with `SecurityPolicy#None`, anonymous | `Good` |
| 2 | Read `Server_ServerStatus_State` | `Good`, value is `ServerState` |
| 3 | Read `Server_NamespaceArray` | `Good`, length > 0, index 0 = `http://opcfoundation.org/UA/` |
| 4 | Disconnect | Clean close |

### T-2: Username/Password Authentication (No Security)

> **Note:** Many secure servers (including the open62541 `interop_server`) correctly
> reject username/password credentials over the `None` policy to prevent
> credential leaking. Both outcomes (accept or reject) are valid.

| # | Step | Expected |
|---|------|----------|
| 1 | Connect with `SecurityPolicy#None`, user `user1`/`password` | `Good` or rejection (server-dependent) |
| 2 | Read `Server_ServerStatus_State` (if connected) | `Good` |
| 3 | Disconnect | Clean close |

### T-3: Read Custom Variable

| # | Step | Expected |
|---|------|----------|
| 1 | Connect anonymous (any policy) | `Good` |
| 2 | Read `ns=1;s=the.answer` | `Good`, Int32, value = `43` |
| 3 | Disconnect | Clean close |

### T-4: Call Method

| # | Step | Expected |
|---|------|----------|
| 1 | Connect anonymous (any policy) | `Good` |
| 2 | Call `ns=1;i=62541`.HelloWorld("World") on `ns=1;i=1000` | Output contains `"Hello World"` |
| 3 | Disconnect | Clean close |

### T-5: Encrypted Connection — Basic256Sha256

| # | Step | Expected |
|---|------|----------|
| 1 | Discover endpoints | Server offers `Basic256Sha256` / `SignAndEncrypt` |
| 2 | Connect with `Basic256Sha256`, `SignAndEncrypt`, anonymous | `Good` |
| 3 | Read `Server_ServerStatus_State` | `Good` |
| 4 | Disconnect | Clean close |

### T-6: Encrypted Connection — Aes128_Sha256_RsaOaep

| # | Step | Expected |
|---|------|----------|
| 1 | Connect with `Aes128_Sha256_RsaOaep`, `SignAndEncrypt`, anonymous | `Good` |
| 2 | Read `Server_ServerStatus_State` | `Good` |
| 3 | Disconnect | Clean close |

### T-7: Encrypted Connection — Aes256_Sha256_RsaPss

| # | Step | Expected |
|---|------|----------|
| 1 | Connect with `Aes256_Sha256_RsaPss`, `SignAndEncrypt`, anonymous | `Good` |
| 2 | Read `Server_ServerStatus_State` | `Good` |
| 3 | Disconnect | Clean close |

### T-8: Username/Password over Encrypted Channel

| # | Step | Expected |
|---|------|----------|
| 1 | Connect with `Basic256Sha256`, `SignAndEncrypt`, user `user1`/`password` | `Good` |
| 2 | Read `Server_ServerStatus_State` | `Good` |
| 3 | Disconnect | Clean close |

### T-9: X509 Certificate Authentication

| # | Step | Expected |
|---|------|----------|
| 1 | Connect with `Basic256Sha256`, `SignAndEncrypt`, X509 cert identity | `Good` |
| 2 | Read `Server_ServerStatus_State` | `Good` |
| 3 | Disconnect | Clean close |

## Certificate Setup

The test infrastructure generates these RSA-2048 certificate/key pairs:

| Name | CN | ApplicationUri SAN | Usage |
|------|----|--------------------|-------|
| `server_c` | `open62541 CI Server` | `urn:open62541.unconfigured.application` | C server SecureChannel |
| `client_c` | `open62541 CI Client` | `urn:open62541.client.application` | C client SecureChannel + X509 auth |
| `server_dotnet` | `OPC UA .NET Reference Server` | `urn:localhost:UA:ReferenceServer` | .NET server SecureChannel |
| `client_dotnet` | `OPC UA .NET Interop Client` | `urn:localhost:UA:InteropClient` | .NET client SecureChannel |
| `server_nodeopcua` | `node-opcua Interop Server` | `urn:localhost:node-opcua:interop` | node-opcua server SecureChannel |
| `client_nodeopcua` | `node-opcua Interop Client` | `urn:localhost:node-opcua:client` | node-opcua client SecureChannel |

### Trust Relationships

- **C server** trusts: `client_c`, `client_dotnet`, `client_nodeopcua` (for both SecureChannel and session/X509 auth)
- **.NET server** trusts: `server_c`, `client_c`
- **node-opcua server** trusts: `client_c`

## CI Integration

The orchestration script `tools/ci/cross-sdk/interop_test.sh` runs:

- **Scenario A**: C server ↔ C / .NET / node-opcua clients (all tests T-1 through T-9)
- **Scenario B**: .NET server ↔ C client (all tests T-1 through T-9)
- **Scenario C**: node-opcua server ↔ C client (all tests T-1 through T-9)

The CI workflow (`.github/workflows/interop_tests.yml`) runs all scenarios
with OpenSSL and mbedTLS encryption backends.

## Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `OPCUA_INTEROP_SERVER_URL` | Target server endpoint | `opc.tcp://localhost:4840` |
| `OPCUA_INTEROP_CERT_DIR` | Directory with generated certificates | — |
| `DOTNET_CONFIG` | .NET build configuration | `Debug` |

## Adding a New SDK

To test against a new OPC UA SDK:

1. Implement a server matching the requirements above
2. Implement a client exercising tests T-1 through T-9
3. Add a new scenario to `interop_test.sh`
4. Generate SDK-specific certificates in `generate_interop_certs.sh`
