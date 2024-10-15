#include "OceanDataViewer.hpp"
#include "al/sphere/al_SphereUtils.hpp"

using namespace al;

void OceanDataViewer::init(const SearchPaths &paths) {
  shaderManager.setSearchPaths(paths);
  shaderManager.setPollInterval(1);

  if (sphere::isSphereMachine()) {
    if (sphere::isRendererMachine()) {
      dataPath = "/data/Sensorium/";
    } else {
      dataPath = "/Volumes/Data/Sensorium/";
    }
  } else {
    dataPath = "data/";
  }
}

void OceanDataViewer::create() {
  shaderManager.add("sphere", "common.vert", "sphere.frag");
  shaderManager.add("data", "common.vert", "data.frag");
  shaderManager.add("video", "common.vert", "video.frag");

  addTexSphere(earthMesh, 2, 50, false);
  earthMesh.update();

  // visible earth, nasa
  // earthImage = Image(dataPath + "blue_marble_brighter.jpg");
  auto earthImage = Image(dataPath + "16k.jpg");
  if (earthImage.array().size() == 0) {
    std::cerr << "failed to load sphere image" << std::endl;
  }

  earthTex.create2D(earthImage.width(), earthImage.height());
  earthTex.filter(Texture::LINEAR);
  earthTex.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE);
  earthTex.submit(earthImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

  addTexSphere(spaceMesh, 50, 50, true);
  spaceMesh.update();

  // paulbourke.net
  auto spaceImage = Image(dataPath + "Stellarium3.jpg");
  if (spaceImage.array().size() == 0) {
    std::cerr << "failed to load background image" << std::endl;
  }

  spaceTex.create2D(spaceImage.width(), spaceImage.height());
  spaceTex.filter(Texture::LINEAR);
  spaceTex.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE);
  spaceTex.submit(spaceImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

  loadAllData();
}

void OceanDataViewer::update(double dt, Nav &nav, State &state,
                             bool isPrimary) {
  shaderManager.update();

  if (isPrimary) {
    if (rotateGlobe.get()) {
      nav.moveR(0.003 * dt);
    }

    if (faceTo.get()) {
      // examplary point that you want to see
      Vec3f targetPoint = Vec3f(0, 0, 0);
      nav.faceToward(targetPoint, Vec3f(0, 1, 0), 0.1);
    }

    if (animateCam.get()) {
      // easing on both in and out
      // easeIn(value, target, speed)
      anim_speed += easeIn(anim_speed, anim_target_speed, anim_target_speed);

      nav.set(nav.lerp(navTarget, anim_speed));

      // end animation when we get close enough
      if ((nav.pos() - navTarget.pos()).mag() <= .1) {
        anim_speed = 0;
        animateCam = 0;
      }
    }

    if (cycleYears.get()) {
      nasaYear.setNoCalls(nasaYear.get() + 3.f * dt);
      chiYear.setNoCalls(chiYear.get() + 3.f * dt);

      if (nasaYear.get() == 2023) {
        nasaYear.setNoCalls(2013);
      }
      if (chiYear.get() == 2013) {
        chiYear.setNoCalls(2003);
      }
    }

    // Assign shared states for renderers
    state.global_pose.set(nav);
    state.nasa_year = nasaYear;
    state.chi_year = chiYear;

    state.swtch[0] = s_sst;
    state.swtch[1] = s_carbon;
    state.swtch[2] = s_chl;
    state.swtch[3] = s_flh;
    state.swtch[4] = s_fish;
    state.swtch[5] = s_shp;
    state.swtch[6] = s_oa;
    // state.swtch[6] = s_plastics;
    // state.swtch[7] = s_resiliency;
    state.swtch[7] = s_slr;
    state.cloud_swtch[0] = s_cloud;
    state.cloud_swtch[1] = s_cloud_eu;
    state.cloud_swtch[2] = s_cloud_storm;
    state.co2_swtch = s_co2;
    state.co2Playing = s_co2_vid;

    if (state.co2Playing) {
      state.co2_clock += dt;
    }
  } else {
    nav.set(state.global_pose);
  }

  if (state.co2Playing) {
    MediaFrame *frame;
    if (videoDecoder != NULL) {
      frame = videoDecoder->getVideoFrame(state.co2_clock);
      if (frame) {
        texY.submit(frame->dataY.data());
        texU.submit(frame->dataU.data());
        texV.submit(frame->dataV.data());
        videoDecoder->gotVideoFrame();
      } else if (videoDecoder->finished() && videoDecoder->isLooping()) {
        state.co2_clock = 0;
        videoDecoder->seek(0);
      }
    }
  }
}

void OceanDataViewer::draw(Graphics &g, Nav &nav, State &state) {
  if (state.videoPlaying) {
    return;
  }

  g.clear(0);

  gl::depthFunc(GL_LEQUAL);
  g.culling(false);
  g.depthTesting(true);
  g.blending(true);
  g.blendTrans();

  // auto &shaderSphere = shaderManager.get("sphere");
  // g.shader(shaderSphere);
  // shaderSphere.uniform("tex0", 0);
  g.texture();

  // sky
  g.pushMatrix();
  spaceTex.bind(0);
  g.translate(nav.pos());
  g.rotate(nav.quat()); // keeps sky still
  g.draw(spaceMesh);
  spaceTex.unbind(0);
  g.popMatrix();

  g.pushMatrix();
  // inside sphere
  if (nav.pos().mag() < 2.01f) {
    g.scale(-1, 1, 1);
  }

  // sphere (earth)
  earthTex.bind(0);
  g.draw(earthMesh); // only needed if we go inside the earth
  earthTex.unbind(0);

  //  co2 frames
  if (state.co2Playing) {
    auto &shaderVideo = shaderManager.get("video");
    g.shader(shaderVideo);
    texY.bind(0);
    texU.bind(1);
    texV.bind(2);
    shaderVideo.uniform("texY", 0);
    shaderVideo.uniform("texU", 1);
    shaderVideo.uniform("texV", 2);
    g.draw(earthMesh);
    texY.unbind(0);
    texU.unbind(1);
    texV.unbind(2);
  }

  auto &shaderDataset = shaderManager.get("data");
  g.shader(shaderDataset);
  int bind_index = 0;
  for (int j = 0; j < data::num_stressors; j++) {
    if (state.swtch[j]) {
      int offset = 2013;
      float year = state.nasa_year;
      if (j >= 4) {
        offset = 2003;
        year = state.chi_year;
      }
      pic[(int)year - offset][j].bind(bind_index);
      std::string texIndex = "tex" + std::to_string(bind_index);
      shaderDataset.uniform(texIndex.c_str(), bind_index);
      if (++bind_index > 3) {
        break;
      }
    }
  }
  shaderDataset.uniform("dataNum", (float)bind_index);
  g.draw(earthMesh);

  // draw cloud
  // for (int j = 0; j < num_cloud; j++) {
  //   if (state.cloud_swtch[j]) {
  //     cloud[j].bind();
  //     g.draw(earthMesh);
  //   }
  // }

  g.popMatrix();
}

float OceanDataViewer::easeIn(float _value, float _target, float _speed) {
  float d = _target - _value;
  float x = d * _speed;
  return x;
}

void OceanDataViewer::setNavTarget(float lat, float lon, float alt) {
  navTarget.pos(
      Vec3d(-alt * cos(lat / 180.0 * M_PI) * sin(lon / 180.0 * M_PI),
            alt * sin(lat / 180.0 * M_PI),
            -alt * cos(lat / 180.0 * M_PI) * cos(lon / 180.0 * M_PI)));
  navTarget.faceToward(Vec3d(0), Vec3d(0, 1, 0));
  animateCam = true;
  anim_speed = anim_speed / 5;
}

void OceanDataViewer::loadAllData() {
  loadDataset("nasa/sst/", 0);    // SST
  loadDataset("nasa/carbon/", 1); // Carbon
  loadDataset("nasa/chl/", 2);    // Chlorophyll
  loadDataset("nasa/flh/", 3);    // Fluroscene Line Height

  loadChiDataset("chi/fish/fph_100_", "_impact.png",
                 4);                                  // Fishing demersal low
  loadChiDataset("ship/shipping_", "_impact.png", 5); // Shipping
  loadChiDataset("oa/oa_", "_impact.png", 6);         // Ocean Acidification
  loadChiDataset("slr/slr_", "_impact.png", 7);       // Sea level rise

  // Co2 Video data
  loadCO2Dataset(
      "co2/"
      "SOS_TaggedCO2_4-1-2024a_co2_FF_quality_ScienceOnASphere_1024p30.mp4");

  std::cout << "Loaded Ocean data." << std::endl;
}

void OceanDataViewer::loadDataset(const std::string &pathPrefix,
                                  int stressorIndex) {
  Image oceanData;
  std::cout << "Start loading stressorIndex: " << stressorIndex << std::endl;
  for (int d = 0; d < data::years_nasa; d++) {
    std::ostringstream ostr;
    ostr << dataPath << pathPrefix << d + 2012 << ".png";
    oceanData = Image(ostr.str());

    pic[d][stressorIndex].create2D(oceanData.width(), oceanData.height());
    pic[d][stressorIndex].submit(oceanData.array().data(), GL_RGBA,
                                 GL_UNSIGNED_BYTE);
    pic[d][stressorIndex].wrap(Texture::REPEAT);
    pic[d][stressorIndex].filter(Texture::LINEAR);
    loaded[d][stressorIndex] = true;
  }
}

void OceanDataViewer::loadChiDataset(const std::string &prefix,
                                     const std::string &postfix,
                                     int stressorIndex) {
  Image oceanData;
  std::cout << "Start loading stressorIndex: " << stressorIndex << std::endl;
  for (int d = 0; d < data::years_chi; d++) {
    std::ostringstream ostr;
    ostr << dataPath << prefix << d + 2003 << postfix;

    oceanData = Image(ostr.str());

    pic[d][stressorIndex].create2D(oceanData.width(), oceanData.height());
    pic[d][stressorIndex].submit(oceanData.array().data(), GL_RGBA,
                                 GL_UNSIGNED_BYTE);
    pic[d][stressorIndex].wrap(Texture::REPEAT);
    pic[d][stressorIndex].filter(Texture::LINEAR);
    loaded[d][stressorIndex] = true;
  }
}

void OceanDataViewer::loadCO2Dataset(const std::string &video) {
  std::string path = dataPath + video;

  videoDecoder = std::make_unique<VideoDecoder>();
  videoDecoder->enableAudio(false);
  videoDecoder->loop(true);

  if (!videoDecoder->load(path.c_str())) {
    std::cerr << "Error loading video file: " << path << std::endl;
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
}

void OceanDataViewer::registerParams(ControlGUI &gui, PresetHandler &presets,
                                     PresetSequencer &seq, State &state,
                                     Nav &nav) {
  gui << blend;
  gui << nasaYear << chiYear;
  gui << cycleYears;
  gui << rotateGlobe << faceTo << animateCam;
  gui << s_carbon << s_slr << s_chl << s_flh << s_oa << s_sst;
  gui << s_fish << s_shp << s_plastics << s_resiliency;
  gui << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid;

  presets << nasaYear << blend;
  presets << cycleYears << rotateGlobe;
  presets << s_carbon << s_slr << s_chl << s_flh << s_oa << s_sst;
  presets << s_fish << s_shp << s_plastics << s_resiliency;
  presets << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid;

  seq << geoCoord << cycleYears << rotateGlobe << faceTo << blend;
  seq << s_carbon << s_slr << s_shp << s_chl << s_flh << s_oa << s_sst;
  seq << s_fish << s_plastics << s_resiliency;
  seq << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid;

  geoCoord.registerChangeCallback(
      [&](Vec3f v) { setNavTarget(v.x, v.y, v.z); });

  cycleYears.registerChangeCallback([&](float value) {
    if (value > 0) {
      nasaYear.setNoCalls(2013);
      chiYear.setNoCalls(2003);
    }
  });
}