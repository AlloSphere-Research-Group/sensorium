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

  addTexSphere(skyMesh, 50, 50, true);
  skyMesh.update();

  addTexSphere(sphereMesh, 2, 50, false);
  sphereMesh.generateNormals();
  sphereMesh.update();

  // visible earth, nasa
  // sphereImage = Image(dataPath + "blue_marble_brighter.jpg");
  auto sphereImage = Image(dataPath + "16k.jpg");
  if (sphereImage.array().size() == 0) {
    std::cerr << "failed to load sphere image" << std::endl;
  }

  sphereTex.create2D(sphereImage.width(), sphereImage.height());
  sphereTex.filter(Texture::LINEAR);
  sphereTex.submit(sphereImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

  // paulbourke.net
  auto skyImage = Image(dataPath + "Stellarium3.jpg");
  if (skyImage.array().size() == 0) {
    std::cerr << "failed to load background image" << std::endl;
  }

  skyTex.create2D(skyImage.width(), skyImage.height());
  skyTex.filter(Texture::LINEAR);
  skyTex.submit(skyImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

  loadAllData();
}

void OceanDataViewer::update(double dt, Nav &nav, State &state,
                             bool isPrimary) {
  shaderManager.update();

  if (isPrimary) {
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

    Vec3d pos = nav.pos();
    pos.normalize();

    // Molph year time
    if (state.molph) {
      year = year + 3 * dt;
      chiyear = chiyear + 3 * dt;

      if (year == 2023) {
        year = 2013;
        // state.molph = false;
        s_years.set(1);
      }
      if (chiyear == 2013) {
        chiyear = 2003;
        // state.molph = false;
        s_years.set(1);
      }
    }
    // Molph frame time
    if (state.molphFrame) {
      if (year == 0) {
        year = 0;
        state.molphFrame = false;
        // s_frames.set(1);
      }
    }
    if (s_nav) {
      nav.moveR(0.003 * 0.25);
    }

    // Assign shared states for renderers
    state.global_pose.set(nav);
    state.year = year;
    state.chiyear = chiyear;
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

  camPose.setNoCalls(nav);

  if (state.co2Playing) {
    MediaFrame *frame;
    if (videoDecoder != NULL) {
      frame = videoDecoder->getVideoFrame(state.co2_clock);
      if (frame) {
        tex0Y.submit(frame->dataY.data());
        tex0U.submit(frame->dataU.data());
        tex0V.submit(frame->dataV.data());
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
  skyTex.bind(0);
  g.translate(nav.pos());
  g.rotate(nav.quat()); // keeps sky still
  g.draw(skyMesh);
  skyTex.unbind(0);
  g.popMatrix();

  g.pushMatrix();
  // inside sphere
  if (nav.pos().mag() < 2.01f) {
    g.scale(-1, 1, 1);
  }

  // sphere (earth)
  sphereTex.bind(0);
  g.draw(sphereMesh); // only needed if we go inside the earth
  sphereTex.unbind(0);

  //  co2 frames
  if (state.co2Playing) {
    auto &shaderVideo = shaderManager.get("video");
    g.shader(shaderVideo);
    tex0Y.bind(0);
    tex0U.bind(1);
    tex0V.bind(2);
    shaderVideo.uniform("texY", 0);
    shaderVideo.uniform("texU", 1);
    shaderVideo.uniform("texV", 2);
    g.draw(sphereMesh);
    tex0Y.unbind(0);
    tex0U.unbind(1);
    tex0V.unbind(2);
  }

  auto &shaderDataset = shaderManager.get("data");
  g.shader(shaderDataset);
  int bind_index = 0;
  for (int j = 0; j < stressors; j++) {
    if (state.swtch[j]) {
      int offset = 2013;
      float year = state.year;
      if (j >= 4) {
        offset = 2003;
        year = state.chiyear;
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
  g.draw(sphereMesh);

  // draw cloud
  // for (int j = 0; j < num_cloud; j++) {
  //   if (state.cloud_swtch[j]) {
  //     cloud[j].bind();
  //     g.draw(sphereMesh);
  //   }
  // }

  g.popMatrix();
}

float OceanDataViewer::easeIn(float _value, float _target, float _speed) {
  float d = _target - _value;
  float x = d * _speed;
  return x;
}

void OceanDataViewer::setGeoTarget(float la, float lo, float r) {
  navTarget.pos(Vec3d(-r * cos(la / 180.0 * M_PI) * sin(lo / 180.0 * M_PI),
                      r * sin(la / 180.0 * M_PI),
                      -r * cos(la / 180.0 * M_PI) * cos(lo / 180.0 * M_PI)));

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
  for (int d = 0; d < years; d++) {
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
  for (int d = 0; d < chiyears; d++) {
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

  tex0Y.create2D(videoDecoder->lineSize()[0], videoDecoder->height(),
                 Texture::RED, Texture::RED, Texture::UBYTE);
  tex0U.create2D(videoDecoder->lineSize()[1], videoDecoder->height() / 2,
                 Texture::RED, Texture::RED, Texture::UBYTE);
  tex0V.create2D(videoDecoder->lineSize()[2], videoDecoder->height() / 2,
                 Texture::RED, Texture::RED, Texture::UBYTE);
}

void OceanDataViewer::registerParams(ControlGUI &gui, PresetHandler &presets,
                                     PresetSequencer &seq, State &state,
                                     Nav &nav) {
  gui << blend;           // << year << gain;
  gui << year << chiyear; // << frame;
  gui << s_years;         // << s_frames;
  gui << s_nav << faceTo << animateCam;
  gui << s_carbon << s_slr << s_chl << s_flh << s_oa << s_sst;
  gui << s_fish << s_shp << s_plastics << s_resiliency;
  gui << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid;

  presets << year << camPose << blend;
  presets << s_years << s_nav;
  presets << s_carbon << s_slr << s_chl << s_flh << s_oa << s_sst;
  presets << s_fish << s_shp << s_plastics << s_resiliency;
  presets << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid;

  seq << llr << s_years << s_nav << camPose << faceTo << blend;
  seq << s_carbon << s_slr << s_shp << s_chl << s_flh << s_oa << s_sst;
  seq << s_fish << s_plastics << s_resiliency;
  seq << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid;

  camPose.registerChangeCallback([&](Pose p) { nav.set(p); });
  llr.registerChangeCallback([&](Vec3f v) { setGeoTarget(v.x, v.y, v.z); });

  s_years.registerChangeCallback([&](int value) {
    if (value) {
      state.molph = true; //! state.molph;
      year = 2012;
      chiyear = 2003;
      // s_years.set(0);
    } else {
      state.molph = false;
    }
  });
}