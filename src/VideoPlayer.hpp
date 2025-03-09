#pragma once

#include <memory>
#include <string>

#include "AppState.hpp"
#include "ShaderManager.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Texture.hpp"
#include "al/graphics/al_VAOMesh.hpp"
#include "al/ui/al_ControlGUI.hpp"
// #include "al_ext/video/al_VideoDecoder.hpp"
#include "al_ext/video/al_VideoPlayer.hpp"


using namespace al;

struct VideoPlayerW {
  void init(const SearchPaths& paths);
  void create();
  void update(al_sec dt, State& state, bool isPrimary);
  void draw(Graphics& g, Nav& nav, bool isPrimary);
  bool loadVideoFile();
  void registerParams(ControlGUI& gui, PresetHandler& presets,
                      PresetSequencer& seq, State& state);

  std::unique_ptr<VideoPlayer> videoDecoder;
  ShaderManager shaderManager;
  Texture texY, texU, texV;
  VAOMesh sphereMesh;

  std::string dataPath;
  ParameterString videoToLoad{"videoToLoad", ""};
  bool loadVideo{false};

  ParameterBool playingVideo{"playingVideo", "", 0.0};
  ParameterBool renderVideoInSim{"renderVideoInSim", "", 0.0};

  Parameter videoBlend{"videoBlend", "", 1.0, 0.0, 1.0};
  ParameterPose videoPose{"videoPose", "", Pose(Vec3d(0, 0, 0))};
  ParameterVec3 videoScale{"videoScale", "", Vec3f(1, 1, 1)};

  Trigger playAerialImages{"Play_AerialImages", ""};
  Trigger playSF{"Play_SF", ""};
  Trigger playBoardwalk{"Play_Boardwalk", ""};
  Trigger playCoral{"Play_Coral", ""};
  Trigger playOverfishing{"Play_Overfishing", ""};
  Trigger playAcidification{"Play_Acidification", ""};
  Trigger playBoat{"Play_Boat", ""};
};