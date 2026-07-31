import { EventEmitter } from "events";
import { readFileSync } from "fs";
import { ClientTransportBase } from "node-opcua-transport";
import WebSocket from "ws";

const UACP_SUBPROTOCOL = "opcua+uacp";

class WebSocketAdapter extends EventEmitter {
    constructor(ws, url) {
        super();
        this.ws = ws;
        this.destroyed = false;
        this.timeoutMs = 0;
        this.timeoutCallback = undefined;
        this.timeoutHandle = undefined;

        const endpoint = new URL(url);
        this.remoteAddress = endpoint.hostname;
        this.remotePort = endpoint.port ? Number(endpoint.port) : 443;

        ws.binaryType = "nodebuffer";
        ws.once("open", () => this.emit("connect"));
        ws.on("message", (data, isBinary) => {
            this.resetTimeout();
            if (!isBinary) {
                this.emit("error", new Error("Received a text WebSocket message"));
                return;
            }
            this.emit("data", Buffer.from(data));
        });
        ws.on("error", (error) => this.emit("error", error));
        ws.on("close", (code) => {
            this.clearTimeout();
            this.emit("end");
            this.emit("close", code !== 1000);
        });
    }

    write(data, callback) {
        if (this.destroyed || this.ws.readyState !== WebSocket.OPEN) {
            callback?.(new Error("WebSocket is not open"));
            return;
        }
        this.ws.send(data, { binary: true }, callback);
    }

    end() {
        this.clearTimeout();
        if (this.ws.readyState === WebSocket.OPEN)
            this.ws.close(1000, "normal closure");
    }

    destroy(error) {
        this.destroyed = true;
        this.clearTimeout();
        if (this.ws.readyState < WebSocket.CLOSING)
            this.ws.close(1001, error?.message || "going away");
    }

    setKeepAlive() { return this; }
    setNoDelay() { return this; }

    setTimeout(timeout, callback) {
        this.timeoutMs = timeout;
        this.timeoutCallback = callback;
        this.resetTimeout();
        return this;
    }

    resetTimeout() {
        this.clearTimeout();
        if (this.timeoutMs > 0 && this.timeoutCallback) {
            this.timeoutHandle = setTimeout(() => {
                this.emit("timeout");
                this.timeoutCallback();
            }, this.timeoutMs);
        }
    }

    clearTimeout() {
        if (this.timeoutHandle) {
            globalThis.clearTimeout(this.timeoutHandle);
            this.timeoutHandle = undefined;
        }
    }
}

class ClientWssTransport extends ClientTransportBase {
    constructor(settings, caCertificate) {
        super(settings);
        this.caCertificate = caCertificate;
    }

    connect(endpointUrl, callback) {
        this.endpointUrl = endpointUrl;
        let wsUrl;
        try {
            const endpoint = new URL(endpointUrl);
            if (endpoint.protocol !== "opc.wss:")
                throw new Error(`Expected opc.wss URL, got ${endpointUrl}`);
            wsUrl = `wss://${endpoint.host}${endpoint.pathname}${endpoint.search}`;
        } catch (error) {
            callback(error);
            return;
        }

        const ws = new WebSocket(wsUrl, UACP_SUBPROTOCOL, {
            ca: this.caCertificate,
            rejectUnauthorized: true,
        });
        const socket = new WebSocketAdapter(ws, wsUrl);
        const onEarlyError = (error) => callback(error);
        socket.once("error", onEarlyError);
        socket.once("connect", () => {
            socket.removeListener("error", onEarlyError);
            if (ws.protocol !== UACP_SUBPROTOCOL) {
                socket.destroy();
                callback(new Error(
                    `Expected WebSocket subprotocol ${UACP_SUBPROTOCOL}, got ${ws.protocol || "none"}`
                ));
                return;
            }

            this._install_socket(socket);
            this._perform_HEL_ACK_transaction((error) => {
                if (!error) {
                    this._install_post_connect_error_handler(endpointUrl);
                    this.emit("connect");
                }
                callback(error);
            });
        });
    }
}

export function createWssTransportFactory(serverCertificateFile) {
    const caCertificate = readFileSync(serverCertificateFile);
    return {
        create(settings) {
            return new ClientWssTransport(settings, caCertificate);
        },
    };
}
