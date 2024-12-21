#pragma once

#include <cstdint>
#include <optional>
#include <type_traits>
#include <xstring>

namespace AltheaEngine {
struct Parser {
  char* c;

  std::optional<char> parseChar(char ref);
  std::optional<std::string_view> parseWhitespace();
  std::optional<char> parseLetter();
  std::optional<uint32_t> parseDigit();
  std::optional<std::string_view> parseName();
  std::optional<bool> parseBool();
  std::optional<uint32_t> parseUint();
  std::optional<int32_t> parseInt();
  std::optional<float> parseFloat();

  template <typename T> std::optional<T> parseLiteral() = delete;

  template <> std::optional<bool> parseLiteral() { return parseBool(); }
  template <> std::optional<uint32_t> parseLiteral() { return parseUint(); }
  template <> std::optional<int32_t> parseLiteral() { return parseInt(); }
  template <> std::optional<float> parseLiteral() { return parseFloat(); }

  std::optional<uint32_t> parseUintOrVar(); // TODO

  static uint32_t getOperPrecedence(char op) {
    if (op == '+' || op == '-')
      return 0;
    if (op == '*' || op == '/')
      return 1;

    return ~0;
  }

  template <typename T>
  std::optional<T> parseExpression(std::optional<T> resolver(std::string_view));

  template <typename T>
  std::optional<T> parseExpressionHelper(
      T lhs,
      uint32_t maxPrecedence,
      std::optional<T> resolver(std::string_view));

  template <typename T>
  std::optional<T>
  parseExpressionToken(std::optional<T> resolver(std::string_view));
};

template <typename T>
std::optional<T>
Parser::parseExpressionToken(std::optional<T> resolver(std::string_view)) {
  parseWhitespace();

  if (parseChar('(')) {
    return parseExpression<T>(resolver);
  } else if (parseChar(')')) {
    return std::nullopt;
  } else if (auto t = parseLiteral<T>()) {
    return t;
  } else if (resolver) {
    if (auto n = parseName())
      return resolver(*n);
  }

  return std::nullopt;
}

template <typename T>
std::optional<T>
Parser::parseExpression(std::optional<T> resolver(std::string_view)) {
  char* c0 = c;
  auto lhs = parseExpressionToken(resolver);
  if (lhs) {
    if (auto res = parseExpressionHelper(*lhs, 0, resolver))
      return res;
  }
  c = c0;
  return std::nullopt;
}

template <typename T>
std::optional<T> Parser::parseExpressionHelper(
    T lhs,
    uint32_t maxPrecedence,
    std::optional<T> resolver(std::string_view)) {
  uint32_t curPrecedence = maxPrecedence;
  while (curPrecedence >= maxPrecedence) {
    parseWhitespace();

    if (*c == 0)
      return lhs;

    char op = *c;
    c++;

    parseWhitespace();

    auto rhs = parseExpressionToken(resolver);
    if (!rhs)
      return lhs;

    parseWhitespace();
    char lookahead = *c;

    while (getOperPrecedence(lookahead) > getOperPrecedence(op)) {
      rhs = parseExpressionHelper(*rhs, getOperPrecedence(op) + 1, resolver);
      lookahead = *c;
    }

    switch (op) {
    case '+':
      lhs = lhs + *rhs;
      break;
    case '-':
      lhs = lhs - *rhs;
      break;
    case '*':
      lhs = lhs * *rhs;
      break;
    case '/':
      lhs = lhs / *rhs;
      break;
    }
  }

  return lhs;
}
} // namespace AltheaEngine