#pragma once

#include "DmgrLib.h"
#include "debug.h"
#include "exec_cmd.h"
#include "StringUtils.hpp"

// ========== Test Framework ==========
// v0.9.0 - Simple test harness with result tracking and reporting

struct TestResult {
    std::string name;
    bool success;
    std::string message;
};

/**
 * @brief RAII wrapper to restore stdin after mocking in tests
 */
struct CinRestorer {
    std::streambuf* old;
    CinRestorer() : old(std::cin.rdbuf()) {}
    ~CinRestorer() { std::cin.rdbuf(old); }
};

// ========== Command Execution Tests ==========
TestResult test_cmdexec_success() {
    auto res = EXEC_QUIET("echo hello");

    if (!res.success)

        return {"test_cmdexec_success", false, "Command execution failed unexpectedly"};

    if (res.output.find("hello") == std::string::npos)

        return {"test_cmdexec_success", false, "Output missing expected 'hello' string"};

    return {"test_cmdexec_success", true, ""};
}

TestResult test_cmdexec_failure() {
    auto res = EXEC_QUIET("false");

    if (res.success)

        return {"test_cmdexec_failure", false, "Command should have failed but reported success"};

    if (res.exit_code == 0)

        return {"test_cmdexec_failure", false, "Exit code was 0, expected non-zero"};

    return {"test_cmdexec_failure", true, ""};
}

TestResult test_cmdexec_stderr() {
    auto res = EXEC_QUIET("ls /this/path/does/not/exist 2>&1");

    if (res.success)

        return {"test_cmdexec_stderr", false, "Command should have failed due to invalid path"};

    if (res.output.find("No such file") == std::string::npos && res.output.find("not found") == std::string::npos)
        
        return {"test_cmdexec_stderr", false, "stderr not captured correctly"};

    return {"test_cmdexec_stderr", true, ""};
}

TestResult test_cmdexec_combined_output() {
    auto res = EXEC_QUIET("bash -c \"echo OUT; echo ERR 1>&2\"");

    if (!res.success)

        return {"test_cmdexec_combined_output", false, "Command failed unexpectedly"};

    if (res.output.find("OUT") == std::string::npos)
    
        return {"test_cmdexec_combined_output", false, "Missing stdout 'OUT' in combined output"};

    if (res.output.find("ERR") == std::string::npos)

        return {"test_cmdexec_combined_output", false, "Missing stderr 'ERR' in combined output"};

    return {"test_cmdexec_combined_output", true, ""};
}

TestResult test_cmdexec_dry_run() {
    Globals::g_dry_run = true;
    std::cout << "\n";
    auto res = EXEC_QUIET("touch /tmp/test_drivemgr_dryrun_test_file_12345");
    Globals::g_dry_run = false;

    // Verify command wasn't actually executed
    if (fileExists("/tmp/test_drivemgr_dryrun_test_file_12345"))

        return {"test_cmdexec_dry_run", false, "Dry-run executed command when it should not have"};

    if (!res.success)

        return {"test_cmdexec_dry_run", false, "Dry-run should always return success"};

    return {"test_cmdexec_dry_run", true, ""};
}

TestResult test_cmdexec_long_running() {
    auto res = EXEC("sleep 1");

    if (!res.success)

        return {"test_cmdexec_long_running", false, "Long-running command failed"};

    return {"test_cmdexec_long_running", true, ""};
}

TestResult test_cmdexec_exit_code() {
    auto res = EXEC("bash -c \"exit 42\"");

    if (res.exit_code != 42)

        return {"test_cmdexec_exit_code", false, "Exit code mismatch (expected 42, got " + std::to_string(res.exit_code) + ")"};

    return {"test_cmdexec_exit_code", true, ""};
}

// ========== Input Validation Tests ==========

TestResult test_getInt_valid() {
    CinRestorer restore;
    std::istringstream fakeInput("5\n");
    std::cin.rdbuf(fakeInput.rdbuf());

    auto res = InputValidation::getInt({1,2,3,4,5});

    if (!res.has_value())

        return {"test_getInt_valid", false, "Expected valid integer but got nullopt"};

    if (res.value() != 5)

        return {"test_getInt_valid", false, "Returned value mismatch (expected 5, got " + std::to_string(res.value()) + ")"};

    return {"test_getInt_valid", true, ""};
}

TestResult test_getInt_invalid() {
    CinRestorer restore;
    std::istringstream fakeInput("99\n");
    std::cin.rdbuf(fakeInput.rdbuf());

    auto res = InputValidation::getInt({1,2,3,4,5});

    if (res.has_value())


        return {"test_getInt_invalid", false, "Expected nullopt for out-of-range input, but got value"};

    return {"test_getInt_invalid", true, ""};
}

TestResult test_getInt_range() {
    CinRestorer restore;
    std::istringstream fakeInput("3\n");
    std::cin.rdbuf(fakeInput.rdbuf());

    auto res = InputValidation::getInt(1, 5);

    if (!res.has_value())

        return {"test_getInt_range", false, "Expected valid value within range"};

    if (res.value() != 3)

        return {"test_getInt_range", false, "Range validation failed (expected 3, got " + std::to_string(res.value()) + ")"};

    return {"test_getInt_range", true, ""};
}

TestResult test_getInt_non_numeric() {
    CinRestorer restore;
    std::istringstream fakeInput("abc\n");
    std::cin.rdbuf(fakeInput.rdbuf());

    auto res = InputValidation::getInt({1,2,3,4,5});

    if (res.has_value())

        return {"test_getInt_non_numeric", false, "Expected nullopt for non-numeric input"};

    return {"test_getInt_non_numeric", true, ""};
}

TestResult test_getChar_valid() {
    CinRestorer restore;
    std::istringstream fakeInput("y\n");
    std::cin.rdbuf(fakeInput.rdbuf());

    auto res = InputValidation::getChar({'y','n'});

    if (!res.has_value())

        return {"test_getChar_valid", false, "Expected valid character"};

    if (res.value() != 'y')

        return {"test_getChar_valid", false, "Character mismatch (expected 'y')"};

    return {"test_getChar_valid", true, ""};
}

TestResult test_getChar_invalid() {
    CinRestorer restore;
    std::istringstream fakeInput("x\n");
    std::cin.rdbuf(fakeInput.rdbuf());

    auto res = InputValidation::getChar({'y','n'});

    if (res.has_value())

        return {"test_getChar_invalid", false, "Expected nullopt for invalid character"};

    return {"test_getChar_invalid", true, ""};
}

TestResult test_getChar_too_long() {
    CinRestorer restore;
    std::istringstream fakeInput("yes\n");
    std::cin.rdbuf(fakeInput.rdbuf());

    auto res = InputValidation::getChar();

    if (res.has_value())

        return {"test_getChar_too_long", false, "Expected nullopt for multi-character input"};

    return {"test_getChar_too_long", true, ""};
}

TestResult test_confirmationKeyGenerator() {
    std::string key = confirmationKeyGenerator();

    if (key.size() != 10)
        return {"test_confirmationKeyGenerator", false, "Key length mismatch (expected 10, got " + std::to_string(key.size()) + ")"};

    for (char c : key) {

        if (!std::isalnum(c))

            return {"test_confirmationKeyGenerator", false, "Key contains non-alphanumeric character: '" + std::string(1, c) + "'"};
    
        }

    return {"test_confirmationKeyGenerator", true, ""};
}

TestResult test_removeFirstLines() {
    std::string input = "line1\nline2\nline3\n";
    std::string expected = "line3\n";
    std::string output = StrUtils::removeFirstLines(input, 2);

    if (output != expected)

        return {"test_removeFirstLines", false, "Line removal failed (expected '" + expected + "', got '" + output + "')"};

    return {"test_removeFirstLines", true, ""};
}

TestResult test_fileExists() {
    std::string path = filePathHandler("/.local/share/DriveMgr_3/data/config.conf");
    
    if (!fileExists(path))

        return {"test_fileExists", false, "Config file should exist at: " + path};

    if (fileExists("/this/path/definitely/does/not/exist/12345"))

        return {"test_fileExists", false, "Nonexistent path incorrectly reported as existing"};

    return {"test_fileExists", true, ""};
}

TestResult test_filePathHandler() {
    std::string input = "./test/path/file.txt";
    std::string path = filePathHandler(input);

    if (path.empty()) {

        return {"test_filePathHandler", false, "filePathHandler returned empty string for valid input"};
   
    } else if (path.find("~") != std::string::npos) {

        return {"test_filePathHandler", true, ""};
    
    }

    return {"test_filePathHandler", true, ""};
}

TestResult test_StrUtils_trimWhiteSpace() {
    std::string s = " string ";
    std::string new_s = StrUtils::trimWhiteSpace(s);

    if (new_s.find(" ") != std::string::npos || new_s == s) {
        return {"test_StrUtils_trimWhiteSpace", false, "contains whitespace"};
    }

    return {"test_StrUtils_trimWhiteSpace", true, ""};
}

TestResult test_StrUtils_replaceAll() {
    std::string s = "test";
    std::string s2 = "test";
    s2 = StrUtils::replaceAll("hamburger");

    if (s == s2) {
        return {"test_StrUtils_replaceAll", false, "string couldnt be overwritten"};
    }
    return {"test_StrUtils_replaceAll", true, ""};
}

TestResult test_StrUtils_toLowerCase() {
    std::string s = "HAMBURGER";
    std::string l_s = StrUtils::toLowerString(s);

    if (s == l_s) {
        return {"test_StrUtils_toLower", false, "Didnt convert upper case to lower case"};
    }
    return {"test_StrUtils_toLower", true, ""};
}

TestResult test_StrUtils_toUpperCase() {
    std::string s = "Hamburger";
    std::string l_s = StrUtils::toUpperString(s);

    if (s == l_s) {
        return {"test_StrUtils_toLower", false, "Didnt convert lower case to upper case"};
    }
    return {"test_StrUtils_toLower", true, ""};
}



std::vector<TestResult> run_all_tests_internal() {
    std::vector<TestResult> results;

    std::cout << BOLD << "\n========== Running Tests ==========" << RESET << "\n\n";

    // Command execution tests
    std::cout << CYAN << "\n[CmdExec Tests]" << RESET << "\n";
    results.push_back(test_cmdexec_success());
    results.push_back(test_cmdexec_failure());
    results.push_back(test_cmdexec_stderr());
    results.push_back(test_cmdexec_combined_output());
    results.push_back(test_cmdexec_dry_run());
    results.push_back(test_cmdexec_long_running());
    results.push_back(test_cmdexec_exit_code());

    // Input validation tests
    std::cout << "\n" << CYAN << "\n[Input Validation Tests]" << RESET << "\n";
    results.push_back(test_getInt_valid());
    results.push_back(test_getInt_invalid());
    results.push_back(test_getInt_range());
    results.push_back(test_getInt_non_numeric());
    results.push_back(test_getChar_valid());
    results.push_back(test_getChar_invalid());
    results.push_back(test_getChar_too_long());

    // Utility tests
    std::cout << "\n" << CYAN << "[Utility Tests]" << RESET << "\n";
    results.push_back(test_confirmationKeyGenerator());
    results.push_back(test_removeFirstLines());
    results.push_back(test_fileExists());
    results.push_back(test_filePathHandler());
    results.push_back(test_StrUtils_trimWhiteSpace());
    results.push_back(test_StrUtils_replaceAll());
    results.push_back(test_StrUtils_toLowerCase());
    results.push_back(test_StrUtils_toUpperCase());

    return results;
}

/**
 * @brief Prints a colored test summary with pass/fail statistics
 * @param results Vector of test results to summarize
 */
void print_test_summary(const std::vector<TestResult>& results) {
    int passed = 0;
    int failed = 0;

    for (const auto& r : results) {

        if (r.success) {

            passed++;

        } else {

            failed++;

        }
    }

    // Print summary header
    std::cout << "\n" << BOLD << "======== Test Summary ========" << RESET << "\n";
    
    if (passed > 0) {

        std::cout << GREEN << "[PASSED]" << RESET << " " << passed << "\n";

    }
    
    if (failed > 0) {

        std::cout << RED << "[FAILED]" << RESET << " " << failed << "\n";

    }

    // Print failed test details
    if (failed > 0) {

        std::cout << "\n" << YELLOW << "[Failed Tests Details]" << RESET << "\n";

        for (const auto& r : results) {

            if (!r.success) {

                std::cout << RED << " ✗ " << RESET << r.name << "\n";
                std::cout << "   Message: " << r.message << "\n";

            }

        }

    }

    std::cout << BOLD << "===============================" << RESET << "\n\n";
}
