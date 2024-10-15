#include "VideoPlayer.hpp"
#include "al/sphere/al_SphereUtils.hpp"

using namespace al;

void VideoPlayer::init(const SearchPaths &paths) {
  shaderManager.setSearchPaths(paths);
  shaderManager.setPollInterval(1);

  if (sphere::isSphereMachine()) {
    if (sphere::isRendererMachine()) {
      dataPath = "/data/Sensorium/video/";
    } else {
      dataPath = "/Volumes/Data/Sensorium/video/";
    }
  } else {
    dataPath = "data/video/";
  }
}

void VideoPlayer::create() {
  // compile & initialize shader
  shaderManager.add("video", "mono.vert", "video.frag");
  auto &videoShader = shaderManager.get("video");
  videoShader.begin();
  videoShader.uniform("texY", 0);
  videoShader.uniform("texU", 1);
  videoShader.uniform("texV", 2);
  videoShader.uniform("videoBlend", 1.f);
  videoShader.end();

  // generate texture
  texY.filter(Texture::LINEAR);
  texY.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);
  texU.filter(Texture::LINEAR);
  texU.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);
  texV.filter(Texture::LINEAR);
  texV.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);

  addTexSphere(sphereMesh, 10, 50);
  sphereMesh.update();

  videoToLoad.registerChangeCallback([&](std::string value) {
    std::cout << "loading video file: " << value << std::endl;
    loadVideo = true;
  });
}

void VideoPlayer::update(al_sec dt, Nav &nav, State &state, bool isPrimary) {
  shaderManager.update();

  if (loadVideo) {
    if (loadVideoFile()) {
      loadVideo = false;
      if (isPrimary) {
        state.video_clock = 0.0;
        nav.home();
      }
    }
  }

  if (playingVideo.get()) {
    if (isPrimary) {
      state.video_clock += dt;
    }

    MediaFrame *frame;
    if (videoDecoder != nullptr) {
      frame = videoDecoder->getVideoFrame(state.video_clock);
      if (frame) {
        texY.submit(frame->dataY.data());
        texU.submit(frame->dataU.data());
        texV.submit(frame->dataV.data());
        videoDecoder->gotVideoFrame();
      } else if (videoDecoder->finished() && videoDecoder->isLooping()) {
        if (isPrimary) {
          state.video_clock = 0.0;
        }
        videoDecoder->seek(0);
      }
    }
  }
}

bool VideoPlayer::draw(Graphics &g, bool isPrimary) {
  if (!playingVideo.get()) {
    return false;
  }

  g.clear();

  if (isPrimary && !renderVideoInSim.get()) {
    return true;
  }

  auto &videoShader = shaderManager.get("video");

  g.shader(videoShader);
  videoShader.uniform("videoBlend", videoBlend.get());

  texY.bind(0);
  texU.bind(1);
  texV.bind(2);

  g.pushMatrix();
  g.translate(videoPose.get().pos());
  g.rotate(videoPose.get().quat());
  g.scale(videoScale.get());
  g.draw(sphereMesh);
  g.popMatrix();

  texY.unbind(0);
  texU.unbind(1);
  texV.unbind(2);

  return true;
}

bool VideoPlayer::loadVideoFile() {
  std::string path = dataPath + videoToLoad.get();

  if (videoDecoder != nullptr) {
    videoDecoder->stop();
  }

  videoDecoder = std::make_unique<VideoDecoder>();
  videoDecoder->enableAudio(false);

  if (!videoDecoder->load(path.c_str())) {
    std::cerr << "Error loading video file: " << path << std::endl;
    return false;
  }

  videoDecoder->start();

  texY.create2D(videoDecoder->lineSize()[0], videoDecoder->height(),
                Texture::RED, Texture::RED, Texture::UBYTE);
  texY.filter(Texture::LINEAR);
  texY.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE);
  texU.create2D(videoDecoder->lineSize()[1], videoDecoder->height() / 2,
                Texture::RED, Texture::RED, Texture::UBYTE);
  texU.filter(Texture::LINEAR);
  texU.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE);
  texV.create2D(videoDecoder->lineSize()[2], videoDecoder->height() / 2,
                Texture::RED, Texture::RED, Texture::UBYTE);
  texV.filter(Texture::LINEAR);
  texV.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE);

  return true;
}

void VideoPlayer::registerParams(ControlGUI &gui, PresetHandler &presets,
                                 PresetSequencer &seq, State &state) {
  gui << renderVideoInSim << playingVideo << videoBlend;
  gui << playAerialImages << playSF;
  gui << playBoardwalk << playCoral;
  gui << playOverfishing << playAcidification << playBoat;
  gui << videoPose << videoScale;

  presets << renderVideoInSim << playingVideo << videoBlend;

  seq << videoToLoad << videoBlend << playingVideo;
  seq << playBoardwalk << playCoral << playOverfishing << playAerialImages
      << playAcidification << playSF << playBoat;

  playAerialImages.registerChangeCallback([&](float value) {
    videoToLoad.set("aerialimages_+_sf (1080p).mp4");
    playingVideo.set(1.0);
  });
  playSF.registerChangeCallback([&](float value) {
    videoToLoad.set("sfmegamodel_v8 (1080p).mp4");
    playingVideo.set(1.0);
  });
  playBoardwalk.registerChangeCallback([&](float value) {
    videoToLoad.set("boardwalk_preview_v8 (1080p).mp4");
    playingVideo.set(1.0);
  });
  playCoral.registerChangeCallback([&](float value) {
    videoToLoad.set("Sensorium_Mono_Final_Comped_02_1080p.mov");
    playingVideo.set(1.0);
  });
  playOverfishing.registerChangeCallback([&](float value) {
    videoToLoad.set("overfishing_scene_comp_2 (1080p).mp4");
    playingVideo.set(1.0);
  });
  playAcidification.registerChangeCallback([&](float value) {
    videoToLoad.set("sensorium_preview (1080p).mp4");
    playingVideo.set(1.0);
  });
  playBoat.registerChangeCallback([&](float value) {
    videoToLoad.set("sensorium_boat_scene_12 (1080p).mp4");
    playingVideo.set(1.0);
  });
}