/*
 * Minimal Arduino.h shim for native unit tests
 * Provides just enough of the Arduino String API for header-only
 * logic code (RocRailParser) to compile and run on the host machine.
 */

#pragma once

#include <cstdint>
#include <string>

class String {
public:
    String() = default;
    String(const char* s) : m_data(s ? s : "") {}
    String(const std::string& s) : m_data(s) {}

    unsigned int length() const {
        return static_cast<unsigned int>(m_data.length());
    }

    const char* c_str() const { return m_data.c_str(); }

    bool startsWith(const String& prefix) const {
        return m_data.rfind(prefix.m_data, 0) == 0;
    }

    bool endsWith(const String& suffix) const {
        if (suffix.m_data.length() > m_data.length()) return false;
        return m_data.compare(m_data.length() - suffix.m_data.length(),
                              suffix.m_data.length(), suffix.m_data) == 0;
    }

    int indexOf(const String& sub, unsigned int from = 0) const {
        if (from > m_data.length()) return -1;
        const std::size_t pos = m_data.find(sub.m_data, from);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }

    int indexOf(char c, unsigned int from = 0) const {
        if (from > m_data.length()) return -1;
        const std::size_t pos = m_data.find(c, from);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }

    String substring(unsigned int begin) const {
        if (begin >= m_data.length()) return String();
        return String(m_data.substr(begin));
    }

    String substring(unsigned int begin, unsigned int end) const {
        if (begin >= m_data.length() || end <= begin) return String();
        return String(m_data.substr(begin, end - begin));
    }

    String operator+(const String& rhs) const {
        return String(m_data + rhs.m_data);
    }

    String& operator+=(const String& rhs) {
        m_data += rhs.m_data;
        return *this;
    }

    bool operator==(const String& rhs) const { return m_data == rhs.m_data; }
    bool operator==(const char* rhs) const { return m_data == (rhs ? rhs : ""); }
    bool operator!=(const String& rhs) const { return !(*this == rhs); }
    bool operator!=(const char* rhs) const { return !(*this == rhs); }

private:
    std::string m_data;
};

inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}
