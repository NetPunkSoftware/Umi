include(FetchContent)



####################################################################################
# Boost
find_package(Boost ${BOOST_VERSION} REQUIRED)
target_include_directories(umi_core_lib SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(umi_core_lib PUBLIC ${Boost_LIBRARY_DIRS})



####################################################################################
# Tamashii
FetchContent_Declare(
    tamashii
    GIT_REPOSITORY https://github.com/NetPunkSoftware/Tamashii
    GIT_TAG        master
)

FetchContent_GetProperties(tamashii)
if(NOT tamashii_POPULATED)
    FetchContent_Populate(tamashii)
    add_subdirectory(${tamashii_SOURCE_DIR} ${tamashii_BINARY_DIR})
endif()

target_link_libraries_system(umi_core_lib PUBLIC tamashii)



####################################################################################
# Tao tuple
set(TAOCPP_TUPLE_BUILD_TESTS OFF CACHE INTERNAL "")

FetchContent_Declare(
    tuple
    GIT_REPOSITORY https://github.com/taocpp/tuple
    GIT_TAG        97618cee3ffe7738acea87871abdf5dddd415602
)

FetchContent_GetProperties(tuple)
if(NOT tuple_POPULATED)
    FetchContent_Populate(tuple)
    add_subdirectory(${tuple_SOURCE_DIR} ${tuple_BINARY_DIR})
endif()

target_link_libraries_system(umi_core_lib PUBLIC tuple)



####################################################################################
# Range-v3
FetchContent_Declare(
    range_v3_dep
    GIT_REPOSITORY https://github.com/ericniebler/range-v3
    GIT_TAG        9aa032ccd0eb4bd77f58e5b181168f1038c222c6
)

FetchContent_GetProperties(range_v3_dep)
if(NOT range_v3_dep_POPULATED)
    FetchContent_Populate(range_v3_dep)
    add_subdirectory(${range_v3_dep_SOURCE_DIR} ${range_v3_dep_BINARY_DIR})
endif()

target_link_libraries_system(umi_core_lib PUBLIC range-v3)



####################################################################################
# SpdLog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        v1.9.2
)

FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()

target_link_libraries(umi_core_lib PUBLIC spdlog)
