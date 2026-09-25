# CMake generated Testfile for 
# Source directory: /Users/ishakishanvasisht/Multithreaded-HTTP-server
# Build directory: /Users/ishakishanvasisht/Multithreaded-HTTP-server/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(httpserver_tests "/Users/ishakishanvasisht/Multithreaded-HTTP-server/build/httpserver_tests")
set_tests_properties(httpserver_tests PROPERTIES  _BACKTRACE_TRIPLES "/Users/ishakishanvasisht/Multithreaded-HTTP-server/CMakeLists.txt;63;add_test;/Users/ishakishanvasisht/Multithreaded-HTTP-server/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
