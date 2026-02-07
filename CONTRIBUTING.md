# Contributing to Drive-Management-Utility-for-Linux

Thanks for your interest in helping out! This guide explains how to report bugs, request features, and contribute code to this C++ project.

---

## Reporting Bugs

Please open an issue with the following details:

- A clear description of the bug
- What you expected to happen
- How to reproduce it (step by step)
- Screenshots or a screen recording (optional, but helpful)
- Whether you changed any code before the issue happened
- What function(s) you were trying to use

---

## Requesting Features

When opening a feature request, please include:

- A description of the new feature
- Why it's useful or needed
- Examples or references (if available)
- Let us know if you want help writing example code for it

---

## Contributing Code

To contribute code:

1. Fork the repository
2. Create a new branch for your changes
3. Follow the coding style and guidelines (see below)
4. Build and test your code
5. Open a Pull Request (PR)
6. In the PR description, explain **exactly what you changed**
   

---

## Coding Style (C++)

- Use **4 spaces or 1 tab** for indentation (just stay consistent)
- Use **clear, descriptive names** for variables and functions (e.g if for a new funtion then write: newfunction_input_cmd)
  
     for Variables: snake_case
  
     for functions: camelCase
  
     for Classes/structs: PascalCase
  
     for Contants/macros: UPPER_SNAKE
  
- Comment your code **only when necessary** — keep comments helpful and to the point, dont over use then (e.g. 10line of comments for a single line of code)
- Use C++ 17 or later 
- Try to write self explaining, readable code
  this is good:
  ```cpp
   if (devZero.find("error") != std::string::npos && devrandom.find("error") != std::string::npos) {
                Logger::log("[ERROR] failed to overwrite drive data\n");
                throw std::runtime_error("[Error] Failed to Overwrite drive data");

  }
  ```

  this is bad:
  ```cpp
   #define X if
   #define N std::string::npos
   #define Z Logger::log
   #define R std::runtime_error
   #define Q throw
   
   X((devZero.find("\x65\x72\x72\x6f\x72")!=N) && (devrandom.find("\x65\x72\x72\x6f\x72")!=N)){
   Z("\x5b\x45\x52\x52\x4f\x52\x5d\x20\x66\x61\x69\x6c\x65\x64\x20\x74\x6f\x20\x6f\x76\x65\x72\x77\x72\x69\x74\x65\x20\x64\x72\x69\x76\x65\x20\x64\x61\x74\x61\x0a");
   Q R("\x5b\x45\x72\x72\x6f\x72\x5d\x20\x46\x61\x69\x6c\x65\x64\x20\x74\x6f\x20\x4f\x76\x65\x72\x77\x72\x69\x74\x65\x20\x64\x72\x69\x76\x65\x20\x64\x61\x74\x61");
   }
  ```

But, if there is no other way to write somthing, then its ok

other things to avoid:
- nesting (multiple if statments in one)
- to much global variables that dont have an clear owner
- to much raw pointers
