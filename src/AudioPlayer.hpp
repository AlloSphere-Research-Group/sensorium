
#ifndef AUDIO_PLAYER_HPP
#define AUDIO_PLAYER_HPP

#include "al/sound/al_SoundFile.hpp"

namespace al {

struct AudioPlayer {

  SoundFilePlayerTS audioPlayerTS;
  std::vector<float> buffer;
  ParameterBool playAudio{"Play Audio", "", 0};
  ParameterBool playLoop{"Loop Audio", "", 1};
  ParameterBool pauseAudio{"Pause Audio", "", 0};
  ParameterBool rewindAudio{"Rewind Audio", "", 0};
  Parameter AudioVolume{"Audio Volume", 0.6, 0, 2.5};

  void onInit(){
    const char name[] = "data/jkm-wave-project.wav";
    if (!audioPlayerTS.open(name)) {
      std::cerr << "File not found: " << name << std::endl;
    }
    std::cout << "sampleRate: " << audioPlayerTS.soundFile.sampleRate << std::endl;
    std::cout << "channels: " << audioPlayerTS.soundFile.channels << std::endl;
    std::cout << "frameCount: " << audioPlayerTS.soundFile.frameCount << std::endl;
    audioPlayerTS.setPause();
    audioPlayerTS.setLoop();
  }
  void registerParams(ControlGUI *gui, State &state) {
    *gui << playAudio << playLoop << pauseAudio << rewindAudio << AudioVolume;

    playAudio.registerChangeCallback([&](float value) {
      audioPlayerTS.setPlay();
    });
    rewindAudio.registerChangeCallback([&](float value) {
      audioPlayerTS.setRewind();
    });
    playLoop.registerChangeCallback([&](float value) {
      if (value) {
        audioPlayerTS.setLoop();
      } else {
        audioPlayerTS.setNoLoop();
      }
    });
    pauseAudio.registerChangeCallback([&](float value) {
      audioPlayerTS.setPause();
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
      io.out(0) = buffer[idx] * AudioVolume;
      io.out(1) = buffer[idx + second] * AudioVolume;
    }
  }

};

}
#endif