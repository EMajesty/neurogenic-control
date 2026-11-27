add_library(neurogenic_protocol_fixture src/protocol_fixture.cpp)
target_include_directories(neurogenic_protocol_fixture PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/upstream)
target_compile_features(neurogenic_protocol_fixture PUBLIC cxx_std_20)
