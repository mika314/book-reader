#include "text-to-speech.hpp"
#include <cstring>
#include <log/log.hpp>

TextToSpeech::TextToSpeech()
  : engine(RHVoice_new_tts_engine([]() {
      static RHVoice_init_params params;
      params.data_path = "/usr/share/RHVoice";
      params.config_path = "/etc/RHVoice";
      params.resource_paths = nullptr;
      params.callbacks.set_sample_rate = setSampleRate;
      params.callbacks.play_speech = playSpeech;
      params.callbacks.process_mark = nullptr;
      params.callbacks.word_starts = nullptr;
      params.callbacks.word_ends = nullptr;
      params.callbacks.sentence_starts = nullptr;
      params.callbacks.sentence_ends = nullptr;
      params.callbacks.play_audio = nullptr;
      params.callbacks.done = nullptr;
      params.options = 0;
      return &params;
    }()))
{
  auto voiceNum = RHVoice_get_number_of_voices(engine);
  LOG("number of voices:", voiceNum);
  auto voices = RHVoice_get_voice_profiles(engine);
  for (auto i = 0U; i < voiceNum; ++i)
    LOG(i, voices[i]);
}

TextToSpeech::~TextToSpeech()
{
  RHVoice_delete_tts_engine(engine);
}

auto TextToSpeech::operator()(const std::string &text, float rate) const
  -> std::pair<int, std::vector<int16_t>>
{
  RHVoice_synth_params params;
  params.voice_profile = "Slt";
  /* The values must be between -1 and 1. */
  /*     They are normalized this way, because users can set different */
  /* parameters for different voices in the configuration file. */
  params.absolute_rate = rate - 1.f;
  params.absolute_pitch = 0.;
  params.absolute_volume = 1.;
  /* Relative values, in case someone needs them. */
  /* If you don't, just set each of them to 1. */
  params.relative_rate = 1.;
  params.relative_pitch = 1.;
  params.relative_volume = 1.;
  /* Set to RHVoice_punctuation_default to allow the synthesizer to decide */
  params.punctuation_mode = RHVoice_punctuation_default;
  /* Optional */
  params.punctuation_list = "";
  /* This mode only applies to reading by characters. */
  /* If your program doesn't support this setting, set to RHVoice_capitals_default. */
  params.capitals_mode = RHVoice_capitals_default;
  /* Set to 0 for defaults. */
  params.flags = 0;

  std::pair<int, std::vector<int16_t>> ret;
  auto m = RHVoice_new_message(engine, text.c_str(), text.size(), RHVoice_message_text, &params, &ret);
  RHVoice_speak(m);

  return ret;
}

auto TextToSpeech::setSampleRate(int sample_rate, void *user_data) -> int
{
  auto &ret = *static_cast<std::pair<int, std::vector<int16_t>> *>(user_data);
  ret.first = sample_rate;
  return true;
}

auto TextToSpeech::playSpeech(const short *samples, unsigned int count, void *user_data) -> int
{
  auto &ret = *static_cast<std::pair<int, std::vector<int16_t>> *>(user_data);
  ret.second.insert(std::end(ret.second), samples, samples + count);
  return true;
}
