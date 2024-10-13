### RbxStu V3/Events

RbxStu V3's events submodule, contains a wrapper for RbxStu's Events, similar to RBXScriptSignals, but only for C++.

- `RbxStu::Events::Event` wrapper around an `std::vector<RbxStu::Events::Connection>`.
- `RbxStu::Events::Connection` returned as a result to a connection into an `RbxStu::Events::Event`

These tools allow for RbxStu's codebase to be more flexible and allow for a neat observer pattern to be written without
major effort.

- ROBLOX Studio is not owned by RbxStu.