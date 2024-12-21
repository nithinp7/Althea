#include "Parser.h"

namespace AltheaEngine {

std::optional<char> Parser::parseChar(char ref) {
  if (*c == ref) {
    char cr = *c;
    c++;
    return cr;
  }
  return std::nullopt;
}

std::optional<std::string_view> Parser::parseWhitespace() {
  char* c0 = c;
  while (parseChar(' '))
    ;
  return c != c0 ? std::make_optional<std::string_view>(c0, c - c0)
                 : std::nullopt;
}

std::optional<char> Parser::parseLetter() {
  if ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z')) {
    char l = *c;
    c++;
    return l;
  }
  return std::nullopt;
}

std::optional<uint32_t> Parser::parseDigit() {
  if (*c >= '0' && *c <= '9') {
    uint32_t d = *c - '0';
    c++;
    return d;
  }
  return std::nullopt;
}

std::optional<std::string_view> Parser::parseName() {
  char* c0 = c;
  if (!parseChar('_') && !parseLetter())
    return std::nullopt;
  while (parseChar('_') || parseLetter() || parseDigit())
    ;
  return std::string_view(c0, c - c0);
}

std::optional<bool> Parser::parseBool() {
  char* c0 = c;
  auto word = parseName();
  if (!word)
    return std::nullopt;

  if (word->size() == 4 && !strncmp(word->data(), "true", 4))
    return true;
  else if (word->size() == 5 && !strncmp(word->data(), "false", 5))
    return false;

  c = c0;
  return std::nullopt;
}

std::optional<uint32_t> Parser::parseUint() {
  auto d = parseDigit();
  if (!d)
    return std::nullopt;
  uint32_t u = *d;
  while (d = parseDigit())
    u = 10 * u + *d;
  return u;
}

std::optional<int32_t> Parser::parseInt() {
  char* c0 = c;
  int sn = parseChar('-') ? -1 : 1;
  if (auto u = parseUint())
    return sn * *u;
  c = c0;
  return std::nullopt;
}

std::optional<float> Parser::parseFloat() {
  char* c0 = c;
  if (!parseInt())
    return std::nullopt;
  parseChar('.');
  parseUint();
  char* c1 = c;
  parseChar('f');
  parseChar('F');
  return static_cast<float>(std::atof(c0));
}
} // namespace AltheaEngine