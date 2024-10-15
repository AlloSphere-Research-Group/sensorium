#pragma once

#include "AppState.hpp"
#include "ShaderManager.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Texture.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al_ext/video/al_VideoDecoder.hpp"
#include <iostream>
#include <memory>

using namespace al;

struct OceanDataViewer {
  void init(const SearchPaths &paths);
  void create(Lens &lens);
  void update(double dt, Nav &nav, State &state, bool isPrimary);
  void draw(Graphics &g, Nav &nav, State &state, Lens &lens);

  float easeIn(float _value, float _target, float _speed);
  void setNavTarget(float lat, float lon, float alt = 3.2);

  void registerParams(ControlGUI &gui, PresetHandler &presets,
                      PresetSequencer &seq, State &state, Nav &nav);

  void loadAllData();
  void loadDataset(const std::string &pathPrefix, int stressorIndex);
  void loadChiDataset(const std::string &prefix, const std::string &postfix,
                      int stressorIndex);
  void loadCO2Dataset(const std::string &video);

  std::unique_ptr<VideoDecoder> videoDecoder;
  Texture texY, texU, texV;

  ShaderManager shaderManager;
  Texture spaceTex, earthTex;
  VAOMesh spaceMesh, earthMesh;

  std::string dataPath;

  // TODO: add blend to shaders
  Parameter blend{"blend", "", 1.0, 0.0, 1.0};

  ParameterVec3 geoCoord{"geoCoord", "Nav"};
  ParameterBool rotateGlobe{"Rotate_Globe", "", 0.0};
  ParameterBool faceTo{"Face_Center", "Nav", 1.0};
  ParameterBool animateCam{"animateCam", "Nav", 0.0};
  Pose navTarget;
  float anim_speed = 0.0;
  float anim_target_speed = 0.002;

  ParameterBool cycleYears{"CycleYears", "", 0.0};
  Parameter nasaYear{"NASAYear", 2013, 2013, 2023};
  Parameter chiYear{"CHIYear", 2003, 2003, 2013};

  Texture cloud[data::num_clouds];
  Texture pic[data::years_nasa][data::num_stressors];
  bool loaded[data::years_nasa][data::num_stressors];

  ParameterBool show_sst{"Sea_surface_temperature", "", 0.0};
  ParameterBool show_carbon{"Ocean_Carbon", "", 0.0};
  ParameterBool show_chl{"Chlorophyll_Concentration", "", 0.0};
  ParameterBool show_flh{"Fluorescence_Line_Height", "", 0.0};
  ParameterBool show_fish{"Over-fishing", "", 0.0};
  ParameterBool show_ship{"Shipping", "", 0.0};
  ParameterBool show_oa{"Ocean_acidification", "", 0.0};
  ParameterBool show_slr{"Sea_level_rise", "", 0.0};
  ParameterBool show_clouds{"Clouds", "", 0.0};
  ParameterBool show_co2{"CO2", "", 0.0};
};