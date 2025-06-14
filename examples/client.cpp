/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <functional>
#include "Monitoring.cpp"
#include "MQTThandler.cpp"

#include <unordered_map>






std::atomic<bool> g_running(true);

void stopHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    g_running = false;
}

using namespace std;

struct UA_Client_Deleter {
    void operator()(UA_Client *client) const {
        if(client)
            UA_Client_delete(client);
    }
};

struct ClientContext {
    std::string name;
    std::string endpoint;
    std::unique_ptr<UA_Client, UA_Client_Deleter> client;
    UA_CreateSubscriptionResponse subscription;
    std::atomic<bool> running{true};
    std::thread thread;
    std::mutex taskMutex;
    std::queue<std::function<void()>> taskQueue;

    void startLoop() {
        thread = std::thread([this]() {
            std::cout << "Thread started for " << name << "\n";
            while (running) {
                {
                    std::lock_guard<std::mutex> lock(taskMutex);
                    while (!taskQueue.empty()) {
                        auto task = std::move(taskQueue.front());
                        taskQueue.pop(); 
                        task();
                    }
                }
                UA_StatusCode code = UA_Client_run_iterate(client.get(), 100);
                if (code != UA_STATUSCODE_GOOD) {
                    std::cerr << name << ": UA_Client_run_iterate failed with "
                              << UA_StatusCode_name(code) << "\n";
                    break;
                }
            }
            std::cout << "Thread exiting for " << name << "\n";
        });
    }

    void stopLoop() {
        running = false;
        if(thread.joinable()) {
            thread.join();
        }
    }
};

struct ServerInfo {
    std::string name;
    std::string endpoint;
    std::optional<std::string> username;
    std::optional<std::string> password;
};

int main() {


#ifdef UA_ENABLE_SUBSCRIPTIONS
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
#endif

    std::vector<std::unique_ptr<ClientContext>> clientContexts;
    std::unordered_map<std::string, ClientContext *> clientPool;

    std::vector<ServerInfo> servers = { {"Anexee", "opc.tcp://localhost:53531"},
                                        {"Prosys", "opc.tcp://localhost:53530"} };

    for(const auto &server : servers) {
        auto context = std::make_unique<ClientContext>();
        context->name = server.name;
        context->endpoint = server.endpoint;
        context->client = std::unique_ptr<UA_Client, UA_Client_Deleter>(UA_Client_new());
        UA_ClientConfig_setDefault(UA_Client_getConfig(context->client.get()));

        UA_StatusCode retval = UA_Client_connect(context->client.get(), server.endpoint.c_str());
        if(retval != UA_STATUSCODE_GOOD) {
            std::cerr << "Could not connect to server: " << server.endpoint << std::endl;
            continue;
        }

#ifdef UA_ENABLE_SUBSCRIPTIONS
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        request.requestedMaxKeepAliveCount = 60;

        context->subscription = UA_Client_Subscriptions_create(
            context->client.get(), request, nullptr, nullptr, nullptr);
        if(context->subscription.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
            std::cout << "Subscription created successfully for server: "
                      << server.endpoint << std::endl;
        }
#endif

        context->startLoop();
        clientPool[context->name] = context.get();
        clientContexts.push_back(std::move(context));
    }

    std::cout << "Client pool initialized. Press Ctrl+C to stop..." << std::endl;





    std::this_thread::sleep_for(std::chrono::seconds(1));



    MQTTHandler *mqttHandler = new MQTTHandler("TDSPL/Test31-01/tag20-1");

    mqttHandler->mqtt_subscribe_and_update("TDSPL/Test31-01/tag20-1");





    if(clientPool.count("Anexee")) {
        auto context = clientPool["Anexee"];
        std::cout << "Ready to use client: Anexee (connected to "
                  << context->endpoint << ")" << std::endl;

        if (UA_STATUSCODE_GOOD == context->subscription.responseHeader.serviceResult &&
            context->subscription.subscriptionId != 0) {

            std::lock_guard<std::mutex> lock(context->taskMutex);
            context->taskQueue.push([context]() {
                MonitorItem(context->client.get(), context->subscription, "ns=1;i=194", "Anexee");
            });
        } else {
            std::cerr << "Failed to create subscription" << std::endl;
        }
    }










    //     if(clientPool.count("Prosys")) {
    //     auto context = clientPool["Prosys"];
    //     std::cout << "Ready to use client: Anexee (connected to "
    //               << context->endpoint << ")" << std::endl;

    //     if (UA_STATUSCODE_GOOD == context->subscription.responseHeader.serviceResult &&
    //         context->subscription.subscriptionId != 0) {

    //         std::lock_guard<std::mutex> lock(context->taskMutex);
    //         context->taskQueue.push([context]() {
    //             MonitorItem(context->client.get(), context->subscription, "ns=3;i=1002", "Prosys");
    //         });
    //     } else {
    //         std::cerr << "Failed to create subscription" << std::endl;
    //     }
    // }








    while(g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(200));
    }

    std::cout << "Cleaning up..." << std::endl;

    for(auto &context : clientContexts) {
        UA_Client_disconnect(context->client.get());
        context->stopLoop();
    }

    clientPool.clear();
    clientContexts.clear();

    return EXIT_SUCCESS;



    //     UA_Client *client = UA_Client_new();
    //     UA_ClientConfig *config = UA_Client_getConfig(client);

    //     const char *certPath = "D:/OPC UA Server/OPCUA- open "
    //                           "62451/open62541-C/build/bin/examples/client/client.der";
    //     const char *certKey = "D:/OPC UA Server/OPCUA- open "
    //                            "62451/open62541-C/build/bin/examples/client/client_key.der";
    //     const char *servercertPath = "D:/OPC UA Server/OPCUA- open "
    //                            "62451/open62541-C/build/bin/examples/Certs/own/certs/server_cert.der";

    //     // Step 1: Set default config
    //     UA_ClientConfig_setDefault(config);

    //     // Step 2: Load certificates
    //     UA_ByteString certificate = loadFile(certPath);
    //     UA_ByteString privateKey = loadFile(certKey);
    //     UA_ByteString serverCert = loadFile(servercertPath);

    //         if (certificate.length == 0) {
    //             UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to load
    //             client certificate"); return EXIT_FAILURE;
    //         }
    //         if (privateKey.length == 0) {
    //             UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to load
    //             client private key"); return EXIT_FAILURE;
    //         }
    //         if (serverCert.length == 0) {
    //             UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to load
    //             server certificate into trust list"); return EXIT_FAILURE;
    //         }

    //     UA_STACKARRAY(UA_ByteString, trustList, 1);
    //     trustList[0] = serverCert;

    //     // Step 3: Apply encryption
    //     UA_ClientConfig_setDefaultEncryption(config, certificate, privateKey, NULL, 0,
    //     NULL, 0);

    //     UA_CertificateGroup_AcceptAll(&config->certificateVerification);

    //     // Step 5: Other settings
    //     config->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    //     config->securityPolicyUri =
    //     UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    //     config->maxTrustListSize = 1;

    //     UA_String_clear(&config->applicationUri);
    //     config->applicationUri = UA_STRING_ALLOC("urn:Anexee.server.application");
    //     config->clientDescription.applicationUri =
    //     UA_STRING_ALLOC("urn:Anexee.server.application");
    //     config->clientDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en-US",
    //     "Anexee"); config->clientDescription.productUri =
    //     UA_STRING_ALLOC("urn:Anexee.server");

    //     /* Connect to a server */
    //     UA_StatusCode retval = UA_Client_connectUsername(client,
    //     "opc.tcp://Asce:53531", "user1", "password1");

    //     // retval = UA_Client_connectSecureChannel(client,
    //     "opc.tcp://localhost:53531");

    //         // Clean up certificate and private key
    //         UA_ByteString_clear(&certificate);
    //     UA_ByteString_clear(&privateKey);
    //     UA_ByteString_clear(&serverCert);

    //     if(retval != UA_STATUSCODE_GOOD) {
    //         cout << "Could not connect" << endl;
    //         UA_Client_delete(client);
    //         return EXIT_SUCCESS;
    //     }

    // #ifdef UA_ENABLE_SUBSCRIPTIONS
    //     /* Create a subscription */
    //     UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    //     request.requestedMaxKeepAliveCount = 60;
    //     UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client,
    //     request,
    //                                                                             NULL,
    //                                                                             NULL,
    //                                                                             NULL);

    //     UA_UInt32 subId = response.subscriptionId;
    //     if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
    //         cout << "Create subscription succeeded, id " << subId << endl;
    //         cout << "Revised publishing interval: " <<
    //         response.revisedPublishingInterval << " ms" << endl; cout << "Revised
    //         lifetime count: " << response.revisedLifetimeCount << endl; cout <<
    //         "Revised max keep alive count: " << response.revisedMaxKeepAliveCount <<
    //         endl;
    //     }

    //     MonitorItem(client, response, "ns=1;i=194");
    //     UA_Client_run_iterate(client, 1000);
    // #endif

    //     /* Read attribute */
    //     UA_Int32 value = 0;
    //     cout << "\nReading the value of node (1, \"the.answer\"):" << endl;
    //     UA_Variant *val = UA_Variant_new();
    //     retval = UA_Client_readValueAttribute(
    //         client, UA_NODEID_STRING(1, const_cast<char *>("the.answer")), val);
    //     if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) &&
    //        val->type == &UA_TYPES[UA_TYPES_INT32]) {
    //             value = *(UA_Int32*)val->data;
    //             cout << "the value is: " << value << endl;
    //     }
    //     UA_Variant_delete(val);

    // #ifdef UA_ENABLE_SUBSCRIPTIONS

    //     cout << "Listening for events. Press Ctrl-C to exit." << endl;
    //     while(running) {
    //         UA_Client_run_iterate(client, 100);
    //     }

    // #endif

    //     cout << "Press Enter to continue...";
    //     getchar();

    //     UA_Client_disconnect(client);
    //     UA_Client_delete(client);
    //     return EXIT_SUCCESS;
}
