// ExpressionParser.h
#pragma once
#include <string>
#include <cmath>
#include <stdexcept>
#include <cctype>

class ExpressionParser {
public:
    static double evaluate(const std::string& expr, double x) {
        std::string normalized = normalizeExpression(expr);
        ExpressionParser parser(normalized, x);
        double result = parser.parseExpression();
        if (parser.pos < parser.input.size()) {
            return NAN; // unexpected trailing characters
        }
        return result;
    }

private:
    static constexpr double kPi = 3.14159265358979323846;
    static constexpr double kE = 2.71828182845904523536;

    static std::string trim(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
            start++;
        }

        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
            end--;
        }

        return s.substr(start, end - start);
    }

    static std::string normalizeExpression(const std::string& expr) {
        std::string normalized = trim(expr);

        // Allow assignment-like input from UI, e.g. "f(x)=x^2+1" or "y = sin(x)".
        size_t equalsPos = normalized.find('=');
        if (equalsPos != std::string::npos && equalsPos + 1 < normalized.size()) {
            normalized = trim(normalized.substr(equalsPos + 1));
        }

        return normalized;
    }

    std::string input;
    size_t pos;
    double xValue;

    ExpressionParser(const std::string& expr, double x) 
        : input(expr), pos(0), xValue(x) {}

    void skipWhitespace() {
        while (pos < input.size() && std::isspace(input[pos])) pos++;
    }

    char peek() {
        skipWhitespace();
        return pos < input.size() ? input[pos] : '\0';
    }

    char consume() {
        skipWhitespace();
        return pos < input.size() ? input[pos++] : '\0';
    }

    bool match(char c) {
        if (peek() == c) { consume(); return true; }
        return false;
    }

    // Grammar:
    // expression = term (('+' | '-') term)*
    // term       = power (('*' | '/') power)*
    // power      = unary ('^' power)?       (right-associative)
    // unary      = ('-' | '+') unary | call
    // call       = IDENT '(' expression ')' | primary
    // primary    = NUMBER | 'x' | 'pi' | 'e' | '(' expression ')'

    double parseExpression() {
        double left = parseTerm();
        while (true) {
            if (match('+'))      left = left + parseTerm();
            else if (match('-')) left = left - parseTerm();
            else break;
        }
        return left;
    }

    double parseTerm() {
        double left = parsePower();
        while (true) {
            if (match('*'))      left = left * parsePower();
            else if (match('/')) left = left / parsePower();
            else break;
        }
        return left;
    }

    double parsePower() {
        double base = parseUnary();
        if (match('^')) {
            double exp = parsePower(); // right-associative
            return std::pow(base, exp);
        }
        return base;
    }

    double parseUnary() {
        if (match('-')) return -parseUnary();
        if (match('+')) return parseUnary();
        return parseCall();
    }

    double parseCall() {
        skipWhitespace();
        
        // Try to read an identifier
        if (pos < input.size() && std::isalpha(input[pos])) {
            size_t start = pos;
            while (pos < input.size() && std::isalpha(input[pos])) pos++;
            std::string name = input.substr(start, pos - start);

            // Check if it's a function call (followed by '(')
            if (peek() == '(') {
                consume(); // '('
                double arg = parseExpression();
                if (!match(')')) return NAN; // missing closing paren

                if (name == "sin")   return std::sin(arg);
                if (name == "cos")   return std::cos(arg);
                if (name == "tan")   return std::tan(arg);
                if (name == "asin")  return std::asin(arg);
                if (name == "acos")  return std::acos(arg);
                if (name == "atan")  return std::atan(arg);
                if (name == "sinh")  return std::sinh(arg);
                if (name == "cosh")  return std::cosh(arg);
                if (name == "tanh")  return std::tanh(arg);
                if (name == "sqrt")  return std::sqrt(arg);
                if (name == "cbrt")  return std::cbrt(arg);
                if (name == "abs")   return std::abs(arg);
                if (name == "log")   return std::log(arg);
                if (name == "log2")  return std::log2(arg);
                if (name == "log10") return std::log10(arg);
                if (name == "ln")    return std::log(arg);
                if (name == "exp")   return std::exp(arg);
                if (name == "floor") return std::floor(arg);
                if (name == "ceil")  return std::ceil(arg);
                if (name == "round") return std::round(arg);
                return NAN; // unknown function
            }

            // It's a variable or constant
            if (name == "x")  return xValue;
            if (name == "pi") return kPi;
            if (name == "e")  return kE;
            
            // Single-char 'x' already handled; could be implicit multiply
            // For now, unknown variable
            return NAN;
        }

        return parsePrimary();
    }

    double parsePrimary() {
        // Number
        if (std::isdigit(peek()) || peek() == '.') {
            return parseNumber();
        }

        // Parenthesized expression
        if (match('(')) {
            double val = parseExpression();
            match(')'); // consume closing paren
            return val;
        }

        return NAN; // parse error
    }

    double parseNumber() {
        skipWhitespace();
        size_t start = pos;
        while (pos < input.size() && (std::isdigit(input[pos]) || input[pos] == '.'))
            pos++;
        
        try {
            return std::stod(input.substr(start, pos - start));
        } catch (...) {
            return NAN;
        }
    }
};