#pragma once
#include <map>
#include <ser/macro.hpp>
#include <string>

struct Book
{
  SER_PROPS(readingPos);
  int readingPos = 0;
};

struct Save
{
  SER_PROPS(books, selectedBook, readingSpeed);
  std::map<std::string, Book> books;
  std::string selectedBook;
  float readingSpeed = 1.f;
};
