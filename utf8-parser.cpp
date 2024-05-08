#include "utf8-parser.hpp"

Utf8Parser::Utf8Parser(std::string s) : ss(std::move(s)) {}

auto Utf8Parser::getCh() -> std::string
{
  auto r = std::string{};
  while (ss)
  {
    char c;
    ss.read(&c, 1);
    if ((c & 0x80) == 0)
    {
      if (r.empty())
      {
        r += c;
        break;
      }
      ss.putback(c);
      break;
    }
    r += c;
  }
  return r;
}
