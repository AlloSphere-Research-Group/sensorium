#pragma once

#include "al/spatial/al_Pose.hpp"
#include "al/types/al_Color.hpp"

constexpr int chiyears = 11;  // Total number of years (2003~2013)
constexpr int years = 12;     // Total number of years (2012~2023)
constexpr int stressors = 12; // Total number of stressors
constexpr int num_cloud = 3;  // Total number of cloud images
constexpr int frames = 400;   // Total number of CO2 frames

struct State {
  // video player
  bool videoPlaying;
  double global_clock;

  // ocean data viewer
  double co2_clock;
  al::Pose global_pose;
  bool swtch[stressors]{false};
  bool cloud_swtch[num_cloud]{false};
  bool molph{false};
  bool molphFrame{false};
  bool co2_swtch{false};
  bool co2Playing{false};
  float year;
  float chiyear;
};