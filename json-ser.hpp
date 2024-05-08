// (c) 2024 Mika Pi

#pragma once
#include <iostream>
#include <json/json.hpp>
#include <map>
#include <ser/is_serializable.hpp>
#include <sstream>

template <typename T>
constexpr auto jsonSer(std::ostream &st, T v, int lvl = 0) -> void;

template <typename T>
auto jsonDeser(const json::Val &jv, T &v) -> void;

namespace Internal
{
  template <typename T>
  struct IsVariant : std::false_type
  {
  };

  template <typename... Args>
  struct IsVariant<std::variant<Args...>> : std::true_type
  {
  };

  auto jsonEsc(std::ostream &, std::string) -> void;
  auto indent(std::ostream &, int lvl) -> void;

  auto jsonSerVal(std::ostream &st, std::string v, int /*lvl*/) -> void;

  template <typename T>
  auto jsonSerVal(std::ostream &st, T v, int /*lvl*/)
    -> std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>>
  {
    st << v;
  }

  template <typename T>
  auto jsonSerVal(std::ostream &st, std::vector<T> v, int lvl) -> void
  {
    st << "[\n";
    auto first = true;
    for (auto &&e : v)
    {
      if (first)
        first = false;
      else
        st << ",\n";
      indent(st, lvl + 1);
      jsonSer(st, std::move(e), lvl + 1);
    }
    st << "\n";
    indent(st, lvl);
    st << "]";
  }

  auto jsonSerVal(std::ostream &st, bool v, int /*lvl*/) -> void;

  template <typename... Ts>
  auto jsonSerVal(std::ostream &st, std::variant<Ts...> v, int lvl) -> void
  {
    std::visit([&](auto vv) { jsonSer(st, std::move(vv), lvl); }, v);
  }

  template <typename T>
  auto jsonSerVal(std::ostream &st, std::unordered_map<std::string, T> v, int lvl) -> void
  {
    st << "{\n";
    auto first = true;
    for (auto &&e : v)
    {
      if (first)
        first = false;
      else
        st << ",\n";
      indent(st, lvl + 1);
      jsonEsc(st, e.first);
      st << ": ";
      jsonSer(st, std::move(e.second), lvl + 1);
    }
    st << "\n";
    indent(st, lvl);
    st << "}";
  }

  template <typename T>
  auto jsonSerVal(std::ostream &st, std::map<std::string, T> v, int lvl) -> void
  {
    st << "{\n";
    auto first = true;
    for (auto &&e : v)
    {
      if (first)
        first = false;
      else
        st << ",\n";
      indent(st, lvl + 1);
      jsonEsc(st, e.first);
      st << ": ";
      jsonSer(st, std::move(e.second), lvl + 1);
    }
    st << "\n";
    indent(st, lvl);
    st << "}";
  }

  template <typename U, typename T>
  auto jsonSerVal(std::ostream &st, std::unordered_map<U, T> v, int lvl)
    -> std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>
  {
    st << "{\n";
    auto first = true;
    for (auto &&e : v)
    {
      if (first)
        first = false;
      else
        st << ",\n";
      indent(st, lvl + 1);
      st << "\"" << e.first << "\": ";
      jsonSer(st, std::move(e.second), lvl + 1);
    }
    st << "\n";
    indent(st, lvl);
    st << "}";
  }

  template <typename U, typename T>
  auto jsonSerVal(std::ostream &st, std::map<U, T> v, int lvl)
    -> std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>
  {
    st << "{\n";
    auto first = true;
    for (auto &&e : v)
    {
      if (first)
        first = false;
      else
        st << ",\n";
      indent(st, lvl + 1);
      st << "\"" << e.first << "\": ";
      jsonSer(st, std::move(e.second), lvl + 1);
    }
    st << "\n";
    indent(st, lvl);
    st << "}";
  }

  auto jsonDeserVal(const json::Val &j, std::string &v) -> void;

  template <typename T>
  auto jsonDeserVal(const json::Val &j, T &v)
    -> std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>>
  {
    if (!j.isNum())
      return;
    if constexpr (std::is_integral_v<T>)
    {
      if constexpr (std::is_unsigned_v<T>)
      {
        v = j.asUInt64();
        return;
      }

      v = j.asInt64();
      return;
    }
    if constexpr (std::is_enum_v<T>)
    {
      v = static_cast<T>(j.asUInt64());
      return;
    }
    v = j.asDouble();
  }

  template <typename T>
  auto jsonDeserVal(const json::Val &j, std::vector<T> &v) -> void
  {
    if (!j.isArr())
      return;
    const auto &arr = j.asArr();
    v.clear();
    for (const auto &e : arr)
      jsonDeser(e, v.emplace_back());
  }

  auto jsonDeserVal(const json::Val &j, bool &v) -> void;

  template <auto N = 0, typename... Ts>
  auto jsonDeserVal(const json::Val &j, size_t idx, std::variant<Ts...> &v) -> void
  {
    if constexpr (N >= sizeof...(Ts))
      return;
    else
    {
      if (idx == N)
        jsonDeser(j, v.template emplace<N>());
      else
        jsonDeserVal<N + 1, Ts...>(j, idx, v);
    }
  }

  template <typename T>
  auto jsonDeserVal(const json::Val &j, std::unordered_map<std::string, T> &v) -> void
  {
    if (!j.isObj())
      return;
    auto fields = j.getFields();
    for (const auto &f : fields)
    {
      auto tmp = v.emplace(f, T{});
      jsonDeser(j(f), tmp.first->second);
    }
  }

  template <typename T>
  auto jsonDeserVal(const json::Val &j, std::map<std::string, T> &v) -> void
  {
    if (!j.isObj())
      return;
    auto fields = j.getFields();
    for (const auto &f : fields)
    {
      auto tmp = v.emplace(f, T{});
      jsonDeser(j(f), tmp.first->second);
    }
  }

  template <typename U, typename T>
  auto jsonDeserVal(const json::Val &j, std::unordered_map<U, T> &v)
    -> std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>
  {
    if (!j.isObj())
      return;
    auto fields = j.getFields();
    for (const auto &f : fields)
    {
      auto key = U{};
      auto st = std::istringstream{std::string{f}};
      st >> key;
      auto tmp = v.emplace(key, T{});
      jsonDeser(j(f), tmp.first->second);
    }
  }

  template <typename U, typename T>
  auto jsonDeserVal(const json::Val &j, std::map<U, T> &v)
    -> std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>
  {
    if (!j.isObj())
      return;
    auto fields = j.getFields();
    for (const auto &f : fields)
    {
      auto key = U{};
      auto st = std::istringstream{std::string{f}};
      st >> key;
      auto tmp = v.emplace(key, T{});
      jsonDeser(j(f), tmp.first->second);
    }
  }

  // null
  // optional

  struct JsonArch
  {
    JsonArch(const json::Val &aJv) : jv(aJv) {}

    template <typename T>
    auto operator()(const char *name, T &vv) -> void
    {
      if constexpr (IsSerializableClassV<T>)
        jsonDeser(jv(name), vv);
      else if constexpr (Internal::IsVariant<T>::value)
        Internal::jsonDeserVal(jv(name), jv(name + std::string{"Type"}).asUInt64(), vv);
      else
        Internal::jsonDeserVal(jv(name), vv);
    }
    const json::Val &jv;
  };
} // namespace Internal

template <typename T>
constexpr auto jsonSer(std::ostream &st, T v, int lvl) -> void
{
  if constexpr (IsSerializableClassV<T>)
  {
    // object (JSON object)
    st << "{\n";
    auto first = true;
    auto l = [&](const char *name, auto vv) mutable {
      if constexpr (Internal::IsVariant<decltype(vv)>::value)
      {
        if (first)
          first = false;
        else
          st << ",\n";
        Internal::indent(st, lvl + 1);
        Internal::jsonEsc(st, name + std::string{"Type"});
        st << ": ";
        jsonSer(st, vv.index(), lvl + 1);
      }

      if (first)
        first = false;
      else
        st << ",\n";
      Internal::indent(st, lvl + 1);
      Internal::jsonEsc(st, name);
      st << ": ";
      jsonSer(st, std::move(vv), lvl + 1);
    };
    v.ser(l);
    st << "\n";
    Internal::indent(st, lvl);
    st << "}";
  }
  else
    Internal::jsonSerVal(st, std::move(v), lvl);
}

template <typename T>
auto jsonDeser(const json::Val &jv, T &v) -> void
{
  if constexpr (IsSerializableClassV<T>)
  {
    auto arch = Internal::JsonArch{jv};
    v.deser(arch);
  }
  else
    Internal::jsonDeserVal(jv, v);
}

template <typename T>
auto jsonDeser(std::istream &st, T &v) -> bool
{
  static_assert(IsSerializableClassV<T>);
  auto root = json::Root{st};
  if (root.empty())
    return false;

  jsonDeser(root.root(), v);
  return true;
}
