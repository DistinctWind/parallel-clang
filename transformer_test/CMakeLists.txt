add_executable(parallel-transformer-test
  ParallelTransformerTest.cpp
)

target_link_libraries(parallel-transformer-test
  PRIVATE
  clangTooling
  clangTransformer
  ParallelAttribute
  GTest::gtest_main
)

gtest_discover_tests(parallel-transformer-test)