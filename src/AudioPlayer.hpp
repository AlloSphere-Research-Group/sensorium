#pragma once

#include "al/sound/al_SoundFile.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include <vector>

using namespace al;

struct AudioPlayer {
  void init();
  void onSound(AudioIOData &io);
  void registerParams(ControlGUI &gui, PresetHandler &presets,
                      PresetSequencer &seq);

  SoundFilePlayerTS audioPlayerTS;
  SoundFilePlayerTS intro;

  std::vector<float> buffer;
  std::vector<float> buffer2;

  ParameterBool playIntro{"Play_Intro", "", 0};
  ParameterBool playAudio{"Play_Audio", "", 0};
  ParameterBool playLoop{"Loop_Audio", "", 1};
  ParameterBool pauseAudio{"Pause_Audio", "", 0};
  ParameterBool rewindAudio{"Rewind_Audio", "", 0};
  Parameter AudioVolume{"Audio_Volume", 1.1, 0, 2.5};
  Parameter SubVolume{"Sub_Volume", 0.5, 0, 2.5};
};