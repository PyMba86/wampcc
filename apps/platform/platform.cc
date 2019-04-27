/*
 * Copyright (c) 2017 Darren Smith
 *
 * wampcc is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "wampcc/wampcc.h"

#include <iostream>

using namespace wampcc;
using namespace std;

int main(int, char**)
{
    try {

        json_value config = json_load_file("config.json");

        /* Create the wampcc kernel. */

        std::cout << config["name"].as_string() << std::endl;

        kernel the_kernel;

        /* Create an embedded wamp router. */

        wamp_router router(&the_kernel);

        /* Accept clients on IPv4 port, without authentication. */

        auto fut = router.listen(auth_provider::no_auth_required(), 55555);

        if (auto ec = fut.get())
            throw runtime_error(ec.message());

        /* Suspend main thread */
        std::promise<void> forever;
        forever.get_future().wait();
    } catch (const exception& e) {
        cout << e.what() << endl;
        return 1;
    }
}
