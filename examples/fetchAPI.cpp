#include <async_mqtt/all.hpp>
#include <async_mqtt/asio_bind/predefined_layer/mqtts.hpp>
#include <async_mqtt/asio_bind/predefined_layer/ws.hpp> 
#include <async_mqtt/asio_bind/predefined_layer/wss.hpp>
#include <boost/asio.hpp>

#include <open62541/plugin/log_stdout.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>


#include <stdio.h>
#include <stdlib.h>
#include <iostream>


#include <nlohmann/json.hpp>
using json = nlohmann::json;


using namespace std;
namespace as = boost::asio;
namespace am = async_mqtt;       
namespace beast = boost::beast;
using tcp = boost::asio::ip::tcp;  

as::io_context ioc;


json getBearerToken() {
    try {
        std::string host = "164.52.221.177";
        std::string port = "5128";
        std::string target = "/api/Login";
        int version = 11;

        // JSON body
        std::string json_body = R"({
        "Username":"ajay.sharma@techondater.co.in",
        "password":"VvvQVRdH7JheYR7lLgbPCp4fcNEslXnKqhR59bdFMK8="
        })";

        // Set up I/O context and resolver
        // as::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Resolve domain name
        auto const results = resolver.resolve(host, port);

        // Connect to host
        stream.connect(results);

        // Create HTTP POST request
        beast::http::request<beast::http::string_body> req{beast::http::verb::post, target, version};
        req.set(beast::http::field::host, host);
        req.set(beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(beast::http::field::content_type, "application/json");
        req.body() = json_body;
        req.prepare_payload();

        // Send request
        beast::http::write(stream, req);

        // Read response
        beast::flat_buffer buffer;
        beast::http::response<beast::http::string_body> res;
        beast::http::read(stream, buffer, res);

        json result = json::parse(res.body());
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, res.body().c_str());

        // Gracefully close the connection
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        return result;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return json{};
    }
}



json getTopicList(string bearerToken) {

try{
    std::string host = "164.52.221.177";
    std::string port = "5128";
    std::string target = "/api/GetTopicList";
    int version = 11;

    // JSON body
    std::string json_body = R"(
    {
   
        "orgId": 0,
        "roleId": "",
        "userId": 0,
        "moduleId": 0,
        "userType": "",
        "requestDateTime": "2024-12-26T08:16:05.629Z",
        "ipAddress": "",
        "originName": "",
        "filterModel": {
            
            "customValue": "all"
        },

        "data": {
            "isLogging" : true
        }
    }
    )";

    // Set up I/O context and connection
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    // Resolve and connect
    auto const results = resolver.resolve(host, port);
    stream.connect(results);

    // Build HTTP POST request
    beast::http::request<beast::http::string_body> req{beast::http::verb::post, target, version};
    req.set(beast::http::field::host, host);
    req.set(beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(beast::http::field::content_type, "application/json");

    // Set Bearer Authorization header
    req.set(beast::http::field::authorization, "Bearer " + bearerToken);

    req.body() = json_body;
    req.prepare_payload();

    // Send request
    beast::http::write(stream, req);

    // Get response
    beast::flat_buffer buffer;
    beast::http::response<beast::http::string_body> res;
    beast::http::read(stream, buffer, res);

    
    // Output response

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, res.body().c_str());
    json result = json::parse(res.body());
    // Shutdown connection
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    return result;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return json{};
    }
    
}
