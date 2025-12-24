set(HSTL_PUBLIC_HEADERS
    include/Result.h
    include/Defer.h
    include/Array.h
    include/Str.h
    include/Hash_Set.h
    include/Hash_Map.h
    include/Log.h)

set(HSTL_SOURCES)

add_library(HSTL STATIC ${HSTL_SOURCES} ${HSTL_PUBLIC_HEADERS})

set_target_properties(HSTL PROPERTIES LINKER_LANGUAGE CXX)

add_library(HSTL::HSTL ALIAS HSTL)

target_include_directories(HSTL
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(HSTL PUBLIC cxx_std_20)
