// (c) 2024 Mika Pi

#include "json-ser.hpp"

namespace Internal
{
  auto jsonEsc(std::ostream &st, std::string v) -> void
  {
    st << "\"";
    for (auto c : v)
    {
      switch (c)
      {
      case '"': st << "\\\""; break;
      case '\\': st << "\\\\"; break;
      case '\b': st << "\\b"; break;
      case '\f': st << "\\f"; break;
      case '\n': st << "\\n"; break;
      case '\r': st << "\\r"; break;
      case '\t': st << "\\t"; break;
      default: st << c; break;
      }
    }
    st << "\"";
  }

  auto indent(std::ostream &st, int lvl) -> void
  {
    for (auto i = 0; i < lvl * 2; ++i)
      st << " ";
  }

  auto jsonSerVal(std::ostream &st, std::string v, int /*lvl*/) -> void
  {
    jsonEsc(st, std::move(v));
  }

  auto jsonSerVal(std::ostream &st, bool v, int /*lvl*/) -> void
  {
    st << (v ? "true" : "false");
  }

  auto jsonDeserVal(const json::Val &j, std::string &v) -> void
  {
    if (!j.isStr())
      return;
    v = j.asStr();
  }

  auto jsonDeserVal(const json::Val &j, bool &v) -> void
  {
    if (!j.isBool())
      return;
    v = j.asBool();
  }
} // namespace Internal
