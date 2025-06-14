#include <async_mqtt/all.hpp>
#include <async_mqtt/asio_bind/predefined_layer/mqtts.hpp>
#include <async_mqtt/asio_bind/predefined_layer/ws.hpp> 
#include <async_mqtt/asio_bind/predefined_layer/wss.hpp>
#include <boost/asio.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

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
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;
namespace as = boost::asio;
namespace am = async_mqtt;       
namespace beast = boost::beast;
using tcp = boost::asio::ip::tcp;  





class MQTTHandler {
private:
    as::io_context ioc;
    using client_t = am::client<am::protocol_version::v5, am::protocol::mqtt>;
    std::unique_ptr<client_t> amcl;
    std::thread mqtt_thread;
    std::atomic<bool> running{true};
    std::string topic_id;  // Store the single topic ID






public:
    MQTTHandler(const std::string& topic) : 
        amcl(std::make_unique<client_t>(ioc.get_executor())),
        topic_id(topic) {
        mqtt_thread = std::thread([this]() {
            while(running) {
                ioc.run();
            }
        });
    }

    ~MQTTHandler() {
        running = false;
        if(mqtt_thread.joinable()) {
            mqtt_thread.join();
        }
    }






    void publish_to_mqtt(const std::string &topic, const std::string &payload) {
        as::post(ioc, [this, topic, payload]() {
            as::co_spawn(
                ioc,
                [this, topic, payload]() -> as::awaitable<void> {
                    try {
                        co_await amcl->async_publish(topic, payload, am::qos::at_most_once);
                    } catch(const std::exception &e) {
                        std::cerr << "MQTT publish error: " << e.what() << std::endl;
                    }
                    co_return;
                },
                as::detached);
        });
    }









    void mqtt_subscribe_and_update(std::string topic_id) {
        as::co_spawn(
            ioc,
            [this, topic_id]() -> as::awaitable<void> {
                try {
                    // Connect to broker
                    co_await amcl->async_underlying_handshake("216.48.184.131", "15579", as::use_awaitable);
                    std::cerr << "Connected" << std::endl;

                    // Start MQTT session with username/password
                    auto connack_opt = co_await amcl->async_start(
                        am::v5::connect_packet{
                            true,   // clean_start
                            0x1234, // keep_alive
                            "",     // Client Identifier
                            std::nullopt, // no will
                            "portal",   // username
                            "dt0Unw7QRh" // password
                        },
                        as::use_awaitable
                    );
                    if (!connack_opt) {
                        std::cerr << "Failed to connect to MQTT broker" << std::endl;
                        co_return;
                    }

                    // Subscribe to the single topic
                    std::vector<am::topic_subopts> sub_entry;
                    sub_entry.push_back({topic_id, am::qos::at_most_once});
                    auto suback_opt = co_await amcl->async_subscribe(
                        am::v5::subscribe_packet{
                            *amcl->acquire_unique_packet_id(),
                            am::force_move(sub_entry)
                        },
                        as::use_awaitable
                    );
                    if (!suback_opt) {
                        std::cerr << "Failed to subscribe" << std::endl;
                        co_return;
                    }

                    // Receive loop
                    while (running) {
                        auto pv_opt = co_await amcl->async_recv(as::use_awaitable);
                        if (!pv_opt) break;
                        pv_opt->visit(
                            am::overload{
                                [&](client_t::publish_packet& p) {
                                    std::string topic = p.topic();
                                    std::string payload = p.payload();
                                                                        if (topic == topic_id) {
                                                                            try {
                                                                                auto j = json::parse(payload);

                                                                                if (j.contains("Data") && j["Data"].is_array() && !j["Data"].empty()) {
                                                                                    const auto& valueField = j["Data"][0]["Value"];

                                                                                    if (valueField.is_number()) {
                                                                                        double value = valueField.get<double>();
                                                                                        std::cout << "[MQTT] Numeric value: " << value << std::endl;

                                                                                    } else if (valueField.is_boolean()) {
                                                                                        bool value = valueField.get<bool>();
                                                                                        std::cout << "[MQTT] Boolean value: " << std::boolalpha << value << std::endl;

                                                                                    } else if (valueField.is_string()) {
                                                                                        std::string strValue = valueField.get<std::string>();
                                                                                        std::cout << "[MQTT] String value: " << strValue << std::endl;

                                                                                    } else {
                                                                                        std::cout << "[MQTT] Unsupported value type." << std::endl;
                                                                                    }
                                                                                } else {
                                                                                    std::cout << "[MQTT] No valid Data array found." << std::endl;
                                                                                }
                                                                            } catch (const std::exception& e) {
                                                                                std::cerr << "[MQTT] JSON parse error: " << e.what() << std::endl;
                                                                            }
                                                                        }
                                },
                                [](auto&) {}
                            }
                        );
                    }
                } catch (const std::exception& e) {
                    std::cerr << "MQTT error: " << e.what() << std::endl;
                }
                co_return;
            },
            as::detached
        );
    }


    
};