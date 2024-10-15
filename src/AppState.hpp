#pragma once

#include "al/spatial/al_Pose.hpp"
#include "al/types/al_Color.hpp"

namespace data {
constexpr int years_chi = 11;     // Total number of years (2003~2013)
constexpr int years_nasa = 12;    // Total number of years (2012~2023)
constexpr int num_stressors = 12; // Total number of stressors
constexpr int num_clouds = 3;     // Total number of cloud images
} // namespace data

struct State {
  bool videoPlaying{false};
  double video_clock{0.0};
  double co2_clock{0.0};

  al::Pose global_pose;
  float nasa_year{2013.f};
  float chi_year{2003.f};

  bool swtch[data::num_stressors];
  bool cloud_swtch[data::num_clouds];

  bool co2_swtch{false};
  bool co2Playing{false};
};