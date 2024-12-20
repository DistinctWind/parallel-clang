add_library(parallel-attribute
  ParallelAttribute.cpp
)

target_link_libraries(parallel-attribute
  PRIVATE
  clangTooling
  clangTransformer
)
