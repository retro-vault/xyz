#ifndef XPROG_ERRORS_H
#define XPROG_ERRORS_H

#include <stdexcept>
#include <string>

namespace xprog {

class error : public std::runtime_error {
public:
    explicit error(const std::string& message) : std::runtime_error(message) {}
};

class usage_error : public error {
public:
    explicit usage_error(const std::string& message) : error(message) {}
};

} // namespace xprog

#endif
