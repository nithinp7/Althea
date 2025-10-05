#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <xstring>

namespace AltheaEngine {

struct Parser {
  template <typename T>
  using RefResolver = std::function<std::optional<T>(std::string_view)>;

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
  std::optional<std::string_view> parseStringLiteral();

  template <typename T> std::optional<T> parseRef(RefResolver<T> resolver) {
    char* c0 = c;
    if (auto n = parseName()) {
      if (auto t = resolver(*n)) {
        return t;
      }
    }

    c = c0;
    return std::nullopt;
  }

  template <typename T> std::optional<T> parseLiteral() = delete;

  template <> std::optional<bool> parseLiteral() { return parseBool(); }
  template <> std::optional<uint32_t> parseLiteral() { return parseUint(); }
  template <> std::optional<int32_t> parseLiteral() { return parseInt(); }
  template <> std::optional<float> parseLiteral() { return parseFloat(); }

  template <typename T>
  std::optional<T> parseLiteralOrRef(RefResolver<T> resolver) {
    if (auto t = parseLiteral<T>()) {
      return t;
    } else if (auto n = parseName()) {
      return resolver(*n);
    }

    return std::nullopt;
  }

  template <class TEnum>
  std::optional<TEnum> parseToken(const char* const* strs, size_t numTokens)
  {
    if (auto n = parseName()) {
      for (size_t i = 0; i < numTokens; i++) {
        const char* tn = strs[i];
        size_t tnLen = strlen(tn);
        if (n->size() == tnLen &&
          !strncmp(n->data(), tn, n->size())) {
          return static_cast<TEnum>(i);
        }
      }
    }

    return std::nullopt;
  }
  
  static uint32_t getOperPrecedence(char op) {
    if (op == '+' || op == '-')
      return 0;
    if (op == '*' || op == '/')
      return 1;

    return ~0;
  }

  bool peakIsValidExprTerminator() const { return *c == 0 || *c == ')'; }

  std::optional<char> parseOp() {
    std::optional<char> op = parseChar('+');
    if (!op)
      op = parseChar('-');
    if (!op)
      op = parseChar('*');
    if (!op)
      op = parseChar('/');
    return op;
  }

  std::optional<char> peakOp() const {
    if (*c == '+' || *c == '-' || *c == '*' || *c == '/')
      return *c;
    return std::nullopt;
  }

  template <typename T>
  std::optional<T> parseExpression(RefResolver<T> resolver);

  template <typename T>
  std::optional<T> parseExpressionHelper(
      T lhs,
      uint32_t minPrecedence,
      RefResolver<T> resolver);

  template <typename T>
  std::optional<T>
  parseExpressionToken(RefResolver<T> resolver);
};

template <typename T>
std::optional<T>
Parser::parseExpressionToken(RefResolver<T> resolver) {
  char* c0 = c;

  parseWhitespace();

  if (parseChar('(')) {
    if (auto lhs = parseExpressionToken(resolver)) {
      auto res = parseExpressionHelper(*lhs, 0, resolver);
      if (parseChar(')')) {
        return res ? res : lhs;
      }
    }
  } else if (auto t = parseLiteral<T>()) {
    return t;
  } else if (resolver) {
    if (auto n = parseName())
      return resolver(*n);
  }

  c = c0;
  return std::nullopt;
}

template <typename T>
std::optional<T>
Parser::parseExpression(RefResolver<T> resolver) {
  char* c0 = c;
  if (auto lhs = parseExpressionToken(resolver)) {
    auto res = parseExpressionHelper(*lhs, 0, resolver);
    if (*c == 0) {
      return res ? res : lhs;
    }
  }
  c = c0;
  return std::nullopt;
}

template <typename T>
std::optional<T> Parser::parseExpressionHelper(
    T lhs,
    uint32_t minPrecedence,
    RefResolver<T> resolver) {
  char* c0 = c;
  parseWhitespace();
  auto lookahead = peakOp();
  while (lookahead && getOperPrecedence(*lookahead) >= minPrecedence) {
    auto op = parseOp();
    uint32_t opPrec = getOperPrecedence(*op);

    parseWhitespace();

    auto rhs = parseExpressionToken(resolver);
    if (!rhs) {
      c = c0;
      return std::nullopt; // cannot end expr on op
    }

    parseWhitespace();

    lookahead = peakOp();
    while (lookahead && getOperPrecedence(*lookahead) > opPrec && opPrec < 1) {
      rhs = parseExpressionHelper(*rhs, opPrec + 1, resolver);
      lookahead = peakOp();
    }

    switch (*op) {
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