set(_SOURCE_FILES 
ParallelAttribute.cpp 
ParallelLintAction.cpp
)

add_llvm_component_library(ParallelLintPlugin
    MODULE ${_SOURCE_FILES}
    PLUGIN_TOOL clang
)