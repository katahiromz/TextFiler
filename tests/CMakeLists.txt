# tests/CMakeLists.txt

add_executable(test_textfiler test_posix.cpp)
target_include_directories(test_textfiler PRIVATE ..)
target_link_libraries(test_textfiler PRIVATE libTextFiler)

add_test(NAME textfiler_test COMMAND test_textfiler)
