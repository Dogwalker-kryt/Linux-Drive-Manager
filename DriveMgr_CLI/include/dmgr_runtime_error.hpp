#pragma once

#include "TermiosIO.h"

/**
 * @brief A helper function to throw a std::runtime_error with a given error message and handles the terminal state poperly
 * @param msg The error message to include in the exception.
 */
void dmgr_runtime_error(const std::string &msg);