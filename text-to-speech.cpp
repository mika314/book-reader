#include "text-to-speech.hpp"
#include <cstring>
#include <log/log.hpp>
#include <sherpa-onnx/c-api/c-api.h>

TextToSpeech::TextToSpeech()
{
  SherpaOnnxOfflineTtsConfig config;
  memset(&config, 0, sizeof(config));
  config.model.kokoro.model = "./kokoro-en-v0_19/model.onnx";
  config.model.kokoro.voices = "./kokoro-en-v0_19/voices.bin";
  config.model.kokoro.tokens = "./kokoro-en-v0_19/tokens.txt";
  config.model.kokoro.data_dir = "./kokoro-en-v0_19/espeak-ng-data";
  config.model.num_threads = 2;
  config.model.debug = 0;
  config.model.provider = "cuda";

  tts = SherpaOnnxCreateOfflineTts(&config);
}

TextToSpeech::~TextToSpeech()
{
  SherpaOnnxDestroyOfflineTts(tts);
}

auto TextToSpeech::operator()(const std::string &text, float rate, int voice) const
  -> std::pair<int, std::vector<int16_t>>
{
  // mapping of sid to voice name
  // 0->af, 1->af_bella, 2->af_nicole, 3->af_sarah, 4->af_sky, 5->am_adam
  // 6->am_michael, 7->bf_emma, 8->bf_isabella, 9->bm_george, 10->bm_lewis
  int32_t sid = voice;
  float speed = rate; // larger -> faster in speech speed
  // If you don't want to use a callback, then please enable this branch
  const SherpaOnnxGeneratedAudio *audio = SherpaOnnxOfflineTtsGenerate(tts, text.c_str(), sid, speed);

  auto ret = std::pair<int, std::vector<int16_t>>{};
  ret.first = audio->sample_rate;
  for (auto it = audio->samples; it != audio->samples + audio->n; ++it)
    ret.second.push_back(static_cast<int16_t>(*it * 0x8000));
  for (auto i = 0; i < static_cast<int>(static_cast<float>(audio->n) / rate * .1f); ++i)
    ret.second.push_back(0);
  SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);

  return ret;
}
