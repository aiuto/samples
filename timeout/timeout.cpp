#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cfloat>
#include <climits>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#endif

class Timeout {
private:
    int duration_seconds;
    int kill_after_seconds;
    std::string signal_name;
    bool preserve_status;
    bool verbose;
    std::vector<std::string> command_args;

    // Platform-specific process management
#ifdef _WIN32
    HANDLE process_handle;
    DWORD process_id;
#else
    pid_t child_pid;
#endif

public:
    Timeout() : duration_seconds(0), kill_after_seconds(0), 
                preserve_status(false), verbose(false) {
#ifdef _WIN32
        process_handle = INVALID_HANDLE_VALUE;
        process_id = 0;
#else
        child_pid = -1;
#endif
    }

    bool parse_duration(const std::string& duration_str) {
        if (duration_str.empty()) return false;
        
        size_t pos = 0;
        double value = std::stod(duration_str, &pos);
        
        if (pos < duration_str.length()) {
            char unit = duration_str[pos];
            switch (unit) {
                case 's': break; // seconds (default)
                case 'm': value *= 60; break; // minutes
                case 'h': value *= 3600; break; // hours
                case 'd': value *= 86400; break; // days
                default: return false;
            }
        }
        
        if (value < 0 || value > INT_MAX) return false;
        duration_seconds = static_cast<int>(value);
        return true;
    }

    int parse_signal(const std::string& signal_str) {
        if (signal_str.empty()) return SIGTERM;
        
        // Try to parse as number first
        try {
            return std::stoi(signal_str);
        } catch (...) {
            // Parse as signal name
            if (signal_str == "TERM" || signal_str == "SIGTERM") return SIGTERM;
            if (signal_str == "INT" || signal_str == "SIGINT") return SIGINT;
            if (signal_str == "HUP" || signal_str == "SIGHUP") return SIGHUP;
            if (signal_str == "KILL" || signal_str == "SIGKILL") return SIGKILL;
            if (signal_str == "USR1" || signal_str == "SIGUSR1") return SIGUSR1;
            if (signal_str == "USR2" || signal_str == "SIGUSR2") return SIGUSR2;
        }
        return SIGTERM; // default
    }

    bool parse_args(int argc, char* argv[]) {
        int i = 1;
        
        while (i < argc && argv[i][0] == '-') {
            std::string arg = argv[i];
            
            if (arg == "-k" || arg == "--kill-after") {
                if (++i >= argc) {
                    std::cerr << "Error: -k/--kill-after requires a duration\n";
                    return false;
                }
                if (!parse_duration(argv[i])) {
                    std::cerr << "Error: invalid kill-after duration: " << argv[i] << "\n";
                    return false;
                }
                kill_after_seconds = duration_seconds;
                duration_seconds = 0; // Reset for main duration
            } else if (arg == "-s" || arg == "--signal") {
                if (++i >= argc) {
                    std::cerr << "Error: -s/--signal requires a signal\n";
                    return false;
                }
                signal_name = argv[i];
            } else if (arg == "-p" || arg == "--preserve-status") {
                preserve_status = true;
            } else if (arg == "-v" || arg == "--verbose") {
                verbose = true;
            } else if (arg == "-h" || arg == "--help") {
                print_usage();
                exit(0);
            } else {
                std::cerr << "Error: unknown option: " << arg << "\n";
                return false;
            }
            i++;
        }
        
        // Parse main duration
        if (i >= argc) {
            std::cerr << "Error: duration and command required\n";
            return false;
        }
        
        if (!parse_duration(argv[i])) {
            std::cerr << "Error: invalid duration: " << argv[i] << "\n";
            return false;
        }
        i++;
        
        // Collect command arguments
        while (i < argc) {
            command_args.push_back(argv[i++]);
        }
        
        if (command_args.empty()) {
            std::cerr << "Error: command required\n";
            return false;
        }
        
        return true;
    }

    void print_usage() {
        std::cout << "Usage: timeout [OPTION] DURATION COMMAND [ARG]...\n"
                  << "Start COMMAND, and kill it if still running after DURATION.\n\n"
                  << "Options:\n"
                  << "  -k, --kill-after=DURATION  also send a KILL signal if COMMAND is still\n"
                  << "                              running this long after the initial signal\n"
                  << "  -p, --preserve-status      return exit status of COMMAND, not timeout\n"
                  << "  -s, --signal=SIGNAL        specify the signal to be sent on timeout\n"
                  << "  -v, --verbose              diagnose to stderr any signal sent\n"
                  << "  -h, --help                 display this help and exit\n\n"
                  << "DURATION is a floating point number with an optional suffix:\n"
                  << "'s' for seconds (the default), 'm' for minutes, 'h' for hours or 'd' for days.\n";
    }

#ifdef _WIN32
    bool start_process() {
        if (command_args.empty()) return false;
        
        std::string command_line;
        for (const auto& arg : command_args) {
            if (!command_line.empty()) command_line += " ";
            command_line += "\"" + arg + "\"";
        }
        
        STARTUPINFO si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);
        
        if (!CreateProcess(
            NULL,
            const_cast<char*>(command_line.c_str()),
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi)) {
            std::cerr << "Error: failed to start process\n";
            return false;
        }
        
        process_handle = pi.hProcess;
        process_id = pi.dwProcessId;
        CloseHandle(pi.hThread);
        
        return true;
    }
    
    bool wait_for_process(int timeout_seconds) {
        if (process_handle == INVALID_HANDLE_VALUE) return false;
        
        DWORD result = WaitForSingleObject(process_handle, timeout_seconds * 1000);
        
        if (result == WAIT_OBJECT_0) {
            // Process completed normally
            DWORD exit_code;
            GetExitCodeProcess(process_handle, &exit_code);
            CloseHandle(process_handle);
            return true;
        } else if (result == WAIT_TIMEOUT) {
            // Process timed out
            return false;
        }
        
        return false;
    }
    
    void terminate_process() {
        if (process_handle != INVALID_HANDLE_VALUE) {
            TerminateProcess(process_handle, 1);
            CloseHandle(process_handle);
        }
    }
    
    void kill_process() {
        if (process_handle != INVALID_HANDLE_VALUE) {
            TerminateProcess(process_handle, 1);
            CloseHandle(process_handle);
        }
    }
#else
    bool start_process() {
        if (command_args.empty()) return false;
        
        child_pid = fork();
        
        if (child_pid == -1) {
            std::cerr << "Error: failed to fork process\n";
            return false;
        }
        
        if (child_pid == 0) {
            // Child process
            std::vector<char*> args;
            for (const auto& arg : command_args) {
                args.push_back(const_cast<char*>(arg.c_str()));
            }
            args.push_back(nullptr);
            
            execvp(args[0], args.data());
            std::cerr << "Error: failed to execute command\n";
            exit(127);
        }
        
        return true;
    }
    
    bool wait_for_process(int timeout_seconds) {
        if (child_pid == -1) return false;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (true) {
            int status;
            pid_t result = waitpid(child_pid, &status, WNOHANG);
            
            if (result == child_pid) {
                // Process completed
                return true;
            } else if (result == -1) {
                // Error
                return false;
            }
            
            // Check if timeout has elapsed
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            
            if (elapsed.count() >= timeout_seconds) {
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    void terminate_process() {
        if (child_pid != -1) {
            int signal = parse_signal(signal_name);
            if (verbose) {
                std::cerr << "Sending signal " << signal << " to process " << child_pid << "\n";
            }
            kill(child_pid, signal);
        }
    }
    
    void kill_process() {
        if (child_pid != -1) {
            if (verbose) {
                std::cerr << "Sending SIGKILL to process " << child_pid << "\n";
            }
            kill(child_pid, SIGKILL);
        }
    }
#endif

    int run() {
        if (!start_process()) {
            return 125; // timeout itself fails
        }
        
        // Wait for the main duration
        if (wait_for_process(duration_seconds)) {
            // Process completed within time limit
            return 0;
        }
        
        // Process timed out, send initial signal
        terminate_process();
        
        // If kill-after is specified, wait and then kill
        if (kill_after_seconds > 0) {
            if (wait_for_process(kill_after_seconds)) {
                // Process terminated after initial signal
                return preserve_status ? 0 : 124;
            }
            
            // Process still running, send KILL signal
            kill_process();
            
            // Wait a bit for the process to terminate
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            return 137; // KILL signal sent
        }
        
        return preserve_status ? 0 : 124; // Timeout occurred
    }
};

int main(int argc, char* argv[]) {
    Timeout timeout;
    
    if (!timeout.parse_args(argc, argv)) {
        timeout.print_usage();
        return 125;
    }
    
    return timeout.run();
} 