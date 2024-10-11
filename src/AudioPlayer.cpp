#include "AudioPlayer.hpp"
#include "al/sphere/al_SphereUtils.hpp"

using namespace al;

void AudioPlayer::init() {
  // const char name[] = "data/jkm-wave-project.wav";
  // const char name[] = "data/new-sensorium-project.wav";
  const char name[] = "data/Normalized-Complete-Sensorium-project.wav";
  if (!audioPlayerTS.open(name)) {
    std::cerr << "File not found: " << name << std::endl;
  }
  // std::cout << "sampleRate: " << audioPlayerTS.soundFile.sampleRate
  //           << std::endl;
  // std::cout << "channels: " << audioPlayerTS.soundFile.channels << std::endl;
  // std::cout << "frameCount: " << audioPlayerTS.soundFile.frameCount
  //           << std::endl;
  audioPlayerTS.setPause();
  audioPlayerTS.setLoop();

  if (!intro.open("data/Sketches.wav")) {
    std::cerr << "File not found: " << "data/Sketches.wav" << std::endl;
  }
  intro.setPause();
  intro.setNoLoop();
}

void AudioPlayer::onSound(AudioIOData &io) {
  int frames = (int)io.framesPerBuffer();
  int channels = audioPlayerTS.soundFile.channels;
  int bufferLength = frames * channels;

  if ((int)buffer.size() < bufferLength) {
    buffer.resize(bufferLength);
  }
  if ((int)buffer2.size() < frames) {
    buffer2.resize(frames);
  }
  
  audioPlayerTS.getFrames(frames, buffer.data(), (int)buffer.size());
  intro.getFrames(frames, buffer2.data(), (int)buffer2.size());
  int second = (channels < 2) ? 0 : 1;

  while (io()) {
    int frame = (int)io.frame();
    int idx = frame * channels;
    float l = buffer[idx];
    float r = buffer[idx + second];
    float s = buffer2[frame];

    if (al::sphere::isSphereMachine()) {
      for (int i = 0; i < 12; i++)
        io.out(i) = (s + l) * AudioVolume * 0.4;

      for (int i = 16; i < 31; i++)
        io.out(i) = l * AudioVolume * 0.4;

      io.out(24) = s * AudioVolume * 0.4;

      for (int i = 31; i < 46; i++)
        io.out(i) = r * AudioVolume * 0.4;

      for (int i = 49; i <= 60; i++)
        io.out(i) = r * AudioVolume * 0.4;

      io.out(39) = s * AudioVolume * 0.4;
      // io.out(48) = (l + r) * SubVolume * 0.5;
      io.out(47) = (l + r) * SubVolume * 0.5;
    } else {
      io.out(0) = (s + l) * AudioVolume;
      io.out(1) = (s + r) * AudioVolume;
    }
  }
}

void AudioPlayer::registerParams(ControlGUI &gui, PresetHandler &presets,
                                 PresetSequencer &seq) {
  gui << playIntro << playAudio << playLoop << pauseAudio << rewindAudio
      << AudioVolume << SubVolume;

  presets << AudioVolume << SubVolume << playAudio << rewindAudio;
  seq << playIntro << playAudio << pauseAudio << rewindAudio << AudioVolume
      << SubVolume;

  playIntro.registerChangeCallback([&](float value) {
    if (value == 1.0) {
      intro.setRewind();
      intro.setPlay();
    } else
      intro.setPause();
  });
  playAudio.registerChangeCallback([&](float value) {
    if (value == 1.0)
      audioPlayerTS.setPlay();
  });
  rewindAudio.registerChangeCallback([&](float value) {
    if (value == 1.0)
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
    if (value == 1.0)
      audioPlayerTS.setPause();
  });
}