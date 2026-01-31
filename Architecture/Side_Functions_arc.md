# Global Configuration & Utility Components Documentation

This section documents the global configuration flags, color utilities, terminal execution wrappers, drive‑validation helpers, confirmation utilities, and recovery function declarations used throughout DriveMgr.

These components form the **core infrastructure** that other modules rely on.

------------------------------------------------------------
# VERSION Macro

    ```cpp
    #define VERSION std::string("v0.9.08-86_experimental")
    ```

Defines the current program version as a string.  
Used for displaying version info in menus, logs, and CLI output.

------------------------------------------------------------
# Terminal Mode Globals

    ```cpp
    struct termios oldt;
    struct termios newt;
    ```

These store terminal settings for:

- enabling/disabling raw mode  
- restoring original terminal state after TUI operations  

Used primarily by the interactive drive selector.

------------------------------------------------------------
# Global Flags

    ```cpp
    static bool g_dry_run = false;
    static bool g_no_color = false;
    ```

### `g_dry_run`
When enabled:

- no system commands are executed  
- instead, the program prints:  
  `[DRY-RUN] Would run: <command>`  
- logs the simulated command  

Useful for testing destructive operations safely.

### `g_no_color`
When enabled:

- disables ANSI color codes  
- all color functions return empty strings  

Useful for logs, scripts, or terminals without color support.

------------------------------------------------------------
# Color Namespace

    ```cpp
    namespace Color { ... }
    ```

Provides ANSI color escape sequences, automatically disabled if `g_no_color == true`.

### Available Colors:

- reset  
- red  
- green  
- yellow  
- blue  
- magenta  
- cyan  
- bold  
- inverse  

### Macros for convenience:

    ```cpp
    #define RED Color::red()
    #define BOLD Color::bold()
    ...
    ```

These allow easy use of colors in output:

    std::cout << RED << "Error!" << RESET;

------------------------------------------------------------
# Theme & Selection Colors

    ```cpp
    static std::string THEME_COLOR = RESET;
    static std::string SELECTION_COLOR = RESET;
    ```

Used by the TUI to highlight selected rows or apply themes.  
Can be modified by user settings or future theme modules.

------------------------------------------------------------
# Terminal Execution Wrappers [leagacy]

These functions wrap system command execution and integrate:

- dry‑run mode  
- logging  
- multiple execution backends (`execTerminal`, `execTerminalv2`, `execTerminalv3`)

### 1. `runTerminal()`

    ```cpp
    static std::string runTerminal(const std::string &cmd)
    ```

Uses:

    Terminalexec::execTerminal()

### 2. `runTerminalV2()`

    ```cpp
    static std::string runTerminalV2(const std::string &cmd)
    ```

Uses:

    Terminalexec::execTerminalv2()

### 3. `runTerminalV3()`

    ```cpp
    static std::string runTerminalV3(const std::string &cmd)
    ```

Uses:

    Terminalexec::execTerminalv3()

All three:

- respect `g_dry_run`  
- print simulated commands  
- log simulated commands  
- return empty string in dry‑run mode  

------------------------------------------------------------
# Drive List Cache

    ```cpp
    static std::vector<std::string> g_last_drives;
    ```

Stores the last list of drives retrieved by `listDrives(false)`.  
Used by validation functions to check if a drive exists.

------------------------------------------------------------
# checkDriveName()

    ```cpp
    void checkDriveName(const std::string &driveName)
    ```

Validates that a drive exists by:

1. Calling `listDrives(false)` to refresh `g_last_drives`
2. Checking:
   - full path match  
   - filename match (e.g., `/dev/sda` == `sda`)  

If not found:

- prints error  
- logs error  

Does **not** throw — simply returns.

------------------------------------------------------------
# confirmationKeyGenerator()

    ```cpp
    std::string confirmationKeyGenerator()
    ```

Generates a **10‑character random alphanumeric key** used for confirming destructive operations.

Character set includes:

- lowercase letters  
- uppercase letters  
- digits  

Uses:

- `std::mt19937` seeded with current time  
- uniform distribution  

------------------------------------------------------------
# askForConfirmation()

    ```cpp
    bool askForConfirmation(const std::string &prompt)
    ```

Displays a prompt and waits for `y` or `Y`.

If user declines:

- prints cancellation message  
- logs cancellation  
- returns `false`  

Otherwise returns `true`.

Also clears the input buffer to prevent leftover characters from interfering with later input.

------------------------------------------------------------
# File Recovery Function Declarations

    ```cpp
    void file_recovery_quick(const std::string& drive, const std::string& signature_key);
    void file_recovery_full(const std::string& drive, const std::string& signature_key);
    ```

These are forward declarations for:

- **quick recovery** (signature‑based scanning)  
- **full recovery** (deep scan)  

Actual implementations are defined elsewhere.

------------------------------------------------------------
# Summary

This module provides the foundational utilities used across DriveMgr:

- versioning  
- terminal mode handling  
- color output  
- dry‑run and no‑color modes  
- safe terminal command execution  
- drive validation  
- confirmation prompts  
- random key generation  
- recovery function declarations  

These components ensure consistent behavior, safety, and usability across all DriveMgr features.

------------------------------------------------------------
