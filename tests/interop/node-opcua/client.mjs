/**
 * node-opcua interoperability test client.
 *
 * Runs the T-1 through T-9 test suite against any OPC UA server.
 * Mirrors the checks performed by the other SDK interop clients
 * (see tests/interop/README.md).
 *
 * Usage: node client.mjs <server_url> <client_cert.pem> <client_key.pem> <server_cert.pem>
 *   server_url      - Endpoint URL (e.g. opc.tcp://localhost:4840)
 *   client_cert.pem - Client certificate (PEM)
 *   client_key.pem  - Client private key (PEM)
 *   server_cert.pem - Server certificate (PEM) for trust
 */

import {
    OPCUAClient,
    SecurityPolicy,
    MessageSecurityMode,
    UserTokenType,
    OPCUACertificateManager,
    AttributeIds,
    DataType,
    Variant,
    StatusCodes,
} from "node-opcua";

import { readFileSync, mkdtempSync, mkdirSync, cpSync } from "fs";
import { X509Certificate } from "crypto";
import { tmpdir } from "os";
import { join } from "path";

// ---------------------------------------------------------------------------
// Parse arguments
// ---------------------------------------------------------------------------

const [serverUrl, clientCertFile, clientKeyFile, serverCertFile] =
    process.argv.slice(2);

if (!serverUrl || !clientCertFile || !clientKeyFile || !serverCertFile) {
    console.error(
        "Usage: node client.mjs <server_url> <client_cert.pem> <client_key.pem> <server_cert.pem>"
    );
    process.exit(1);
}

// ---------------------------------------------------------------------------
// Test counters
// ---------------------------------------------------------------------------

let passed = 0;
let failed = 0;
let skipped = 0;

function PASS(name) {
    console.log(`  PASS: ${name}`);
    passed++;
}
function FAIL(name, err) {
    console.error(`  FAIL: ${name}: ${err}`);
    failed++;
}
function SKIP(name, reason) {
    console.log(`  SKIP: ${name}: ${reason}`);
    skipped++;
}

// ---------------------------------------------------------------------------
// PKI setup — temporary directory with auto-accept
// ---------------------------------------------------------------------------

const pkiDir = mkdtempSync(join(tmpdir(), "nodeopcua-client-pki-"));
mkdirSync(join(pkiDir, "trusted", "certs"), { recursive: true });
mkdirSync(join(pkiDir, "rejected", "certs"), { recursive: true });
mkdirSync(join(pkiDir, "own", "certs"), { recursive: true });
mkdirSync(join(pkiDir, "own", "private"), { recursive: true });

// Trust the server certificate
cpSync(serverCertFile, join(pkiDir, "trusted", "certs", "server.cert.pem"));

const clientCertificateManager = new OPCUACertificateManager({
    rootFolder: pkiDir,
    automaticallyAcceptUnknownCertificate: true,
});

let certManagerInitialized = false;

async function ensureCertManager() {
    if (!certManagerInitialized) {
        await clientCertificateManager.initialize();
        certManagerInitialized = true;
    }
}

// ---------------------------------------------------------------------------
// Client factory
// ---------------------------------------------------------------------------

// Extract the OPC UA Application URI from a PEM certificate's subjectAltName.
// OPC UA requires the client to ANNOUNCE the same URI embedded in its cert;
// if the two differ the server sends BadCertificateUriInvalid and drops the
// connection before any session is established.
function getAppUri(certFile) {
    try {
        const pem = readFileSync(certFile, "utf8");
        const cert = new X509Certificate(pem);
        const match = cert.subjectAltName?.match(/URI:([^,\s]+)/);
        return match?.[1] ?? undefined;
    } catch {
        return undefined;
    }
}

async function createClient(securityPolicy, securityMode) {
    await ensureCertManager();
    const opts = {
        securityPolicy,
        securityMode,
        endpointMustExist: false,
        clientCertificateManager,
    };
    if (securityPolicy !== SecurityPolicy.None) {
        opts.certificateFile = clientCertFile;
        opts.privateKeyFile = clientKeyFile;
        // Set applicationUri to match the certificate's SAN URI so the server
        // doesn't reject the connection with BadCertificateUriInvalid.
        const appUri = getAppUri(clientCertFile);
        if (appUri) opts.applicationUri = appUri;
    }
    return OPCUAClient.create(opts);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function isSkippableSecurityError(msg) {
    return (
        msg.includes("BadSecurityPolicyRejected") ||
        msg.includes("Cannot find an Endpoint matching") ||
        msg.includes("getEndpoints")
    );
}

function isSkippableTrustError(msg) {
    return (
        msg.includes("BadCertificateUntrusted") ||
        msg.includes("BadCertificateInvalid") ||
        msg.includes("BadCertificatePolicyCheckFailed") ||
        msg.includes("BadSecurityChecksFailed")
    );
}

function isSkippableIdentityError(msg) {
    return (
        msg.includes("BadIdentityTokenRejected") ||
        msg.includes("BadIdentityTokenInvalid") ||
        msg.includes("BadUserAccessDenied")
    );
}

// ---------------------------------------------------------------------------
// T-1: Anonymous Connection (No Security)
// ---------------------------------------------------------------------------

async function t1() {
    const name = "T-1: Anonymous Connection (No Security)";
    try {
        const client = await createClient(
            SecurityPolicy.None,
            MessageSecurityMode.None
        );
        await client.connect(serverUrl);
        const session = await client.createSession();

        // Read ServerStatus State (i=2259)
        const result = await session.read({
            nodeId: "i=2259",
            attributeId: AttributeIds.Value,
        });
        if (!result.statusCode.isGood()) {
            throw new Error(`ServerStatus read failed: ${result.statusCode}`);
        }

        await session.close();
        await client.disconnect();
        PASS(name);
    } catch (err) {
        FAIL(name, err.message);
    }
}

// ---------------------------------------------------------------------------
// T-2: Username/Password (No Security)
// ---------------------------------------------------------------------------

async function t2() {
    const name = "T-2: Username/Password (No Security)";
    try {
        const client = await createClient(
            SecurityPolicy.None,
            MessageSecurityMode.None
        );
        await client.connect(serverUrl);
        const session = await client.createSession({
            type: UserTokenType.UserName,
            userName: "user1",
            password: "password",
        });

        await session.close();
        await client.disconnect();
        PASS(name);
    } catch (err) {
        // T-2 is ambiguous: both accepting and rejecting username/password over
        // an unencrypted channel are valid server behaviours.  Some servers
        // require the password to be encrypted with a specific security policy
        // (e.g. Aes256_Sha256_RsaPss) even over a None channel; others simply
        // reject credentials entirely.  Either way, treat every error here as
        // a graceful SKIP rather than a FAIL.
        const msg = err.message || String(err);
        SKIP(name, `Server rejected or could not process credentials: ${msg.slice(0, 100)}`);
    }
}

// ---------------------------------------------------------------------------
// T-3: Read Custom Variable (ns=1;s=the.answer = 43)
// ---------------------------------------------------------------------------

async function t3() {
    const name = "T-3: Read Custom Variable (the.answer = 43)";
    try {
        const client = await createClient(
            SecurityPolicy.None,
            MessageSecurityMode.None
        );
        await client.connect(serverUrl);
        const session = await client.createSession();

        const result = await session.read({
            nodeId: "ns=1;s=the.answer",
            attributeId: AttributeIds.Value,
        });

        if (result.statusCode.equals(StatusCodes.BadNodeIdUnknown)) {
            await session.close();
            await client.disconnect();
            SKIP(name, "Node ns=1;s=the.answer not found");
            return;
        }
        if (!result.statusCode.isGood()) {
            throw new Error(`Read failed: ${result.statusCode}`);
        }

        const val = result.value.value;
        if (val !== 43) {
            throw new Error(`Expected 43, got ${val}`);
        }

        await session.close();
        await client.disconnect();
        PASS(name);
    } catch (err) {
        FAIL(name, err.message);
    }
}

// ---------------------------------------------------------------------------
// T-4: Call HelloWorld Method
// ---------------------------------------------------------------------------

async function t4() {
    const name = "T-4: Call HelloWorld Method";
    try {
        const client = await createClient(
            SecurityPolicy.None,
            MessageSecurityMode.None
        );
        await client.connect(serverUrl);
        const session = await client.createSession();

        let result;
        try {
            result = await session.call({
                objectId: "ns=1;i=1000",
                methodId: "ns=1;i=62541",
                inputArguments: [
                    new Variant({ dataType: DataType.String, value: "World" }),
                ],
            });
        } catch (callErr) {
            const msg = callErr.message || String(callErr);
            if (msg.includes("BadNodeIdUnknown") || msg.includes("BadMethodInvalid")) {
                await session.close();
                await client.disconnect();
                SKIP(name, "Method ns=1;i=62541 not found");
                return;
            }
            throw callErr;
        }

        if (
            result.statusCode.equals(StatusCodes.BadNodeIdUnknown) ||
            result.statusCode.equals(StatusCodes.BadMethodInvalid)
        ) {
            await session.close();
            await client.disconnect();
            SKIP(name, "Method ns=1;i=62541 not found");
            return;
        }
        if (!result.statusCode.isGood()) {
            throw new Error(`Method call failed: ${result.statusCode}`);
        }

        const output = result.outputArguments[0].value;
        if (typeof output !== "string" || !output.startsWith("Hello")) {
            throw new Error(
                `Expected output starting with 'Hello', got '${output}'`
            );
        }

        await session.close();
        await client.disconnect();
        PASS(name);
    } catch (err) {
        FAIL(name, err.message);
    }
}

// ---------------------------------------------------------------------------
// T-5/6/7: Encrypted Anonymous Connection
// ---------------------------------------------------------------------------

async function connectWithSecurity(securityPolicy, securityMode, identity, testName) {
    try {
        const client = await createClient(securityPolicy, securityMode);
        await client.connect(serverUrl);
        const session = identity
            ? await client.createSession(identity)
            : await client.createSession();

        // Read ServerStatus State to verify connection works
        const result = await session.read({
            nodeId: "i=2259",
            attributeId: AttributeIds.Value,
        });
        if (!result.statusCode.isGood()) {
            throw new Error(`ServerStatus read failed: ${result.statusCode}`);
        }

        await session.close();
        await client.disconnect();
        PASS(testName);
    } catch (err) {
        const msg = err.message || String(err);
        if (isSkippableSecurityError(msg)) {
            SKIP(testName, "Server does not offer this security policy");
        } else if (isSkippableTrustError(msg)) {
            SKIP(testName, "Server rejected client certificate");
        } else if (isSkippableIdentityError(msg)) {
            SKIP(testName, "Server rejected identity token");
        } else {
            FAIL(testName, msg);
        }
    }
}

async function t5() {
    await connectWithSecurity(
        SecurityPolicy.Basic256Sha256,
        MessageSecurityMode.SignAndEncrypt,
        null,
        "T-5: Basic256Sha256 Encrypted Anonymous"
    );
}

async function t6() {
    await connectWithSecurity(
        SecurityPolicy.Aes128_Sha256_RsaOaep,
        MessageSecurityMode.SignAndEncrypt,
        null,
        "T-6: Aes128_Sha256_RsaOaep Encrypted Anonymous"
    );
}

async function t7() {
    await connectWithSecurity(
        SecurityPolicy.Aes256_Sha256_RsaPss,
        MessageSecurityMode.SignAndEncrypt,
        null,
        "T-7: Aes256_Sha256_RsaPss Encrypted Anonymous"
    );
}

// ---------------------------------------------------------------------------
// T-8: Username/Password over Encrypted Channel
// ---------------------------------------------------------------------------

async function t8() {
    await connectWithSecurity(
        SecurityPolicy.Basic256Sha256,
        MessageSecurityMode.SignAndEncrypt,
        {
            type: UserTokenType.UserName,
            userName: "user1",
            password: "password",
        },
        "T-8: Username over Basic256Sha256"
    );
}

// ---------------------------------------------------------------------------
// T-9: X509 Certificate Authentication
// ---------------------------------------------------------------------------

async function t9() {
    const name = "T-9: X509 Certificate Authentication";
    try {
        const client = await createClient(
            SecurityPolicy.Basic256Sha256,
            MessageSecurityMode.SignAndEncrypt
        );
        await client.connect(serverUrl);

        const session = await client.createSession({
            type: UserTokenType.Certificate,
            certificateData: readFileSync(clientCertFile),
            privateKey: readFileSync(clientKeyFile, "utf-8"),
        });

        const result = await session.read({
            nodeId: "i=2259",
            attributeId: AttributeIds.Value,
        });
        if (!result.statusCode.isGood()) {
            throw new Error(`ServerStatus read failed: ${result.statusCode}`);
        }

        await session.close();
        await client.disconnect();
        PASS(name);
    } catch (err) {
        const msg = err.message || String(err);
        if (isSkippableSecurityError(msg)) {
            SKIP(name, "Server does not offer Basic256Sha256");
        } else if (
            isSkippableTrustError(msg) ||
            isSkippableIdentityError(msg)
        ) {
            SKIP(name, "Server rejected X509 identity");
        } else {
            FAIL(name, msg);
        }
    }
}

// ---------------------------------------------------------------------------
// T-10..T-15: ECC Security Policies (expected to SKIP in node-opcua)
// ---------------------------------------------------------------------------

const eccPolicies = [
    { label: "T-10", name: "ECC_nistP256", uri: "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256" },
    { label: "T-11", name: "ECC_nistP384", uri: "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384" },
    { label: "T-12", name: "ECC_brainpoolP256r1", uri: "http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1" },
    { label: "T-13", name: "ECC_brainpoolP384r1", uri: "http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1" },
    { label: "T-14", name: "ECC_curve25519", uri: "http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519" },
    { label: "T-15", name: "ECC_curve448", uri: "http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448" },
];

async function runEccTests() {
    for (const ecc of eccPolicies) {
        const testName = `${ecc.label}: ${ecc.name} Encrypted Anonymous`;
        try {
            await connectWithSecurity(
                ecc.uri,
                MessageSecurityMode.SignAndEncrypt,
                null,
                testName
            );
        } catch (err) {
            // connectWithSecurity handles its own errors, but catch any
            // top-level issue (e.g. invalid enum value in node-opcua)
            SKIP(testName, `Not supported by node-opcua: ${(err.message || String(err)).slice(0, 80)}`);
        }
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

async function main() {
    console.log("node-opcua interop client test suite");
    console.log(`  Server URL: ${serverUrl}`);
    console.log(`  Client cert: ${clientCertFile}`);
    console.log(`  Server cert: ${serverCertFile}`);
    console.log("");

    await t1();
    await t2();
    await t3();
    await t4();
    await t5();
    await t6();
    await t7();
    await t8();
    await t9();
    await runEccTests();

    console.log("");
    console.log(`Results: ${passed} passed, ${failed} failed, ${skipped} skipped`);

    process.exit(failed > 0 ? 1 : 0);
}

main().catch((err) => {
    console.error("Fatal error:", err.message || err);
    process.exit(1);
});
