#include "OceanDataViewer.hpp"

#include <iterator>

#include "al/sphere/al_SphereUtils.hpp"

using namespace al;

void OceanDataViewer::init(const SearchPaths& paths)
{
  shaderManager.setSearchPaths(paths);
  shaderManager.setPollInterval(1);

  if (sphere::isSphereMachine()) {
    if (sphere::isRendererMachine()) {
      dataPath = "/data/Sensorium/";
    }
    else {
      dataPath = "/Volumes/Data/Sensorium/";
    }
  }
  else {
    dataPath = "data/";
  }
}

void OceanDataViewer::create(Lens& lens)
{
  shaderManager.add("space", "mono.vert", "space.frag");
  shaderManager.add("data", "stereo.vert", "data.frag");
  shaderManager.add("co2", "stereo.vert", "co2.frag");

  auto& spaceShader = shaderManager.get("space");
  spaceShader.begin();
  spaceShader.uniform("texSpace", 0);
  spaceShader.uniform("dataBlend", 1.f);
  spaceShader.end();

  auto& dataShader = shaderManager.get("data");
  dataShader.begin();
  dataShader.uniform("eye_sep", 0.f);
  dataShader.uniform("foc_len", lens.focalLength());
  dataShader.uniform("texEarth", 0);
  dataShader.uniform("texCloud", 1);
  dataShader.uniform("texData", 2);
  dataShader.uniform("validData", 0.f);
  dataShader.uniform("index", 0.f);
  dataShader.uniform("dataBlend", 1.f);
  dataShader.uniform("showCloud", 1.f);
  dataShader.end();

  auto& co2Shader = shaderManager.get("co2");
  co2Shader.begin();
  co2Shader.uniform("eye_sep", 0.f);
  co2Shader.uniform("foc_len", lens.focalLength());
  co2Shader.uniform("texEarth", 0);
  co2Shader.uniform("texY", 1);
  co2Shader.uniform("texU", 2);
  co2Shader.uniform("texV", 3);
  co2Shader.uniform("dataBlend", 1.f);
  co2Shader.end();

  addTexSphere(earthMesh, 2, 50, false);
  earthMesh.update();
  addTexSphere(spaceMesh, 50, 50, true);
  spaceMesh.update();

  // visible earth, nasa
  // earthImage = Image(dataPath + "blue_marble_brighter.jpg");
  auto earthImage = Image(dataPath + "16k.jpg");
  if (earthImage.array().size() == 0) {
    std::cerr << "failed to load earth image" << std::endl;
  }

  earthTex.create2D(earthImage.width(), earthImage.height());
  earthTex.filter(Texture::LINEAR);
  earthTex.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE);
  earthTex.submit(earthImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

  // paulbourke.net
  auto spaceImage = Image(dataPath + "Stellarium3.jpg");
  if (spaceImage.array().size() == 0) {
    std::cerr << "failed to load space image" << std::endl;
  }

  spaceTex.create2D(spaceImage.width(), spaceImage.height());
  spaceTex.filter(Texture::LINEAR);
  spaceTex.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE);
  spaceTex.submit(spaceImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

  auto cloudImage = Image(dataPath + "cloud/2.jpg");
  if (cloudImage.array().size() == 0) {
    std::cerr << "failed to load cloud image" << std::endl;
  }

  cloudTex.create2D(cloudImage.width(), cloudImage.height());
  cloudTex.filter(Texture::LINEAR);
  cloudTex.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE);
  cloudTex.submit(cloudImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

  loadAllData();
}

void OceanDataViewer::update(double dt, Nav& nav, State& state, bool isPrimary)
{
  shaderManager.update();

  if (isPrimary) {
    if (rotateGlobe.get()) {
      nav.nudgeR(0.03 * nav.pos().mag() * dt);
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
      nasaYear.setNoCalls(nasaYear.get() + 2.f * dt);
      chiYear.setNoCalls(chiYear.get() + 2.f * dt);

      if (nasaYear.get() == 2023) {
        nasaYear.setNoCalls(2012);
      }
      if (chiYear.get() == 2013) {
        chiYear.setNoCalls(2003);
      }
    }

    // Assign shared states for renderers
    state.global_pose.set(nav);
    state.nasa_index = (nasaYear - nasaYear.min()) / (data::years_nasa - 1);
    state.chi_index = (chiYear - chiYear.min()) / (data::years_chi - 1);

    if (show_co2.get()) {
      state.co2_clock += dt;
    }
  }
  else {
    nav.set(state.global_pose);
  }

  if (show_co2.get()) {
    MediaFrame* frame;
    if (videoDecoder != NULL) {
      frame = videoDecoder->getVideoFrame(state.co2_clock);
      if (frame) {
        texY.submit(frame->dataY.data());
        texU.submit(frame->dataU.data());
        texV.submit(frame->dataV.data());
        videoDecoder->gotVideoFrame();
      }
      else if (videoDecoder->finished() && videoDecoder->isLooping()) {
        if (isPrimary) {
          state.co2_clock = 0;
        }
        videoDecoder->seek(0);
      }
    }
  }
}

void OceanDataViewer::draw(Graphics& g, Nav& nav, State& state, Lens& lens)
{
  g.clear();

  g.depthTesting(true);

  // space
  auto& spaceShader = shaderManager.get("space");
  g.shader(spaceShader);
  spaceShader.uniform("dataBlend", dataBlend.get());

  g.pushMatrix();
  spaceTex.bind(0);
  g.translate(nav.pos());
  g.rotate(nav.quat());  // keeps space still
  g.draw(spaceMesh);
  spaceTex.unbind(0);
  g.popMatrix();

  // earth
  g.pushMatrix();
  earthTex.bind(0);

  // inside earth
  if (nav.pos().mag() < 2.01f) {
    g.scale(-1, 1, 1);
  }

  if (!show_co2.get()) {
    auto& dataShader = shaderManager.get("data");
    g.shader(dataShader);
    dataShader.uniform("eye_sep", lens.eyeSep() * g.eye() * 0.5f);
    dataShader.uniform("dataBlend", dataBlend.get());
    dataShader.uniform("showCloud", show_clouds.get());
    cloudTex.bind(1);
    if (dataIndex.get() >= 0) {
      float index = dataIndex.get() > 3 ? state.chi_index : state.nasa_index;
      dataShader.uniform("index", index);
      dataShader.uniform("validData", 1.f);
      oceanData[dataIndex.get()].bind(2);
      g.draw(earthMesh);
      oceanData[dataIndex.get()].unbind(2);
    }
    else {
      dataShader.uniform("validData", 0.f);
      g.draw(earthMesh);
    }
    cloudTex.unbind(1);
  }
  else {
    auto& co2Shader = shaderManager.get("co2");
    g.shader(co2Shader);
    co2Shader.uniform("eye_sep", lens.eyeSep() * g.eye() * 0.5f);
    co2Shader.uniform("dataBlend", dataBlend.get());
    texY.bind(1);
    texU.bind(2);
    texV.bind(3);
    g.draw(earthMesh);
    texY.unbind(1);
    texU.unbind(2);
    texV.unbind(3);
  }

  earthTex.unbind(0);
  g.popMatrix();
}

float OceanDataViewer::easeIn(float _value, float _target, float _speed)
{
  float d = _target - _value;
  float x = d * _speed;
  return x;
}

void OceanDataViewer::setNavTarget(float lat, float lon, float alt)
{
  navTarget.pos(
      Vec3d(-alt * cos(lat / 180.0 * M_PI) * sin(lon / 180.0 * M_PI),
            alt * sin(lat / 180.0 * M_PI),
            -alt * cos(lat / 180.0 * M_PI) * cos(lon / 180.0 * M_PI)));
  navTarget.faceToward(Vec3d(0), Vec3d(0, 1, 0));
  animateCam = true;
  anim_speed = anim_speed / 5;
}

void OceanDataViewer::loadAllData()
{
  loadDataNASA("nasa/sst/", 0);     // SST
  loadDataNASA("nasa/carbon/", 1);  // Carbon
  loadDataNASA("nasa/chl/", 2);     // Chlorophyll
  loadDataNASA("nasa/flh/", 3);     // Fluroscene Line Height

  loadDataCHI("chi/fish/fph_100_", "_impact.png",
              4);                                   // Over-Fishing
  loadDataCHI("ship/shipping_", "_impact.png", 5);  // Shipping
  loadDataCHI("oa/oa_", "_impact.png", 6);          // Ocean Acidification
  loadDataCHI("slr/slr_", "_impact.png", 7);        // Sea level rise

  // Co2 Video data
  loadDataCO2(
      "co2/"
      "SOS_TaggedCO2_4-1-2024a_co2_FF_quality_ScienceOnASphere_1024p30.mp4");

  std::cout << "Loaded Ocean data." << std::endl;
}

void OceanDataViewer::loadDataNASA(const std::string& pathPrefix,
                                   int stressorIndex)
{
  std::cout << "Loading stressorIndex: " << stressorIndex << std::endl;

  unsigned int imageWidth, imageHeight;
  std::vector<uint8_t> totalData;
  for (int d = 0; d < data::years_nasa; d++) {
    std::ostringstream ostr;
    ostr << dataPath << pathPrefix << d + 2012 << ".png";
    auto dataImage = Image(ostr.str());
    if (d == 0) {
      imageWidth = dataImage.width();
      imageHeight = dataImage.height();
      totalData.reserve(4 * imageWidth * imageHeight * data::years_nasa);
    }
    else if (imageWidth != dataImage.width() ||
             imageHeight != dataImage.height()) {
      std::cerr << "Image dimensions do not match" << std::endl;
      return;
    }

    totalData.insert(totalData.end(),
                     std::make_move_iterator(dataImage.array().begin()),
                     std::make_move_iterator(dataImage.array().end()));
  }

  Texture& target = oceanData[stressorIndex];
  target.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);
  target.filter(Texture::LINEAR);
  target.create3D(imageWidth, imageHeight, data::years_nasa);
  target.submit(totalData.data(), GL_RGBA, GL_UNSIGNED_BYTE);
}

void OceanDataViewer::loadDataCHI(const std::string& prefix,
                                  const std::string& postfix, int stressorIndex)
{
  std::cout << "Loading stressorIndex: " << stressorIndex << std::endl;

  unsigned int imageWidth, imageHeight;
  std::vector<uint8_t> totalData;
  for (int d = 0; d < data::years_chi; d++) {
    std::ostringstream ostr;
    ostr << dataPath << prefix << d + 2003 << postfix;
    auto dataImage = Image(ostr.str());
    if (d == 0) {
      imageWidth = dataImage.width();
      imageHeight = dataImage.height();
      totalData.reserve(4 * imageWidth * imageHeight * data::years_chi);
    }
    else if (imageWidth != dataImage.width() ||
             imageHeight != dataImage.height()) {
      std::cerr << "Image dimensions do not match" << std::endl;
      return;
    }

    totalData.insert(totalData.end(),
                     std::make_move_iterator(dataImage.array().begin()),
                     std::make_move_iterator(dataImage.array().end()));
  }

  Texture& target = oceanData[stressorIndex];
  target.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);
  target.filter(Texture::LINEAR);
  target.create3D(imageWidth, imageHeight, data::years_chi);
  target.submit(totalData.data(), GL_RGBA, GL_UNSIGNED_BYTE);
}

void OceanDataViewer::loadDataCO2(const std::string& videoFile)
{
  std::string path = dataPath + videoFile;

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

void OceanDataViewer::registerParams(ControlGUI& gui, PresetHandler& presets,
                                     PresetSequencer& seq, State& state,
                                     Nav& nav)
{
  gui << cycleYears << nasaYear << chiYear;
  gui << show_sst << show_carbon << show_chl << show_flh << show_fish
      << show_ship << show_oa << show_slr << show_co2;
  gui << dataBlend << show_clouds;
  gui << rotateGlobe << faceTo << animateCam;

  presets << cycleYears;
  presets << dataIndex << show_co2;
  presets << dataBlend << show_clouds;
  presets << rotateGlobe << faceTo << animateCam;

  seq << cycleYears;
  seq << show_sst << show_carbon << show_chl << show_flh << show_fish
      << show_ship << show_oa << show_slr << show_co2;
  seq << dataBlend << show_clouds;
  seq << geoCoord << rotateGlobe << faceTo;

  geoCoord.registerChangeCallback(
      [&](Vec3f v) { setNavTarget(v.x, v.y, v.z); });

  cycleYears.registerChangeCallback([&](float value) {
    if (value > 0) {
      nasaYear.setNoCalls(2012);
      chiYear.setNoCalls(2003);
    }
  });

  show_sst.registerChangeCallback([&](float value) {
    resetIndex();
    if (value > 0) {
      dataIndex.set(0);
    }
  });

  show_carbon.registerChangeCallback([&](float value) {
    resetIndex();
    if (value > 0) {
      dataIndex.set(1);
    }
  });

  show_chl.registerChangeCallback([&](float value) {
    resetIndex();
    if (value > 0) {
      dataIndex.set(2);
    }
  });

  show_flh.registerChangeCallback([&](float value) {
    resetIndex();
    if (value > 0) {
      dataIndex.set(3);
    }
  });

  show_fish.registerChangeCallback([&](float value) {
    resetIndex();
    if (value > 0) {
      dataIndex.set(4);
    }
  });

  show_ship.registerChangeCallback([&](float value) {
    resetIndex();
    if (value > 0) {
      dataIndex.set(5);
    }
  });

  show_oa.registerChangeCallback([&](float value) {
    resetIndex();
    if (value > 0) {
      dataIndex.set(6);
    }
  });

  show_slr.registerChangeCallback([&](float value) {
    resetIndex();
    if (value > 0) {
      dataIndex.set(7);
    }
  });
}

void OceanDataViewer::resetIndex()
{
  dataIndex.set(-1);
  show_sst.setNoCalls(0.f);
  show_carbon.setNoCalls(0.f);
  show_chl.setNoCalls(0.f);
  show_flh.setNoCalls(0.f);
  show_fish.setNoCalls(0.f);
  show_ship.setNoCalls(0.f);
  show_oa.setNoCalls(0.f);
  show_slr.setNoCalls(0.f);
}