if (BUILD_SHARED_LIBS)
    set(EXTRA_LIBS ${EXTRA_LIBS} wampcc_shared wampcc_json_shared)
else ()
    set(EXTRA_LIBS ${EXTRA_LIBS} wampcc_static wampcc_json_static)
endif ()

set (CMAKE_CXX_STANDARD 17)

list(APPEND EXTRA_LIBS ${LIBUV_LIBRARIES} ${OPENSSL_LIBRARIES} ${JANSSON_LIBRARIES} ${LIST_HEADERS})

add_executable(manager manager.cc mysql_client.h polyfill/datetime.h polyfill/function_traits.h polyfill/optional.hpp type_cast.h)
set_property(TARGET manager PROPERTY CXX_STANDARD 17)
set_property(TARGET manager PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(manager ${EXTRA_LIBS} -lmysqlclient)