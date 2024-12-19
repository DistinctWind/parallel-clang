add_executable(parallel-transformer
  ParallelAttribute.cpp
  ParallelTransformer.cpp
)

target_link_libraries(parallel-transformer
  PRIVATE
  clangTooling
  clangTransformer
)

add_executable(parallel-transformer-test
  ParallelAttribute.cpp
  ParallelTransformerTest.cpp
)

target_link_libraries(parallel-transformer-test
  PRIVATE
  clangTooling
  clangTransformer
  GTest::gtest_main
)

gtest_discover_tests(parallel-transformer-test)