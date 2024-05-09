#pragma once
#include "pstream.h"
#include "save.hpp"
#include <deque>
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
  std::deque<int16_t> wav;
  std::deque<int16_t> speedAdjustedWav;
  double curPos = 0;
  double expectedPos = 0;
  Uint32 cooldown = 0;
  Save save;
  bool needToUpdateScrollBar = false;
  float desiredScroll = -1.f;
  bool justStarted = true;

  auto loadBook(const std::filesystem::path &) -> void;
  auto processTts() -> void;
  auto scanBooks() -> void;
  auto renderBook() -> void;
  auto getWavSample() -> int16_t;
  auto peekWavSample(int) -> int16_t;
  auto getSpeedAdjustedSample() -> int16_t;
};
