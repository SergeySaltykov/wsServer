#include "server_ws.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <string>
#include <math.h>
#include <stdio.h>

using namespace std;

string getResult(float value, int requestID);

string getError(const string &errorMessage, int requestID);

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

int main() {
    // WebSocket (WS)-server at port 8080 using 1 thread
    WsServer server;
    server.config.port = 8080;

    auto &echo = server.endpoint["^/compute/?$"];

    echo.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
        auto message_str = message->string();
        auto send_stream = make_shared<WsServer::SendStream>();

        cout << "Server: Message received: \"" << message_str << "\" from " << connection.get() << endl;
        // {"x":2,"y":4,"action":"add","id":4} формат отправки данных
        rapidjson::Document document;
        document.Parse(message_str.c_str());

        rapidjson::Value &x = document["x"];
        rapidjson::Value &y = document["y"];
        int id = document["id"].GetInt();
        rapidjson::Value &action = document["action"];

        cout << "X: " << x.GetFloat() << endl;
        cout << "Y: " << y.GetFloat() << endl;
        cout << "Action: " << action.GetString() << endl;


        if (action == "add") {
            *send_stream << getResult(x.GetFloat() + y.GetFloat(), id);
        } else if (action == "multiplication") {
            *send_stream << getResult(x.GetFloat() * y.GetFloat(), id);
        } else if (action == "division") {
            *send_stream << getResult(x.GetFloat() / y.GetFloat(), id);
        } else if (action == "subtraction"){
            *send_stream << getResult(x.GetFloat() - y.GetFloat(), id);
        } else if (action == "rate") {
            *send_stream << getResult(pow(x.GetFloat(), y.GetFloat()) , id);
        } else if (action == "root") {
            *send_stream << getResult(sqrt(x.GetFloat()), id);
        } else if (action == "exponent") {
            *send_stream << getResult(exp(x.GetFloat()), id);
        } else if (action == "log") {
            *send_stream << getResult(log(x.GetFloat()), id);
        } else if (action == "rootInN") {
            *send_stream << getResult(pow(x.GetFloat(), (1/y.GetFloat())), id);
        } else *send_stream << getError("Unknown action", id);


        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);

        cout << "Object: " << buffer.GetString() << endl;
//        cout << "Result: " << x.GetInt() + y.GetInt() << endl;
        cout << "Server: Sending message \"" << send_stream << "\" to " << connection.get() << endl;

        // connection->send is an asynchronous function
        connection->send(send_stream, [](const SimpleWeb::error_code &ec) {
            if (ec) {
                cout << "Server: Error sending message. " <<
                     // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
                     "Error: " << ec << ", error message: " << ec.message() << endl;
            }
        });
    };

    echo.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    // See RFC 6455 7.4.1. for status codes
    echo.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string & /*reason*/) {
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    echo.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };


    thread server_thread([&server]() {
        // Start WS-server
        server.start();
    });

    // Wait for server to start so that the client can connect
    this_thread::sleep_for(chrono::seconds(1));


    server_thread.join();

}

string getResult(float value, int requestID) {
    return R"({"result":")" + ::to_string(value) + R"(","id":)" + ::to_string(requestID) + "}";
}

string getError(const string &errorMessage, int requestID) {
    return R"({"error":")" + errorMessage + R"(","id":)" + ::to_string(requestID) + "}";
}
