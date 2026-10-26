// Exceptions for malformed or unreadable PLX streams.
#pragma once
#include <string>

namespace plasma {

class Exception {
public:
    explicit Exception(const std::string &message)
        : m_message(message)
    {}

    virtual const std::string &getMessage() const { return m_message; }

    std::string m_message;
};

class IOException : public Exception {
public:
    IOException(const std::string &message, const std::string &fileName)
        : Exception(message),
          m_fileName(fileName)
    {}

    std::string m_fileName;
};

// Three specialised PLX exceptions.
class InvalidFileFormatException : public IOException {
public:
    explicit InvalidFileFormatException(const std::string &fileName)
        : IOException(fileName + " has invalid format.", fileName)
    {}
};

class InvalidVersionException : public IOException {
public:
    explicit InvalidVersionException(const std::string &fileName)
        : IOException(fileName + " has invalid version.", fileName)
    {}
};

class InvalidDemoLicenseException : public IOException {
public:
    explicit InvalidDemoLicenseException(const std::string &fileName)
        : IOException(fileName + " was created with a different demo license.", fileName)
    {}
};

}
