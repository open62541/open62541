/**
 * node-opcua interoperability client over OPC UA Binary WebSockets.
 *
 * Usage: node websocket/client_wss.mjs <opc.wss_url> <client_cert.pem>
 *        <client_key.pem> <server_cert.pem>
 */

import { runInteropClient } from "../client.mjs";
import { createWssTransportFactory } from "./ws_transport.mjs";

const serverCertificateFile = process.argv[5];
if (!serverCertificateFile) {
    console.error(
        "Usage: node websocket/client_wss.mjs <opc.wss_url> " +
        "<client_cert.pem> <client_key.pem> <server_cert.pem>"
    );
    process.exit(1);
}

const transportFactory = createWssTransportFactory(serverCertificateFile);
runInteropClient(transportFactory).catch((error) => {
    console.error("Fatal error:", error.message || error);
    process.exit(1);
});
