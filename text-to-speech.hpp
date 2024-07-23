#pragma once
#include <RHVoice.h>
#include <string>
#include <vector>

class TextToSpeech
{
public:
  TextToSpeech();
  ~TextToSpeech();
  auto operator()(const std::string &msg, float rate) const -> std::pair<int, std::vector<int16_t>>;

private:
  RHVoice_tts_engine engine;
  static auto setSampleRate(int sample_rate, void *user_data) -> int;
  static auto playSpeech(const short *samples, unsigned int count, void *user_data) -> int;
};
