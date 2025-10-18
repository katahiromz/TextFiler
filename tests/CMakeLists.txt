# tests/CMakeLists.txt

if(WIN32)
    add_executable(test_win test_win.cpp)
    target_compile_definitions(test_win PRIVATE UNICODE _UNICODE NO_OLDNAMES)
    target_include_directories(test_win PRIVATE ..)
    target_link_libraries(test_win PRIVATE libTextFiler)

    add_test(NAME test_win COMMAND test_win)
else()
    add_executable(test_posix test_posix.cpp)
    target_include_directories(test_posix PRIVATE ..)
    target_link_libraries(test_posix PRIVATE libTextFiler)

    add_test(NAME test_win COMMAND test_posix)
endif()
