### RbxStu V3/Analysis

RbxStu V3's analysis submodule, contains classes used to aid in the analysis of ROBLOX Studio at runtime.

- `RbxStu::Analysis::Disassembler` wrapper around `capstone`.
- `RbxStu::Analysis::RTTI` wrapper to read Run-Time Type Information (MSVC), wrapper around `RTTIHook`
- `RbxStu::Analysis::Scanner`, superseeded by `libhat`, which has intrinsics and more features useful for game modding!

These tools allow the integrated RbxStu dumper to be easier to write, expand and maintain, whilst providing a top notch
development experience.

- ROBLOX Studio is not owned by RbxStu.