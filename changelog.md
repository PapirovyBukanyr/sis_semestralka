# Changelog
All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

## [Unreleased] - 2025-11-02
### Added
- Added .gitignore to exclude log files from version control.
- Memory files 
- Scripts for improving UI

### Changed
- Modified nn.c to handle my data as vectors
- Created better neuron structure in neuron.c and neuron_layer.c
- Output via dla and not just terminal
- Makefile fix
- Changed inner structure to be more clean 
    - working with network in separate files
    - working with history in separate files
    - declaration of used datastructures in separate files

### Removed
- Logs deleted

## 0.0.1 - 2025-11-02
### Added
- Created basic project structure and initial files via ChatGPT.
- Data sets for data center examples added in CSV format.
- Initial implementation of net_logger.c for network logging functionality.
- Initial implementation of receiver - main.c for receiving and processing logs into C structures.
- Added Makefile for building the project on multiple platforms (Linux, Windows, macOS).
- Added logs for debugging and error handling in make, net_logger.c and main.c, because in terminal it was mess.

### Changed
- Updated net_logger.c to use a consistent socket_t type across platforms.
- Improved timestamp handling in net_logger.c for better accuracy.
- Refactored sleep and close socket functions in net_logger.c for clarity.
- Enhanced error handling in net_logger.c for socket operations.
- Modified Makefile to include platform-specific compilation flags.

