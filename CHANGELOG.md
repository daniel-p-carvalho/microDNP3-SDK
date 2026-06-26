# Changelog - microDNP3

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.2.0] - 2026-06-23

### Added
- **Automated SDK Build Script**: Introduced [build_sdk.sh](file:///home/daniel/microdnp3-workspace/microDNP3/tools/build_sdk.sh) in `tools/` to automate configuring, building, and packaging for both Native (Linux) and ARM (Cross-compilation) platforms.
- **Dynamic Configuration Structure**: Created `struct dc_config_s` to allow the user application to dynamically register Process Image buffers and point counts at runtime.
- **Dynamic Bounds Checking**: Added runtime validation for all Process Image read/write and pointer-model APIs.

### Changed
- **Process Image Buffer Storage**: Refactored `struct data_core_s` to store pointers instead of compile-time sized arrays. This stabilizes the ABI layout, enabling pre-compiled library distribution.
- **Deep Copy for Copy Model**: Modified `dc_sync_to_buffer` and `dc_sync_from_buffer` to perform deep copying of registered buffer data instead of shallow struct copies.
- **Linux Exclusions for Generic Targets**: Updated `CMakeLists.txt` to exclude Linux-specific TTY/TCP transport sources and native unit tests when cross-compiling for bare-metal targets (`Generic` target).

### Fixed
- **Unit Test Macro Typos**: Fixed macro checks (`CONFIG_DC_ENABLE_...` to `CONFIG_UDNP3_ENABLE_...`) in test files to ensure pointer and copy model tests are correctly compiled and executed.

