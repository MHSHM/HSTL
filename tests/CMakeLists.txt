file(GLOB TEST_SOURCES *.cpp)

add_executable(HSTL_Tests ${TEST_SOURCES})

target_link_libraries(HSTL_Tests
	PRIVATE
		HSTL::HSTL
		Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(HSTL_Tests)