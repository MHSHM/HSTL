set(HSTL_PUBLIC_HEADERS include/Json_Value.h)

set(HSTL_SOURCES
    src/main.cpp
    src/Json_Value.cpp)

add_library(HSTL STATIC ${HSTL_SOURCES} ${HSTL_PUBLIC_HEADERS})

add_library(HSTL::HSTL ALIAS HSTL)

target_include_directories(HSTL
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(HSTL PUBLIC cxx_std_20)
