 
#ifndef APPSTATE_HPP
#define APPSTATE_HPP

static const int years = 12;       // Total number of years (2012~2023)
static const int stressors = 12;   // Total number of stressors
static const int num_cloud = 3;    // Total number of stressors
static const int num_county = 187; // Total number of stressors

struct State {
  // video player
  bool videoPlaying;
  double global_clock;
  double global_clock_next;

  // ocean data viewer
  al::Pose global_pose;
  bool swtch[stressors]{false};
  bool cloud_swtch[num_cloud]{false};
  bool molph{false};
  bool co2_swtch{false};
  float lux;
  float year;
  float radius;
  int osc_click[10];
  // Mesh emission_mesh;
  al::Vec3f render_co2_pos[500];
  al::Color render_co2_col[500];
};


#endif