if (BUILD_SHARED_LIBS)
    set(EXTRA_LIBS ${EXTRA_LIBS} wampcc_shared wampcc_json_shared)
else ()
    set(EXTRA_LIBS ${EXTRA_LIBS} wampcc_static wampcc_json_static)
endif ()

list(APPEND EXTRA_LIBS ${LIBUV_LIBRARIES} ${OPENSSL_LIBRARIES} ${JANSSON_LIBRARIES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread ")

# Helper macro for example compilation
macro(Compile_App app source_path)
    add_executable(${app} "${PROJECT_SOURCE_DIR}/apps/${source_path}/${app}.cc")
    set_property(TARGET ${app} PROPERTY CXX_STANDARD 17)
    set_property(TARGET ${app} PROPERTY CXX_STANDARD_REQUIRED ON)
    target_link_libraries(${app} ${EXTRA_LIBS})

    if (WIN32)
        set_target_properties(${app} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib")
    endif ()
endmacro()


if (BUILD_APPS)

    # Basic
   # Compile_App(manager manager)
    Compile_App(platform platform)

endif () # BUILD_EXAMPLES


add_subdirectory(manager)