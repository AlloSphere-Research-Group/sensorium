
#ifndef AUDIO_PLAYER_HPP
#define AUDIO_PLAYER_HPP

#include "al/sound/al_SoundFile.hpp"

namespace al {

struct AudioPlayer {

  SoundFilePlayerTS audioPlayerTS;
  std::vector<float> buffer;
  ParameterBool playAudio{"Play_Audio", "", 0};
  ParameterBool playLoop{"Loop Audio", "", 1};
  ParameterBool pauseAudio{"Pause Audio", "", 0};
  ParameterBool rewindAudio{"Rewind Audio", "", 0};
  Parameter AudioVolume{"Audio_Volume", 0.6, 0, 2.5};
  Parameter SubVolume{"Sub_Volume", 0.6, 0, 2.5};

  void onInit(){
    // const char name[] = "data/jkm-wave-project.wav";
    // const char name[] = "data/new-sensorium-project.wav";
    const char name[] = "data/Normalized-Complete-Sensorium-project.wav";
    if (!audioPlayerTS.open(name)) {
      std::cerr << "File not found: " << name << std::endl;
    }
    std::cout << "sampleRate: " << audioPlayerTS.soundFile.sampleRate << std::endl;
    std::cout << "channels: " << audioPlayerTS.soundFile.channels << std::endl;
    std::cout << "frameCount: " << audioPlayerTS.soundFile.frameCount << std::endl;
    audioPlayerTS.setPause();
    audioPlayerTS.setLoop();
  }
  void registerParams(ControlGUI *gui, PresetHandler &presets, PresetSequencer &seq, State &state) {
    *gui << playAudio << playLoop << pauseAudio << rewindAudio << AudioVolume << SubVolume;
    
    presets << AudioVolume << SubVolume << playAudio << rewindAudio;
    seq << playAudio << rewindAudio << AudioVolume << SubVolume;

    playAudio.registerChangeCallback([&](float value) {
      if(value == 1.0) audioPlayerTS.setPlay();
    });
    rewindAudio.registerChangeCallback([&](float value) {
      if(value == 1.0) audioPlayerTS.setRewind();
    });
    playLoop.registerChangeCallback([&](float value) {
      if (value) {
        audioPlayerTS.setLoop();
      } else {
        audioPlayerTS.setNoLoop();
      }
    });
    pauseAudio.registerChangeCallback([&](float value) {
      if(value == 1.0) audioPlayerTS.setPause();
    });
  }
  void onSound(AudioIOData& io) {

    int frames = (int)io.framesPerBuffer();
    int channels = audioPlayerTS.soundFile.channels;
    int bufferLength = frames * channels;
    if ((int)buffer.size() < bufferLength) {
      buffer.resize(bufferLength);
    }
    audioPlayerTS.getFrames(frames, buffer.data(), (int)buffer.size());
    int second = (channels < 2) ? 0 : 1;
    while (io()) {
      int frame = (int)io.frame();
      int idx = frame * channels;
      float l = buffer[idx];
      float r = buffer[idx + second];

      if(al::sphere::isSphereMachine()){
        for(int i=1; i <= 12; i++)
          io.out(i) = l * AudioVolume * 0.2;
        io.out(24) = l * AudioVolume * 1.0;
        for(int i=49; i <= 60; i++)
          io.out(i) = r * AudioVolume * 0.2;
        io.out(39) = r * AudioVolume * 1.0;
        io.out(48) = (l+r) * SubVolume * 0.5;
      } else {
        io.out(0) = l * AudioVolume;
        io.out(1) = r * AudioVolume;
      }
    }
  }

};

}
#endif
