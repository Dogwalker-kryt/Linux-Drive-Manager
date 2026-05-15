#pragma once

#include <array>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <utility>
#include <cstdio> // for snprintf

/**
 * @class FixedString
 * @brief A fixed-size, stack-allocated string class.
 * @tparam N Maximum length of the string (excluding null terminator).
 *
 * FixedString is a lightweight, non-allocating string class for use in
 * performance-critical or embedded contexts. It does not depend on std::string.
 */
template<size_t N>
class FixedString {
    std::array<char, N + 1> buffer_ = {};
    size_t len = 0;

public:
    using iterator = char*;
    using const_iterator = const char*;
    static constexpr size_t npos = static_cast<size_t>(-1);

    // --- Constructors ---
    FixedString() = default;
    FixedString(const FixedString&) = default;
    FixedString& operator=(const FixedString&) = default;

    FixedString(const char* str) {
        if (str) {
            size_t n = std::strlen(str);
            if (n > N) throw std::length_error("String too long");
            std::strcpy(buffer_.data(), str);
            len = n;
        }
    }

    FixedString(size_t count, char c) {
        if (count >= N) throw std::length_error("String too long");
        std::fill_n(buffer_.data(), count, c);
        buffer_[count] = '\0';
        len = count;
    }

    template<size_t M>
    FixedString(const FixedString<M>& other) {
        if (other.size() > N) throw std::length_error("String too long");
        std::copy(other.begin(), other.end(), buffer_.data());
        buffer_[other.size()] = '\0';
        len = other.size();
    }

    // --- Assignment ---
    FixedString& operator=(const char* str) {
        if (str) {
            size_t n = std::strlen(str);
            if (n > N) throw std::length_error("String too long");
            std::strcpy(buffer_.data(), str);
            len = n;
        } else {
            clear();
        }
        return *this;
    }

    FixedString& operator=(char c) {
        buffer_[0] = c;
        buffer_[1] = '\0';
        len = 1;
        return *this;
    }

    template<size_t M>
    FixedString& operator=(const FixedString<M>& other) {
        if (other.size() > N) throw std::length_error("String too long");
        std::copy(other.begin(), other.end(), buffer_.data());
        buffer_[other.size()] = '\0';
        len = other.size();
        return *this;
    }

    // --- Element access ---
    char& operator[](size_t pos) { return buffer_[pos]; }
    const char& operator[](size_t pos) const { return buffer_[pos]; }
    char& at(size_t pos) {
        if (pos >= len) throw std::out_of_range("FixedString::at");
        return buffer_[pos];
    }
    const char& at(size_t pos) const {
        if (pos >= len) throw std::out_of_range("FixedString::at");
        return buffer_[pos];
    }
    char& front() { return buffer_[0]; }
    const char& front() const { return buffer_[0]; }
    char& back() { return buffer_[len - 1]; }
    const char& back() const { return buffer_[len - 1]; }
    const char* c_str() const { return buffer_.data(); }
    const char* data() { return buffer_.data(); }

    // --- Iterators ---
    iterator begin() { return buffer_.data(); }
    const_iterator begin() const { return buffer_.data(); }
    iterator end() { return buffer_.data() + len; }
    const_iterator end() const { return buffer_.data() + len; }

    // --- Capacity ---
    size_t size() const { return len; }
    size_t length() const { return len; }
    size_t max_size() const { return N; }
    size_t capacity() const { return N; }
    bool empty() const { return len == 0; }

    // --- Modifiers ---
    void clear() { buffer_[0] = '\0'; len = 0; }

    FixedString& append(const char* str, size_t count) {
        if (len + count > N) throw std::length_error("String too long");
        std::copy(str, str + count, buffer_.data() + len);
        len += count;
        buffer_[len] = '\0';
        return *this;
    }
    FixedString& append(const char* str) {
        if (str) {
            size_t n = std::strlen(str);
            if (len + n >= N) throw std::length_error("String too long");
            std::copy(str, str + n, buffer_.data() + len);
            len += n;
            buffer_[len] = '\0';
        }
        return *this;
    }
    FixedString& append(size_t count, char c) {
        if (len + count > N) throw std::length_error("String too long");
        std::fill_n(buffer_.data() + len, count, c);
        len += count;
        buffer_[len] = '\0';
        return *this;
    }
    FixedString& append(const FixedString& other) {
        return append(other.c_str(), other.size());
    }

    FixedString& operator+=(const FixedString& other) { return append(other); }
    FixedString& operator+=(const char* str) { return append(str); }
    FixedString& operator+=(char c) { push_back(c); return *this; }

    void push_back(char c) {
        if (len + 1 >= N) throw std::length_error("String too long");
        buffer_[len++] = c;
        buffer_[len] = '\0';
    }
    void pop_back() {
        if (len == 0) throw std::out_of_range("FixedString::pop_back");
        --len;
        buffer_[len] = '\0';
    }

    // --- Comparison ---
    int compare(const FixedString& other) const {
        size_t n = std::min(len, other.len);
        int cmp = std::memcmp(buffer_.data(), other.buffer_.data(), n);
        if (cmp != 0) return cmp;
        if (len < other.len) return -1;
        if (len > other.len) return 1;
        return 0;
    }
    int compare(const char* str) const {
        return compare(FixedString<N>(str));
    }

    bool operator==(const FixedString& other) const { return compare(other) == 0; }
    bool operator!=(const FixedString& other) const { return compare(other) != 0; }
    bool operator<(const FixedString& other) const { return compare(other) < 0; }
    bool operator>(const FixedString& other) const { return compare(other) > 0; }
    bool operator<=(const FixedString& other) const { return compare(other) <= 0; }
    bool operator>=(const FixedString& other) const { return compare(other) >= 0; }

    bool operator==(const char* str) const { return compare(str) == 0; }
    bool operator!=(const char* str) const { return compare(str) != 0; }

    // --- Utility ---
    void swap(FixedString& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(len, other.len);
    }

    // --- Substring ---
    FixedString substr(size_t pos = 0, size_t count = npos) const {
        if (pos > len) throw std::out_of_range("FixedString::substr");
        count = std::min(count, len - pos);
        FixedString result;
        std::copy(buffer_.data() + pos, buffer_.data() + pos + count, result.buffer_.data());
        result.buffer_[count] = '\0';
        result.len = count;
        return result;
    }

    // --- Search ---
    size_t find(const char* str, size_t pos = 0) const {
        size_t n = std::strlen(str);
        if (pos > len || n == 0) return npos;
        for (size_t i = pos; i <= len - n; ++i) {
            if (std::memcmp(buffer_.data() + i, str, n) == 0) return i;
        }
        return npos;
    }
    size_t find(char c, size_t pos = 0) const {
        for (size_t i = pos; i < len; ++i) {
            if (buffer_[i] == c) return i;
        }
        return npos;
    }

    // --- C++17 string_view compatibility ---
    operator std::string_view() const { return std::string_view(buffer_.data(), len); }
};

// --- Non-member functions ---
template<size_t N, size_t M>
FixedString<N + M> operator+(const FixedString<N>& lhs, const FixedString<M>& rhs) {
    FixedString<N + M> result;
    result.append(lhs);
    result.append(rhs);
    return result;
}

template<size_t N, size_t L>
FixedString<N + (L - 1)> operator+(const FixedString<N>& lhs, const char(&rhs)[L]) {
    FixedString<N + (L - 1)> result;
    result.append(lhs);
    result.append(rhs);
    return result;
}

template<size_t L, size_t N>
FixedString<(L - 1) + N> operator+(const char(&lhs)[L], const FixedString<N>& rhs) {
    FixedString<(L - 1) + N> result;
    result.append(lhs);
    result.append(rhs);
    return result;
}

template<size_t N>
FixedString<N + 1> operator+(const FixedString<N>& lhs, char rhs) {
    FixedString<N + 1> result;
    result.append(lhs);
    result.push_back(rhs);
    return result;
}

template<size_t N>
FixedString<1 + N> operator+(char lhs, const FixedString<N>& rhs) {
    FixedString<1 + N> result;
    result.push_back(lhs);
    result.append(rhs);
    return result;
}

// --- Hash support ---
namespace std {
    template<size_t N>
    struct hash<FixedString<N>> {
        size_t operator()(const FixedString<N>& s) const {
            return std::hash<std::string_view>{}(std::string_view(s.c_str(), s.size()));
        }
    };
}

namespace scf {
    // --- Convenient aliases ---
    using str8 = FixedString<8>;
    using str16 = FixedString<16>;
    using str32 = FixedString<32>;
    using str64 = FixedString<64>;
    using str128 = FixedString<128>;
    using str256 = FixedString<256>;
    using str512 = FixedString<512>;
    using str1024 = FixedString<1024>;
    using str2048 = FixedString<2048>;
    using str4096 = FixedString<4096>;

    /**
     * @brief Converts a value to a FixedString of size N.
     * @tparam N Size of the FixedString.
     * @tparam T Type of the value to convert.
     * @param value The value to convert.
     * @return FixedString<N> containing the string representation of value.
     *
     * @note If the string representation of value is longer than N,
     *       the result will be truncated and null-terminated.
     */
    template<size_t N, typename T>
    FixedString<N> to_scf_str(const T& value) {
        FixedString<N> out;
        int written = std::snprintf(out.data(), N + 1,
            // Select format specifier based on type
            std::is_same_v<T, int> ? "%d" :
            std::is_same_v<T, unsigned int> ? "%u" :
            std::is_same_v<T, long long> ? "%lld" :
            std::is_same_v<T, unsigned long long> ? "%llu" :
            std::is_same_v<T, float> ? "%f" :
            std::is_same_v<T, double> ? "%lf" :
            std::is_same_v<T, bool> ? "%s" :
            std::is_same_v<T, const char*> ? "%s" :
            "%s"
            , value);
        if (written < 0) {
            out.clear();
        } else {
            out.length() = static_cast<size_t>(written);
        }
        return out;
    }

    // Specialization for bool to print "true"/"false"
    template<size_t N>
    FixedString<N> to_scf_str(const bool& value) {
        FixedString<N> out;
        int written = std::snprintf(out.data(), N + 1, "%s", value ? "true" : "false");
        if (written < 0) {
            out.clear();
        } else {
            out.length() = static_cast<size_t>(written);
        }
        return out;
    }

    // Specialization for const char* to avoid format specifier issues
    template<size_t N>
    FixedString<N> to_scf_str(const char* value) {
        FixedString<N> out;
        if (value) {
            size_t n = std::strlen(value);
            if (n > N) {
                std::copy(value, value + N, out.data());
                out.buffer_[N] = '\0';
                out.length() = N;
            } else {
                std::strcpy(out.data(), value);
                out.length() = n;
            }
        }
        return out;
    }

    // --- Convenience to_strX aliases ---
    template<typename T> str8    to_str8(const T& v)    { return to_scf_str<8>(v); }
    template<typename T> str16   to_str16(const T& v)   { return to_scf_str<16>(v); }
    template<typename T> str32   to_str32(const T& v)   { return to_scf_str<32>(v); }
    template<typename T> str64   to_str64(const T& v)   { return to_scf_str<64>(v); }
    template<typename T> str128  to_str128(const T& v)  { return to_scf_str<128>(v); }
    template<typename T> str256  to_str256(const T& v)  { return to_scf_str<256>(v); }
    template<typename T> str512  to_str512(const T& v)  { return to_scf_str<512>(v); }
    template<typename T> str1024 to_str1024(const T& v) { return to_scf_str<1024>(v); }
    template<typename T> str2048 to_str2048(const T& v) { return to_scf_str<2048>(v); }
    template<typename T> str4096 to_str4096(const T& v) { return to_scf_str<4096>(v); }

    /**
     * @brief Converts a value to a FixedString with automatic size selection.
     * @tparam T Type of the value to convert.
     * @param value The value to convert.
     * @return FixedString<256> containing the string representation of value.
     *
     * @note This is a convenience function that uses a default size of 256.
     *       For most use cases, this is sufficient. For larger strings,
     *       use to_str256, to_str512, etc. explicitly.
     */
    template<typename T>
    FixedString<256> to_str(const T& value) {
        return to_scf_str<256>(value);
    }
}