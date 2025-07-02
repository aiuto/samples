# Timeout

A cross-platform implementation of the GNU `timeout` command that runs a command with a time limit and kills it if it's still running after the specified duration.

## Features

- **Cross-platform**: Works on Linux, macOS, and Windows
- **Signal-based termination**: Uses signals on Unix-like systems, `TerminateProcess()` on Windows
- **Flexible duration parsing**: Supports seconds (s), minutes (m), hours (h), and days (d)
- **Custom signals**: Specify which signal to send on timeout
- **Kill-after option**: Send a KILL signal after an additional delay
- **Verbose output**: Diagnose signal sending
- **Preserve status**: Return the command's exit status instead of timeout status

## Building with Bazel

### Prerequisites

- [Bazel](https://bazel.build/install) installed on your system

### Build Commands

```bash
# Build for current platform
bazel build //:timeout

# Build for specific platforms
bazel build --platforms=@io_bazel_rules_go//go/toolchain:linux_amd64 //:timeout
bazel build --platforms=@io_bazel_rules_go//go/toolchain:darwin_amd64 //:timeout
bazel build --platforms=@io_bazel_rules_go//go/toolchain:windows_amd64 //:timeout

# Run the built binary
bazel run //:timeout -- 5s echo "Hello, World!"

# Clean build artifacts
bazel clean
```

### Alternative: Simple Makefile

For Unix-like systems, you can also use the provided Makefile:

```bash
# Build
make

# Install to /usr/local/bin
sudo make install

# Run tests
make test

# Clean
make clean
```

## Usage

```bash
timeout [OPTION] DURATION COMMAND [ARG]...
```

### Options

- `-k, --kill-after=DURATION`: Also send a KILL signal if COMMAND is still running this long after the initial signal
- `-p, --preserve-status`: Return exit status of COMMAND, not timeout
- `-s, --signal=SIGNAL`: Specify the signal to be sent on timeout
- `-v, --verbose`: Diagnose to stderr any signal sent
- `-h, --help`: Display help and exit

### Duration Format

DURATION is a floating point number with an optional suffix:
- `s` for seconds (the default)
- `m` for minutes
- `h` for hours
- `d` for days

### Examples

```bash
# Basic timeout - terminate after 5 seconds
timeout 5s sleep 10

# Use INT signal instead of TERM
timeout -s INT 5s sleep 10

# Send KILL signal 3 seconds after initial signal
timeout -k 3s 5s sleep 10

# Verbose output
timeout -v -s INT 5s sleep 10

# Preserve command exit status
timeout -p 5s echo "Hello"

# Timeout with minutes
timeout 2m long_running_command

# Timeout with hours
timeout 1h backup_script
```

### Exit Codes

- `0`: Command completed successfully
- `124`: Command timed out (unless `--preserve-status` is used)
- `125`: Timeout itself failed
- `126`: Command found but cannot be invoked
- `127`: Command cannot be found
- `137`: KILL signal was sent (128 + 9)

## Implementation Details

### Platform-Specific Behavior

- **Linux/macOS**: Uses `fork()`, `execvp()`, `kill()`, and `waitpid()` with signals
- **Windows**: Uses `CreateProcess()`, `WaitForSingleObject()`, and `TerminateProcess()`

### Signal Handling

On Unix-like systems, the following signals are supported:
- `TERM`/`SIGTERM` (default)
- `INT`/`SIGINT`
- `HUP`/`SIGHUP`
- `KILL`/`SIGKILL`
- `USR1`/`SIGUSR1`
- `USR2`/`SIGUSR2`

On Windows, `TerminateProcess()` is used for both normal termination and forced kill.

## Testing

```bash
# Run all tests
bazel test //:timeout_test

# Or with Makefile
make test
```

## License

This implementation follows the same interface as GNU Coreutils timeout but is a standalone implementation. 