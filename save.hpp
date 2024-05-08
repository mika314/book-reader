#pragma once
#include <ser/macro.hpp>
#include <string>
#include <unordered_map>

struct Book
{
  SER_PROPS(readingPos);
  int readingPos = 0;
};

struct Save
{
  SER_PROPS(books, selectedBook);
  std::unordered_map<std::string, Book> books;
  std::string selectedBook;
};
