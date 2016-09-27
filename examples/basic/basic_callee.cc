#include "XXX/kernel.h"
#include "XXX/rawsocket_protocol.h"
#include "XXX/wamp_connector.h"
#include "XXX/wamp_session.h"
#include "XXX/websocket_protocol.h"

#include <memory>
#include <iostream>

using namespace XXX;

void rpc(wamp_invocation& invoke)
{
  invoke.yield( jalson::json_array({"hello", "world"}), {} );
}

int __main(int, char**)
{

    std::unique_ptr<kernel> the_kernel( new XXX::kernel({}, logger::nolog() ));

    // Attempt to make a socket connection & build a wamp_session
    auto wconn = wamp_connector::create( the_kernel.get(),
                                         "127.0.0.1", "55555",
                                         false );

    auto connect_status = wconn->completion_future().wait_for(std::chrono::milliseconds(100));

    if (connect_status == std::future_status::timeout)
      throw std::runtime_error("time-out during network connect");

    std::promise<void> promise_on_close;

    std::shared_ptr<wamp_session> session = wconn->create_session<rawsocket_protocol>(
      [&promise_on_close](XXX::session_handle, bool is_open)
      {
        std::cout << std::this_thread::get_id() << " " << "close_cb  (is_open=" << is_open << ") -->" << "\n";
        if (!is_open)
          promise_on_close.set_value();
        std::cout << std::this_thread::get_id() << " " << "close_cb <--" << "\n";
      });

    // Logon to a WAMP realm, and wait for session to be deemed open
    client_credentials credentials;
    credentials.realm="default_realm";
    credentials.authid="peter";
    credentials.authmethods = {"wampcra"};
    credentials.secret_fn = []() -> std::string { return "secret2"; };

    auto session_open_fut = session->initiate_hello(credentials);

    if (session_open_fut.wait_for(std::chrono::milliseconds(5000)) == std::future_status::timeout)
      throw std::runtime_error("time-out during session logon");

    std::cout << std::this_thread::get_id() << " " << "throwing exception"  << std::endl; throw std::runtime_error("xxx");
    // Session is now open, register an RPC
    session->provide("greeting", jalson::json_object(), rpc);

    // Wait until we get disconnected
    promise_on_close.get_future().wait();

    return 0;
}


int main(int a, char** b)
{
  try {
    __main(a, b);

    return 0;
  }
  catch (std::exception& e)
  {
    std::cout << std::this_thread::get_id() << " " << "caught excpetion"  << "\n";
    std::cout << e.what() << std::endl;
    return 0;
  }
}
