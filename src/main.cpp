// Sensorium Main
// TODO :
// - gradually fade in-out stressors

#include "AppState.hpp"
#include "AudioPlayer.hpp"
#include "OceanDataViewer.hpp"
#include "VideoPlayer.hpp"
#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"

using namespace al;

struct SensoriumApp : public DistributedAppWithState<State> {
  std::shared_ptr<CuttleboneDomain<State>> cuttleboneDomain;

  OceanDataViewer oceanDataViewer;
  VideoPlayer videoPlayer;
  AudioPlayer audioPlayer;

  PresetHandler presets{"data/presets", true};
  PresetSequencer sequencer;
  SequenceRecorder recorder;

  SearchPaths searchPaths;

  void onInit() {
    cuttleboneDomain = CuttleboneDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone" << std::endl;
      quit();
    }

    if (isPrimary() && sphere::isSphereMachine()) {
      audioIO().channelsOut(70);
      audioIO().print();
    }

    searchPaths.addSearchPath(".", false);
    searchPaths.addAppPaths();
    searchPaths.addRelativePath("src/shaders", true);

    oceanDataViewer.init(searchPaths);
    videoPlayer.init(searchPaths);
    audioPlayer.init();

    // synchronizes attached params accross renderers
    parameterServer() << videoPlayer.renderPose << videoPlayer.renderScale
                      << videoPlayer.videoToLoad << videoPlayer.brightness
                      << videoPlayer.blend0 << videoPlayer.blend1
                      << oceanDataViewer.blend;
  }

  void onCreate() {
    // lens().near(0.01).fovy(45).eyeSep(0);
    lens().fovy(45).eyeSep(0);
    nav().pos(0, 0, -15);
    nav().quat().fromAxisAngle(0.5 * M_2PI, 0, 1, 0);

    navControl().vscale(0.125 * 0.1 * 0.5);
    navControl().tscale(2 * 0.1 * 0.5);

    oceanDataViewer.create();
    videoPlayer.create();

    // Initialize GUI and Parameter callbacks
    if (isPrimary()) {
      // TODO: review state
      state().global_clock = 0;
      state().videoPlaying = false;

      sequencer.setDirectory("data/presets");
      recorder.setDirectory("data/presets");

      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      auto &gui = guiDomain->newGUI();

      oceanDataViewer.registerParams(gui, presets, sequencer, state(), nav());
      videoPlayer.registerParams(gui, presets, sequencer, state());
      audioPlayer.registerParams(gui, presets, sequencer);

      sequencer << presets;
      // *gui << sequencer << recorder;
    }
  }

  void onAnimate(double dt) {
    oceanDataViewer.update(dt, nav(), state(), isPrimary());
    videoPlayer.update(dt, state(), isPrimary());
  }

  void onSound(AudioIOData &io) { audioPlayer.onSound(io); }

  void onDraw(Graphics &g) {
    oceanDataViewer.draw(g, nav(), state());
    videoPlayer.draw(g, nav(), state(), isPrimary());
  }

  bool onKeyDown(const Keyboard &k) {
    std::string presetName = std::to_string(k.keyAsNumber());
    if (k.alt()) {
      if (k.isNumber()) { // Use alt + any number key to store preset
        presets.storePreset(k.keyAsNumber(), presetName);
        std::cout << "Storing preset:" << presetName << std::endl;
      }
    } else {
      if (k.isNumber()) { // Recall preset using the number keys
        presets.recallPreset(k.keyAsNumber());
        std::cout << "Recalling preset:" << presetName << std::endl;
      }
      if (k.key() == ' ') {
        // Notice that you don't need to add the extension ".sequence" to the
        // name
        sequencer.setVerbose(true);
        if (sequencer.running()) {
          sequencer.stopSequence("sensorium");
          videoPlayer.playingVideo = false;
        } else {
          sequencer.playSequence("sensorium");
        }
      }
    }
    return true;
  }
};

int main() {
  SensoriumApp app;
  app.dimensions(1200, 800);
  app.configureAudio(44100, 512, 60, 0);
  app.start();
}