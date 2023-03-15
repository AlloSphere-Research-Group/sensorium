 
#ifndef APPSTATE_HPP
#define APPSTATE_HPP

static const int years = 11;       // Total number of years (2003~2013)
static const int stressors = 12;   // Total number of stressors
static const int num_cloud = 3;    // Total number of stressors
static const int num_county = 187; // Total number of stressors

struct State {
  // video player
  int videoLoadIndex;
  bool videoPlaying;
  bool videoRendering;
  double global_clock;

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