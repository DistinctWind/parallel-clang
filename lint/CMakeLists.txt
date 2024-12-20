set(_SOURCE_FILES 
ParallelLintAction.cpp
)

add_llvm_component_library(ParallelLintPlugin
    MODULE ${_SOURCE_FILES}
    PLUGIN_TOOL clang
)

target_link_libraries(ParallelLintPlugin PRIVATE
    ParallelAttribute
    ParallelASTMatcher
)