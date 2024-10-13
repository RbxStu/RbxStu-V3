### RbxStu V3/Analysis

RbxStu V3's analysis submodule, contains classes used to aid in the analysis of ROBLOX Studio at runtime.

- `RbxStu::Analysis::Disassembler` wrapper around `capstone`.
- `RbxStu::Analysis::RTTI` wrapper to read Run-Time Type Information (MSVC)
- `RbxStu::Analysis::Scanner` wrapper around `libhat` to provide easier Scanning APIs

These tools allow the integrated RbxStu dumper to be easier to write, expand and maintain, whilst providing a top notch
development experience.

- ROBLOX Studio is not owned by RbxStu.