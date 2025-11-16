add_executable(vms-core-tests
    thread_tests.cpp
)

target_link_libraries(vms-core-tests
    PRIVATE
        vms-core
)

add_test(NAME vms_core_unit_tests COMMAND vms-core-tests)
