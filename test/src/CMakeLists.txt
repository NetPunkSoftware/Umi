add_executable(umi_core_test 
    impl.cpp
    test_all_storages.cpp
    test_orchestrator_moves.cpp
    test_scheme_view.cpp
    test_scheme.cpp)

target_link_libraries(umi_core_test PRIVATE umi_core_lib)
target_compile_features(umi_core_test PRIVATE cxx_std_20)
target_include_directories(umi_core_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(umi_core_test PRIVATE BOOST_ASIO_DISABLE_CONCEPTS)
    target_link_libraries(umi_core_test PRIVATE m)
endif()
