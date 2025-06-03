add_executable(HSTL_Playground
               src/main.cpp)

# Link against the library
target_link_libraries(HSTL_Playground PRIVATE HSTL::HSTL)
