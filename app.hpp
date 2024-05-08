#pragma once
#include "pstream.h"
#include "save.hpp"
#include <filesystem>
#include <sdlpp/sdlpp.hpp>
#include <vector>

class App
{
public:
  App(sdl::Window &, int argc, const char *argv[]);
  ~App();
  auto tick() -> void;

  bool done = false;

private:
  struct TtsPack
  {
    enum class State { magicword, size, tokenId, wav };

    State state = State::magicword;
    int pos = 0;
    std::array<char, 4> magicword;
    int32_t size;
    std::array<char, 10> tokenId;
    std::vector<int16_t> wav;

    auto reset() -> void;
  };

  std::reference_wrapper<sdl::Window> window;
  SDL_GLContext gl_context;
  std::vector<std::filesystem::path> books;
  int selectedBook = -1;
  std::vector<std::string> tmpBooks2;
  std::vector<const char *> tmpBooks;
  std::string bookContent;
  float totalBookHeight = -1;
  bool isReading = false;
  int readingPos = 0;
  redi::pstream ttsPyProc;
  TtsPack ttsPack;
  SDL_AudioSpec want;
  SDL_AudioSpec have;
  sdl::Audio audio;
  std::vector<int16_t> wav;
  Uint32 cooldown = 0;
  Save save;
  bool needToUpdateScrollBar = false;

  auto loadBook(const std::filesystem::path &) -> void;
  auto processTts() -> void;
  auto scanBooks() -> void;
};
