#pragma once
#include "save.hpp"
#include "text-to-speech.hpp"
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
  SDL_AudioSpec want;
  SDL_AudioSpec have;
  sdl::Audio audio;
  std::deque<int16_t> wav;
  double curPos = 0;
  double expectedPos = 0;
  Uint32 cooldown = 0;
  Save save;
  bool needToUpdateScrollBar = false;
  float desiredScroll = -1.f;
  bool needResetScroll = true;
  float lastScrollDelta = 0.f;
  bool searchActive = false;
  std::string searchString;
  int lastSelectedItem = -1;
  TextToSpeech tts;

  auto getSpeedAdjustedSample() -> int16_t;
  auto getWavSample() -> int16_t;
  auto loadBook(const std::filesystem::path &) -> void;
  auto peekWavSample(int) -> int16_t;
  auto processSearch() -> void;
  auto processTts() -> void;
  auto renderBook() -> void;
  auto scanBooks() -> void;
};
