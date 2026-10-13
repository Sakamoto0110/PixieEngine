#pragma once

#include <charconv>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

// How a primitive becomes text: its fields in order, in parentheses, separated by ", ", as in
// "(1.5, -2)" (docs/layout.md, 3). Numbers go through std::to_chars, which ignores the locale, so the
// decimal point is always a point. A floating point number is written with the fewest significant
// digits that read back as the same value, completed with zeros and never in scientific notation:
// 100000 stays 100000, and the float 1e20 is 100000000000000000000, the same text as in C#. (The fixed
// notation of std::to_chars would write 100000002004087734272, the digits closest to the exact value.)
// NaN is always nan, whatever its sign bit; the infinities are inf and -inf.

// Moves the decimal point of "1.5e+20" or "5e-324" to where the exponent says, with zeros.
inline void pxAppendWithoutExponent(std::string& out, std::string_view text) {
    std::size_t e = text.find('e');
    if (e == std::string_view::npos) {
        out += text;
        return;
    }

    bool negative = text[0] == '-';
    std::string_view mantissa = text.substr(negative ? 1 : 0, e - (negative ? 1 : 0));
    std::string_view exponentText = text.substr(e + 1);
    bool negativeExponent = exponentText[0] == '-';
    if (exponentText[0] == '-' || exponentText[0] == '+')
        exponentText.remove_prefix(1);
    int exponent = 0;
    std::from_chars(exponentText.data(), exponentText.data() + exponentText.size(), exponent);
    if (negativeExponent)
        exponent = -exponent;

    std::size_t point = mantissa.find('.');
    std::string digits(mantissa);
    if (point != std::string_view::npos)
        digits.erase(point, 1);
    std::ptrdiff_t integerDigits =
        static_cast<std::ptrdiff_t>(point == std::string_view::npos ? mantissa.size() : point) + exponent;
    std::ptrdiff_t digitCount = static_cast<std::ptrdiff_t>(digits.size());

    if (negative)
        out += '-';
    if (integerDigits <= 0) {
        out += "0.";
        out.append(static_cast<std::size_t>(-integerDigits), '0');
        out += digits;
    } else if (integerDigits >= digitCount) {
        out += digits;
        out.append(static_cast<std::size_t>(integerDigits - digitCount), '0');
    } else {
        out.append(digits, 0, static_cast<std::size_t>(integerDigits));
        out += '.';
        out.append(digits, static_cast<std::size_t>(integerDigits));
    }
}

template<typename T>
void pxAppendNumber(std::string& out, T value) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value)) {
            out += "nan";
            return;
        }
        // The shortest scientific form is at most 24 characters: -2.2250738585072014e-308.
        char buffer[32];
        auto result = std::to_chars(buffer, buffer + sizeof buffer, value, std::chars_format::scientific);
        pxAppendWithoutExponent(out, std::string_view(buffer, static_cast<std::size_t>(result.ptr - buffer)));
    } else {
        char buffer[16];
        out.append(buffer, std::to_chars(buffer, buffer + sizeof buffer, value).ptr);
    }
}

template<typename... Ts>
std::string pxTupleString(Ts... values) {
    std::string out = "(";
    bool first = true;
    ((out += first ? "" : ", ", first = false, pxAppendNumber(out, values)), ...);
    out += ')';
    return out;
}
