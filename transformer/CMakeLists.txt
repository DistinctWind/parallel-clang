add_executable(parallel-transformer
  ParallelAttribute.cpp
  ParallelTransformer.cpp
)

target_link_libraries(parallel-transformer
  PRIVATE
  clangTooling
  clangTransformer
)
