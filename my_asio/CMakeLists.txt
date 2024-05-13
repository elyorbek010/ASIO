Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_library(my_asio   "src/io_context.cpp" )
target_include_directories(my_asio PRIVATE inc)
include_directories("detail")

add_executable(test "test/test.cpp")
target_link_libraries(test PRIVATE my_asio PRIVATE Catch2::Catch2WithMain)
target_include_directories(test PRIVATE inc)

include_directories("/mnt/c/Users/elyor/OneDrive/Desktop/Boostlin/boost_1_84_0")
include_directories("C:/Users/elyor/OneDrive/Desktop/Boostlin/boost_1_84_0/stage/x64-msvc/lib")

target_compile_options(test PUBLIC "$<$<CONFIG:DEBUG>:${GCC_COMPILE_DEBUG_OPTIONS}>")
target_compile_options(test PUBLIC "$<$<CONFIG:RELEASE>:${GCC_COMPILE_RELEASE_OPTIONS}>")
