// Shim mínimo de Arduino para compilar el motor de ajedrez en el PC (tests nativos).
#pragma once
#include <string>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>

typedef bool boolean;
#define F(x) x
inline unsigned long millis() {
    using namespace std::chrono;
    return (unsigned long)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
inline void delay(unsigned long ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
inline void delayMicroseconds(unsigned) {}

class String {
public:
    std::string s;
    String() {}
    String(const char *c) : s(c ? c : "") {}
    String(char c) : s(1, c) {}
    String(int v) : s(std::to_string(v)) {}
    String(unsigned v) : s(std::to_string(v)) {}
    String(long v) : s(std::to_string(v)) {}
    String(unsigned long v) : s(std::to_string(v)) {}
    String(double v, int dec = 2) { char b[40]; snprintf(b, sizeof b, "%.*f", dec, v); s = b; }
    unsigned length() const { return (unsigned)s.size(); }
    const char *c_str() const { return s.c_str(); }
    char charAt(unsigned i) const { return i < s.size() ? s[i] : 0; }
    char operator[](unsigned i) const { return charAt(i); }
    int indexOf(const String &x, unsigned from = 0) const { auto p = s.find(x.s, from); return p == std::string::npos ? -1 : (int)p; }
    int indexOf(char c, unsigned from = 0) const { auto p = s.find(c, from); return p == std::string::npos ? -1 : (int)p; }
    String substring(unsigned a) const { return String(a <= s.size() ? s.substr(a).c_str() : ""); }
    String substring(unsigned a, unsigned b) const { return String(a <= s.size() && b >= a ? s.substr(a, b - a).c_str() : ""); }
    int toInt() const { return atoi(s.c_str()); }
    void trim() { size_t a = s.find_first_not_of(" \t\r\n"); size_t b = s.find_last_not_of(" \t\r\n"); s = a == std::string::npos ? "" : s.substr(a, b - a + 1); }
    void toUpperCase() { for (auto &c : s) c = (char)toupper((unsigned char)c); }
    bool operator==(const String &o) const { return s == o.s; }
    bool operator==(const char *o) const { return s == o; }
    bool operator!=(const char *o) const { return s != o; }
    String &operator+=(const String &o) { s += o.s; return *this; }
};
inline String operator+(const String &a, const String &b) { String r(a); r.s += b.s; return r; }
inline String operator+(const char *a, const String &b) { return String(a) + b; }
inline String operator+(const String &a, const char *b) { return a + String(b); }
inline String operator+(char a, const String &b) { return String(a) + b; }
inline String operator+(const String &a, char b) { return a + String(b); }
#define B1111 15
