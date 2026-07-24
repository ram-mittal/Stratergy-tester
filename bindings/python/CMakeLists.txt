find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.12.0
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(ultrabacktest_py module.cpp)
target_link_libraries(ultrabacktest_py PRIVATE ultrabacktest)
target_compile_definitions(ultrabacktest_py PRIVATE ULTRABACKTEST_PYBIND)
