#include "wampcc/wampcc.h"

#include <memory>
#include <iostream>
#include <chrono>
#include "mysql_client.h"
#include "type_cast.h"

using namespace std;
using namespace daotk::mysql;
using namespace wampcc;

void rpc_registered(std::promise<void> &ready_to_exit, wamp_session &ws, registered_info info) {
    std::cout << "rpc registration "
              << (info.was_error ? "failed, " + info.error_uri : "success")
              << std::endl;
    if (info.was_error)
        ready_to_exit.set_value();
}

void rpc_get_rooms(wamp_session &ws, invocation_info invoke) {
    std::cout << "rpc invoked" << std::endl;

    json_object device1 = json_object({
                                              {"id",   "1"},
                                              {"name", "Кухня"}

                                      });

    json_object device2 = json_object({
                                              {"id",   "2"},
                                              {"name", "Спальня"}
                                      });

    json_array msg{json_value(device1), json_value(device2)};
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ws.yield(invoke.request_id, msg);
}

void rpc_get_devices_room(wamp_session &ws, invocation_info invoke) {
    std::cout << "rpc invoked" << std::endl;

    json_object device1 = json_object({
                                              {"id",        "1"},
                                              {"name",      "Устройство 1"},
                                              {"category",  json_object({
                                                                                {"id",   "1"},
                                                                                {"name", "Категория 1"},
                                                                        })},
                                              {"functions", json_array{json_object({
                                                                                           {"id",            "1"},
                                                                                           {"name",          "Функция 1"},
                                                                                           {"measure",       "1"},
                                                                                           {"value",         0},
                                                                                           {"isWriteAccess", true},
                                                                                           {"minValue",      0},
                                                                                           {"maxValue",      1}
                                                                                   }),
                                                                       json_object({
                                                                                           {"id",            "2"},
                                                                                           {"name",          "Функция 2"},
                                                                                           {"measure",       "2"},
                                                                                           {"value",         -20},
                                                                                           {"isWriteAccess", true},
                                                                                           {"minValue",      -60},
                                                                                           {"maxValue",      60}
                                                                                   }),
                                                                       json_object({
                                                                                           {"id",            "3"},
                                                                                           {"name",          "Функция 3"},
                                                                                           {"measure",       "3"},
                                                                                           {"value",         25.5},
                                                                                           {"isWriteAccess", false},
                                                                                           {"minValue",      -60},
                                                                                           {"maxValue",      60}
                                                                                   })}}
                                      });

    json_object device2 = json_object({
                                              {"id",        "2"},
                                              {"name",      "Устройство 2"},
                                              {"category",  json_object({
                                                                                {"id",   "2"},
                                                                                {"name", "Категория 2"},
                                                                        })},
                                              {"functions", json_array{json_object({
                                                                                           {"id",            "1"},
                                                                                           {"name",          "Функция 1"},
                                                                                           {"measure",       "1"},
                                                                                           {"value",         20.5},
                                                                                           {"isWriteAccess", true},
                                                                                           {"minValue",      1},
                                                                                           {"maxValue",      100}
                                                                                   })}}
                                      });

    json_array msg{json_value(device1), json_value(device2)};
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ws.yield(invoke.request_id, msg);
}


int main(int argc, char **argv) {
    try {
        // load configuration
        json_value config = json_load_file("config.json");

        // connection mysql
        connect_options options{
                config["mysql"]["server"].as_string(),
                config["mysql"]["username"].as_string(),
                config["mysql"]["password"].as_string(),
                config["mysql"]["dbname"].as_string(),
                static_cast<uint32_t>(config["mysql"]["timeout"].as_int()),
                config["mysql"]["autoreconnect"].as_bool(),
                config["mysql"]["init_command"].as_string(),
                config["mysql"]["charset"].as_string(),
                static_cast<uint32_t>(config["mysql"]["port"].as_int()),
                static_cast<uint64_t>(config["mysql"]["client_flag"].as_int())};

        connection mysql{options};

        if (!mysql) {
            cout << "Connection failed mysql" << endl;
            return -1;
        }

        // create the wampcc kernel, which provides event and IO threads.
        wampcc::config client_config;
        client_config.ssl.enable = config["client"]["ssl"].as_bool();
        logger client_log = config["client"]["debug"].as_bool() ? logger::console() : logger::nolog();

        std::unique_ptr<kernel> the_kernel(new kernel({client_config}, client_log));


        const auto client_host = config["client"]["host"].as_string();
        const auto client_port = static_cast<int>(config["client"]["port"].as_int());

        // create the TCP socket and attempt to connect.
        std::unique_ptr<tcp_socket> sock(new tcp_socket(the_kernel.get()));


        auto fut = sock->connect(client_host, client_port);

        // socket reconnect check
        const uint64_t client_timeout = static_cast<uint64_t>(config["client"]["timeout"].as_int());
        const bool client_autoreconnect = config["client"]["autoreconnect"].as_bool();

        while (fut.wait_for(std::chrono::seconds(client_timeout)) != std::future_status::ready) {
            if (!client_autoreconnect)
                throw std::runtime_error("timeout during connect");
        }

        if (fut.get()) {
            client_log.write(wampcc::logger::eError, wampcc::package_string(), __FILE__, __LINE__);
        }

        /* Using the connected socket, now create the wamp session object, using
           the WebSocket protocol. */

        std::promise<void> ready_to_exit;
        std::shared_ptr<wamp_session> session = wamp_session::create<websocket_protocol>(
                the_kernel.get(),
                std::move(sock),
                [&ready_to_exit](wamp_session &, bool is_open) {
                    if (!is_open)
                        try {
                            ready_to_exit.set_value();
                        }
                        catch (...) { /* ignore promise already set error */ }
                }, {});

        /* Logon to a WAMP realm, and wait for session to be deemed open. */

        auto logon_fut = session->hello(config["client"]["realm"].as_string());


        while (logon_fut.wait_for(std::chrono::seconds(client_timeout)) != std::future_status::ready) {
            if (!client_autoreconnect)
                throw std::runtime_error("time-out during session logon");
        }

        if (!session->is_open())
            throw std::runtime_error("session logon failed");

        /* Session is now open, register an RPC. */

        session->provide("get_rooms", json_object(),
                         [&ready_to_exit](wamp_session &ws, registered_info info) {
                             rpc_registered(ready_to_exit, ws, info);
                         },
                         rpc_get_rooms);

        session->provide("get_devices_room", json_object(),
                         [&ready_to_exit](wamp_session &ws, registered_info info) {
                             rpc_registered(ready_to_exit, ws, info);
                         },
                         rpc_get_devices_room);

        session->provide("set_param_device", json_object(),
                         [&ready_to_exit](wamp_session &ws, registered_info info) {
                             rpc_registered(ready_to_exit, ws, info);
                         },
                         [](wamp_session &ws, invocation_info info) {
                             for (auto &item : info.args.args_list)
                                 std::cout << item << std::endl;
                         });


        session->provide("call_test_publish", json_object(),
                         [&ready_to_exit](wamp_session &ws, registered_info info) {
                             rpc_registered(ready_to_exit, ws, info);
                         },
                         [session](wamp_session &ws, invocation_info info) {

                             json_object device1 = json_object({
                                                                       {"idDevice",   1},
                                                                       {"idFunction", 2},
                                                                       {"value",      30}

                                                               });

                             session->publish("sub_devices_function", {}, {{json_encode({device1})}});
                         });

        /* Wait until wamp session is closed. */

        ready_to_exit.get_future().wait();
        return 0;
    }
    catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}
