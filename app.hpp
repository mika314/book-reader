#pragma once
#include <sdlpp/sdlpp.hpp>

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
};
