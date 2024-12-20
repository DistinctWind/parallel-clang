add_executable(parallel-transformer
  ParallelTransformer.cpp
)

target_link_libraries(parallel-transformer
  PRIVATE
  clangTooling
  clangTransformer
  ParallelAttribute
  ParallelASTMatcher
)
