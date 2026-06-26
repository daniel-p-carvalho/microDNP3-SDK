# microDNP3 SDK (Software Development Kit)
    
Welcome to the official pre-compiled SDK for **microDNP3**, a lightweight DNP3 stack implementation optimized for resource-constrained embedded systems and microcontrollers (such as ARM Cortex-M4) running RTOS environments like **NuttX**.
    
This repository contains only the public API headers and pre-compiled static libraries, allowing you to easily integrate microDNP3 into your project without needing access to the private core source code.
    
## Repository Structure
    
```text
microdnp3-sdk/
├── include/              # Public API header files (.h)
│   ├── data_core.h       # Process Image database API
│   ├── data_core_time.h  # Timekeeping adapter
│   └── ...
├── lib/                  # Pre-compiled static libraries (.a)
│   ├── soft/             # ARM Cortex-M4 Soft-Float ABI library (libmicrodnp3.a)
│   └── hard/             # ARM Cortex-M4 Hard-Float ABI library (libmicrodnp3.a)
├── ports/                # Reference transport implementations (source & headers)
│   ├── tty_transport.c   # Serial/UART transport port
│   ├── tcp_transport.c   # TCP/IP transport port
│   └── ...
└── README.md
```

## How to Integrate

### 1. As a Git Submodule (Recommended)

You can add this SDK directly to your project using Git Submodules:

```code
git submodule add https://github.com/your-username/microdnp3-sdk.git path/to/your/project/sdk
```

### 2. Header Include Paths

Add the following directory to your compiler's include search paths:
•  sdk/include 
•  sdk/ports  (if using reference transport implementations)

### 3. Linking the Static Library

Link the appropriate library based on your target's FPU hardware configuration:
• For Software FPU (Soft-Float): link  sdk/lib/soft/libmicrodnp3.a 
• For Hardware FPU (Hard-Float): link  sdk/lib/hard/libmicrodnp3.a 

### 4. Compilation Flags

Ensure your toolchain compiles with C99 support ( -std=c99  or equivalent) as the API headers depend on standard integer types ( stdint.h ) and booleans ( stdbool.h ).

## License

This SDK is distributed under the BSD-3-Clause License. See the license file for details.
