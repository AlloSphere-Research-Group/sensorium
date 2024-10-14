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
  void create();
  void update(double dt, Nav &nav, State &state, bool isPrimary);
  void draw(Graphics &g, Nav &nav, State &state);

  float easeIn(float _value, float _target, float _speed);
  void setGeoTarget(float la, float lo, float r = 3.2);

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



  ParameterVec3 llr{"llr", "Nav"};
  ParameterPose camPose{"camPose", "Nav"};
  
  ParameterBool faceTo{"Face_Center", "Nav", 1.0};
  ParameterBool animateCam{"animateCam", "Nav", 0.0};

  Pose navTarget;
  float anim_speed = 0.0;
  float anim_target_speed = 0.002;

  Parameter blend{"blend", "", 1.0, 0.0, 1.0};

  Parameter year{"Year", 2013, 2013, 2023};
  Parameter chiyear{"CHIYear", 2003, 2003, 2013};

  Parameter gain{"Audio", "Sound", 0, 0, 2};

  ParameterBool s_shp{"Shipping", "", 0.0};
  ParameterBool s_carbon{"Ocean_Carbon", "", 0.0};
  ParameterBool s_slr{"Sea_level_rise", "", 0.0};
  ParameterBool s_chl{"Chlorophyll_Concentration", "", 0.0};
  ParameterBool s_flh{"Fluorescence_Line_Height", "", 0.0};
  ParameterBool s_oa{"Ocean_acidification", "", 0.0};
  ParameterBool s_sst{"Sea_surface_temperature", "", 0.0};
  ParameterBool s_fish{"Over-fishing", "", 0.0};
  ParameterBool s_plastics{"Plastics", "", 0.0};
  ParameterBool s_resiliency{"Resiliency_Map", "", 0.0};
  ParameterBool s_cloud{"Clouds", "", 0.0};
  ParameterBool s_cloud_storm{"Clouds_Storm", "", 0.0};
  ParameterBool s_cloud_eu{"Clouds_EU", "", 0.0};
  ParameterBool s_co2{"CO2", "", 0.0};
  ParameterBool s_co2_vid{"CO2-Video", "", 0.0};
  ParameterBool s_nav{"Explore_Globe", "", 0.0};
  ParameterBool s_years{"CycleYears", "", 0.0};


  


  Texture pic[years][stressors];
  bool loaded[years][stressors];

  Texture cloud[num_cloud];
  float morph_year;




  
};