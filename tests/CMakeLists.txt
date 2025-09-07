file(GLOB TEST_SOURCES *.cpp)

add_executable(HSTL_tests ${TEST_SOURCES})

target_link_libraries(HSTL_tests
	PRIVATE
		HSTL::HSTL
		Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(HSTL_tests)