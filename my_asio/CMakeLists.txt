Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_library(asio "src/asio.cpp")
target_include_directories(asio PRIVATE inc)

add_executable(test "test/test.cpp")
target_link_libraries(test PRIVATE asio PRIVATE Catch2::Catch2WithMain)
target_include_directories(test PRIVATE inc)

target_compile_options(test PUBLIC "$<$<CONFIG:DEBUG>:${GCC_COMPILE_DEBUG_OPTIONS}>")
target_compile_options(test PUBLIC "$<$<CONFIG:RELEASE>:${GCC_COMPILE_RELEASE_OPTIONS}>")
