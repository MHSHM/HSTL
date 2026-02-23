# 1. Define the headers
set(HSTL_PUBLIC_HEADERS
    include/Result.h
    include/Defer.h
    include/Array.h
    include/Str.h
    include/Hash_Set.h
    include/Hash_Map.h
    include/Log.h
    include/Memory.h
    include/File.h
    include/Fixed_Str.h
    include/Str_Format.h
    include/Thread.h
    include/Mutex.h
    include/Condition_Variable.h
    include/Fixed_Queue.h
    include/Task.h
    include/Job_System.h)

add_library(HSTL INTERFACE)
add_library(HSTL::HSTL ALIAS HSTL)

target_compile_definitions(HSTL INTERFACE NOMINMAX _WIN32_WINNT=0x0A00)

target_include_directories(HSTL
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(HSTL INTERFACE cxx_std_20)

target_sources(HSTL INTERFACE ${HSTL_PUBLIC_HEADERS})
