// Sensorium Main
// TODO :
// - gradually fade in-out stressors

#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"

#include "AppState.hpp"
#include "OceanDataViewer.hpp"
#include "VideoPlayer.hpp"
#include "AudioPlayer.hpp"

using namespace al;

struct SensoriumApp : public DistributedAppWithState<State> {

  std::shared_ptr<CuttleboneDomain<State>> cuttleboneDomain;

  OceanDataViewer oceanDataViewer;
  VideoPlayer videoPlayer;
  AudioPlayer audioPlayer;

  ControlGUI *gui;

  void onInit() override {
    cuttleboneDomain = CuttleboneDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone" << std::endl;
      quit();
    }
    // OSC receiver
    // server.open(4444,"0.0.0.0", 0.05);
    // server.handler(oscDomain()->handler());
    // server.start();

    oceanDataViewer.onInit();
    videoPlayer.onInit();
    audioPlayer.onInit();
  }

  void onCreate() override {

    lens().fovy(45).eyeSep(0);
    nav().pos(0, 0, -5);
    nav().quat().fromAxisAngle(0.5 * M_2PI, 0, 1, 0);

    oceanDataViewer.onCreate();
    oceanDataViewer.loadChiData();

    videoPlayer.onCreate(state(), isPrimary());


    // Initialize GUI and Parameter callbacks
    if (isPrimary()) {
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      gui = &guiDomain->newGUI();

      oceanDataViewer.registerParams(gui, nav(), state());
      videoPlayer.registerParams(gui, state());
      audioPlayer.registerParams(gui, state());
    }
    // enable if parameter needs to be shared
    // parameterServer() << lat << lon << radius;
  }

  void onAnimate(double dt) override {
    oceanDataViewer.onAnimate(dt, nav(), state(), isPrimary());
    videoPlayer.onAnimate(dt, state(), isPrimary());
  }

  void onSound(AudioIOData &io) override { 
    // oceanDataViewer.onSound(io); 
    audioPlayer.onSound(io);
  }


  void onDraw(Graphics &g) override {
    if(!state().videoRendering)
      oceanDataViewer.onDraw(g, nav(), state());
    
    videoPlayer.onDraw(g, state(), isPrimary());
  }

  bool onKeyDown(const Keyboard &k) override {
    switch (k.key()) {
    case '1':
      oceanDataViewer.setGeoTarget(53.54123998879464, 9.950943100405375, 3.2, 4.0);
      return true;
    case '2':
      oceanDataViewer.setGeoTarget(54.40820774011447, 12.593582740321027, 3.2, 4.0);
      return true;
    case '3':
      oceanDataViewer.setGeoTarget(53.91580380807132, 9.531183185073399, 3.2, 4.0);
      return true;
    case '4':
      oceanDataViewer.setGeoTarget(53.78527983917765, 9.409205631903566, 3.2, 4.0);
      return true;
    case '5':
      oceanDataViewer.setGeoTarget(0, -80, 30, 6.0);
      return true;
    case '0':
      oceanDataViewer.setGeoTarget(0, 138, 0.01, 4.0);
      return true;
    case 'r':
      oceanDataViewer.setGeoTarget(0, 0, 5, 4.0);
      return true;
    case '=':
      oceanDataViewer.faceTo = !oceanDataViewer.faceTo;
      return true;
    case '9':
      state().molph = !state().molph;
      oceanDataViewer.year = 2003;
      return true;
    case '8':
      for (int i = 0; i < stressors; i++)
        state().swtch[i] = false;
      return true;

    default:
      return false;
    }
  }

  // void onMessage(osc::Message& m) override {
  // }
};

int main() {
  SensoriumApp app;
  app.dimensions(1200, 800);
  app.start();
}
