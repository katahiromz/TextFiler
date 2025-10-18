# tests/CMakeLists.txt

if(WIN32)
    add_executable(test_win test_win.cpp)
    target_compile_definitions(test_win PRIVATE UNICODE _UNICODE)
    target_include_directories(test_win PRIVATE ..)
    target_link_libraries(test_win PRIVATE libTextFiler)

    add_test(NAME test_win_test COMMAND test_win WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
else()
    add_executable(test_posix test_posix.cpp)
    target_include_directories(test_posix PRIVATE ..)
    target_link_libraries(test_posix PRIVATE libTextFiler)

    add_test(NAME test_posix_test COMMAND test_posix WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()
