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
  PresetHandler presets{"data/presets", true};
  PresetSequencer sequencer;
  SequenceRecorder recorder;

  SearchPaths searchPaths;
  struct WatchedFile {
    File file;
    al_sec modified;
  };
  std::map<std::string, WatchedFile> watchedFiles;
  al_sec watchCheckTime;


  void onInit() override {

    searchPaths.addSearchPath(".", false);
    searchPaths.addAppPaths();
    searchPaths.addRelativePath("src/shaders", true);

    cuttleboneDomain = CuttleboneDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone" << std::endl;
      quit();
    }
    // OSC receiver
    // server.open(4444,"0.0.0.0", 0.05);
    // server.handler(oscDomain()->handler());
    // server.start();

    if(isPrimary() && al::sphere::isSphereMachine()){
      audioIO().channelsOut(70);
      audioIO().print();
    }

    oceanDataViewer.onInit();
    videoPlayer.onInit();
    audioPlayer.onInit();
    // synchronizes attached params accross renderers
    parameterServer() << videoPlayer.renderPose << videoPlayer.renderScale << videoPlayer.videoToLoad << videoPlayer.brightness << videoPlayer.blend0 << videoPlayer.blend1 << videoPlayer.swapVideo;

  }

  void reloadShaders() { 
    loadShader(oceanDataViewer.shaderDataset, "tex.vert", "colormap.frag");
  }


  void onCreate() override {

    lens().near(0.01).fovy(45).eyeSep(0);
    nav().pos(0, 0, -15);
    nav().quat().fromAxisAngle(0.5 * M_2PI, 0, 1, 0);

    navControl().vscale(0.125*0.1*0.5);
    navControl().tscale(2*0.1*0.5);

    oceanDataViewer.onCreate();

    // std::thread thread([&] {
    oceanDataViewer.loadChiData();
    // });
    // thread.detach(); 

    videoPlayer.onCreate(state(), isPrimary());


    // Initialize GUI and Parameter callbacks
    if (isPrimary()) {
      sequencer.setDirectory("data/presets");
      recorder.setDirectory("data/presets");

      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      gui = &guiDomain->newGUI();

      oceanDataViewer.registerParams(gui, presets, sequencer, recorder, nav(), state());
      videoPlayer.registerParams(gui, presets, sequencer, recorder, state());
      audioPlayer.registerParams(gui, presets, sequencer, state());

      sequencer << presets;
      // *gui << sequencer << recorder;
    }

    videoPlayer.videoToLoad.registerChangeCallback([&](std::string value) {
      std::cout << "loading file to videoDecoderNext: " << value << std::endl;
      videoPlayer.loadVideo = true;
    });
    videoPlayer.swapVideo.registerChangeCallback([&](float value) {
      std::cout << "swap video to videoDecode" << std::endl;
      videoPlayer.doSwapVideo = true;
    });
    // enable if parameter needs to be shared
    // parameterServer() << lat << lon << radius;

    reloadShaders();
  }

  void onAnimate(double dt) override {

    if (watchCheck()) {
      printf("files changed!\n");
      reloadShaders();
    }

    oceanDataViewer.camPose.setNoCalls(nav());
    oceanDataViewer.onAnimate(dt, nav(), state(), isPrimary());
    videoPlayer.onAnimate(dt, state(), isPrimary());
  }

  void onSound(AudioIOData &io) override { 
    // oceanDataViewer.onSound(io); 
    audioPlayer.onSound(io);
  }


  void onDraw(Graphics &g) override {
    if(!state().videoPlaying)
      oceanDataViewer.onDraw(g, nav(), state());
    
    videoPlayer.onDraw(g, nav(), state(), isPrimary());
  }

  bool onKeyDown(const Keyboard &k) override {
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
        // Notice that you don't need to add the extension ".sequence" to the name
        sequencer.setVerbose(true);
        if(sequencer.running()){
          sequencer.stopSequence("sensorium");
          videoPlayer.playingVideo = false;
        }else{
          sequencer.playSequence("sensorium");
        }
      }
    }
    return true;
  }

  // bool onKeyDown(const Keyboard &k) override {
  //   switch (k.key()) {
  //   case '1':
  //     oceanDataViewer.setGeoTarget(53.54123998879464, 9.950943100405375, 3.2, 4.0);
  //     return true;
  //   case '2':
  //     oceanDataViewer.setGeoTarget(54.40820774011447, 12.593582740321027, 3.2, 4.0);
  //     return true;
  //   case '3':
  //     oceanDataViewer.setGeoTarget(53.91580380807132, 9.531183185073399, 3.2, 4.0);
  //     return true;
  //   case '4':
  //     oceanDataViewer.setGeoTarget(53.78527983917765, 9.409205631903566, 3.2, 4.0);
  //     return true;
  //   case '5':
  //     oceanDataViewer.setGeoTarget(0, -80, 30, 6.0);
  //     return true;
  //   case '0':
  //     oceanDataViewer.setGeoTarget(0, 138, 0.01, 4.0);
  //     return true;
  //   case 'r':
  //     oceanDataViewer.setGeoTarget(0, 0, 5, 4.0);
  //     return true;
  //   case '=':
  //     oceanDataViewer.faceTo = !oceanDataViewer.faceTo;
  //     return true;
  //   case '9':
  //     state().molph = !state().molph;
  //     oceanDataViewer.year = 2003;
  //     return true;
  //   case '8':
  //     for (int i = 0; i < stressors; i++)
  //       state().swtch[i] = false;
  //     return true;

  //   case ' ':
  //     oceanDataViewer.s_years = 1.0;
  //     return true;
  //   default:
  //     return false;
  //   }
  // }

  // void onMessage(osc::Message& m) override {
  // }



  //// shader utils
  void watchFile(std::string path) {
    File file(searchPaths.find(path).filepath());
    watchedFiles[path] = WatchedFile{file, file.modified()};
  }

  bool watchCheck() {
    bool changed = false;
    if (floor(al_system_time()) > watchCheckTime) {
      watchCheckTime = floor(al_system_time());
      for (std::map<std::string, WatchedFile>::iterator i =
               watchedFiles.begin();
           i != watchedFiles.end(); i++) {
        WatchedFile &watchedFile = (*i).second;
        if (watchedFile.modified != watchedFile.file.modified()) {
          watchedFile.modified = watchedFile.file.modified();
          changed = true;
        }
      }
    }
    return changed;
  }

  // read in a .glsl file
  // add it to the watch list of files
  // replace shader #include directives with corresponding file code
  std::string loadGlsl(std::string filename) {
    watchFile(filename);
    std::string code = File::read(searchPaths.find(filename).filepath());
    size_t from = code.find("#include \"");
    if (from != std::string::npos) {
      size_t capture = from + strlen("#include \"");
      size_t to = code.find("\"", capture);
      std::string include_filename = code.substr(capture, to - capture);
      std::string replacement =
          File::read(searchPaths.find(include_filename).filepath());
      code = code.replace(from, to - from + 2, replacement);
      // printf("code: %s\n", code.data());
    }
    return code;
  }

  void loadShader(ShaderProgram &program, std::string vp_filename,
                  std::string fp_filename) {
    std::string vp = loadGlsl(vp_filename);
    std::string fp = loadGlsl(fp_filename);
    program.compile(vp, fp);
  }


};

int main() {
  SensoriumApp app;
  app.dimensions(1200, 800);
  app.configureAudio(44100, 512, 2, 0);
  app.start();
}
