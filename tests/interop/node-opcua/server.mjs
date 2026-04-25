/**
 * node-opcua interoperability test server.
 *
 * Implements a subset of the server requirements from tests/interop/README.md:
 *   - SecurityPolicies: None, Basic256Sha256
 *   - SecurityModes:    None (only with None policy), Sign, SignAndEncrypt
 *   - Authentication:   Anonymous, Username/Password (user1/password), X509
 *   - ns=1;s=the.answer  -> Int32 = 43
 *   - ns=1;i=62541       -> HelloWorld method under InteropTests (ns=1;i=1000)
 *
 * Usage: node server.mjs <port> <cert.pem> <key.pem> <pki-dir>
 *   port      - TCP port to listen on
 *   cert.pem  - Server certificate (PEM)
 *   key.pem   - Server private key (PEM)
 *   pki-dir   - PKI root directory (must contain own/certs, own/private,
 *               trusted/certs, rejected/certs sub-directories)
 */

import {
    OPCUAServer,
    SecurityPolicy,
    MessageSecurityMode,
    DataType,
    Variant,
    StatusCodes,
    OPCUACertificateManager,
} from "node-opcua";

// ---------------------------------------------------------------------------
// Parse arguments
// ---------------------------------------------------------------------------

const [port, certFile, keyFile, pkiDir] = process.argv.slice(2);

if (!port || !certFile || !keyFile || !pkiDir) {
    console.error(
        "Usage: node server.mjs <port> <cert.pem> <key.pem> <pki-dir>"
    );
    process.exit(1);
}

// ---------------------------------------------------------------------------
// Server configuration
// ---------------------------------------------------------------------------

const serverCertificateManager = new OPCUACertificateManager({
    rootFolder: pkiDir,
    automaticallyAcceptUnknownCertificate: true,
});

const server = new OPCUAServer({
    port: parseInt(port, 10),
    certificateFile: certFile,
    privateKeyFile: keyFile,
    serverCertificateManager,
    securityPolicies: [SecurityPolicy.None, SecurityPolicy.Basic256Sha256],
    securityModes: [
        MessageSecurityMode.None,
        MessageSecurityMode.Sign,
        MessageSecurityMode.SignAndEncrypt,
    ],
    allowAnonymous: true,
    userManager: {
        isValidUser(userName, password) {
            return userName === "user1" && password === "password";
        },
        isValidUserAsync: undefined,
    },
    // Accept X509 identity tokens; certificate validation is handled by
    // serverCertificateManager.
    isAuditing: false,
    buildInfo: {
        productName: "node-opcua Interop Test Server",
        buildNumber: "1",
    },
});

// ---------------------------------------------------------------------------
// Build address space
// ---------------------------------------------------------------------------

async function buildAddressSpace(addressSpace) {
    const namespace = addressSpace.getOwnNamespace();

    // --- Variable: ns=1;s=the.answer = Int32(43) ---
    namespace.addVariable({
        organizedBy: addressSpace.rootFolder.objects,
        nodeId: "ns=1;s=the.answer",
        browseName: "the.answer",
        dataType: DataType.Int32,
        minimumSamplingInterval: 1000,
        value: {
            get() {
                return new Variant({ dataType: DataType.Int32, value: 43 });
            },
        },
    });

    // --- InteropTest object (container for the method) ---
    // Methods use HasComponent references which are not allowed directly
    // under ObjectsFolder.  Use an intermediate object.
    const interopObj = namespace.addObject({
        organizedBy: addressSpace.rootFolder.objects,
        browseName: "InteropTests",
        nodeId: "ns=1;i=1000",
    });

    // --- Method: ns=1;i=62541 HelloWorld under InteropTests ---
    const method = namespace.addMethod(interopObj, {
        nodeId: "ns=1;i=62541",
        browseName: "HelloWorld",
        inputArguments: [
            {
                name: "name",
                description: { text: "Name to greet" },
                dataType: DataType.String,
                valueRank: -1,
            },
        ],
        outputArguments: [
            {
                name: "result",
                description: { text: "Greeting string" },
                dataType: DataType.String,
                valueRank: -1,
            },
        ],
    });

    method.bindMethod(async (inputArguments, context) => {
        const name =
            inputArguments[0] && inputArguments[0].value
                ? inputArguments[0].value
                : "World";
        return {
            statusCode: StatusCodes.Good,
            outputArguments: [
                {
                    dataType: DataType.String,
                    value: `Hello ${name}`,
                },
            ],
        };
    });
}

// ---------------------------------------------------------------------------
// Start
// ---------------------------------------------------------------------------

async function main() {
    await serverCertificateManager.initialize();
    await server.initialize();
    await buildAddressSpace(server.engine.addressSpace);
    await server.start();

    const endpointUrl = server.getEndpointUrl();
    console.log(`node-opcua interop server listening at: ${endpointUrl}`);
    console.log("Press Ctrl+C to stop.");
}

main().catch((err) => {
    console.error("Fatal error:", err.message || err);
    process.exit(1);
});

// Graceful shutdown
process.on("SIGTERM", async () => {
    await server.shutdown();
    process.exit(0);
});
process.on("SIGINT", async () => {
    await server.shutdown();
    process.exit(0);
});
