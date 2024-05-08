#pragma once
#include <sstream>

class Utf8Parser
{
public:
  Utf8Parser(std::string);

  auto getCh() -> std::string;

private:
  std::istringstream ss;
};
