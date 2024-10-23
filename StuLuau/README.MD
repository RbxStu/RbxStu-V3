### RbxStu V3/Luau

RbxStu V3's luau submodule, provides a flexible luau environment with stable execution.

- `RbxStu::Luau::ExecutionEngine` Custom execution engine tailor built for ROBLOX Studio environments, this is the main way that the RbxStu codebase should interact with `RbxStu::Scheduling::Jobs::ExecuteScriptJob` and `RbxStu::Scheduling::Jobs::ResumeYieldedThreadsJob` defined under `RbxStu::Scheduling`. The execution engine is in charge of:
  - Yielding
  - Execution
  - Compilation
  - Environment Bootstrapping
  - And more...

- `RbxStu::Luau::Environment::...` Custom environment for RbxStu V3.

This all allows for execution to be efficiently written and initialized with proper functions.

- ROBLOX Studio is not owned by RbxStu.