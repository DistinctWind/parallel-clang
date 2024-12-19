add_executable(parallel-transformer
  ParallelAttribute.cpp
  ParallelTransformer.cpp
)

target_link_libraries(parallel-transformer
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangSerialization
  clangStaticAnalyzerFrontend
  clangTooling
  clangToolingSyntax
  clangTransformer
  GTest::gtest_main
)

gtest_discover_tests(parallel-transformer)