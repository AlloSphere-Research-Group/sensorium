
#ifndef OCEAN_DATA_VIEWER_HPP
#define OCEAN_DATA_VIEWER_HPP

#include <iostream>
#include <string.h>

#include "al/graphics/al_Image.hpp"
#include "al/graphics/al_Light.hpp"
#include "al/graphics/al_Shapes.hpp"

#include "Gamma/Filter.h"
#include "Gamma/Noise.h"
#include "Gamma/Oscillator.h"
#include "al/sound/al_Reverb.hpp"

// #include "al/io/al_CSVReader.hpp"
#include "al/math/al_Random.hpp"

#include "al_ext/video/al_VideoDecoder.hpp"

namespace al {
struct OceanDataViewer {

  struct GeoLoc {
    float lat;
    float lon;
    float radius;
  };

  Parameter lat{"lat", "Nav", 0.0, -90.0, 90.0};
  Parameter lon{"lon", "Nav", 0.0, -180.0, 180.0};
  Parameter radius{"radius", "Nav", 5.0, 0.0, 50.0};
  ParameterVec3 llr{"llr", "Nav"};
  ParameterPose camPose{"camPose", "Nav"};
  ParameterBool animateCam{"animateCam", "Nav", 0.0};
  ParameterBool faceTo{"Face_Center", "Nav", 1.0};

  Parameter lux{"Light", 0.6, 0, 2.5};
  Parameter blend{"blend", "", 1.0, 0.0, 1.0};

  Parameter year{"Year", 2013, 2013, 2023};
  Parameter chiyear{"CHIYear", 2003, 2003, 2013};

  // Parameter frame{"CO2 Year", 0, 0, frames - 1};

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
  // ParameterBool s_chiyears{"2003_2013", "", 0.0};
  // ParameterBool s_frames{"CO2 Play", "", 0.0};

  // video textures
  Texture tex0Y, tex0U, tex0V;
  VideoDecoder *videoDecoder{NULL};

  // bool faceTo = true;
  // bool animateCam = false;
  Pose navTarget;
  float anim_speed = 0.0;
  float anim_target_speed = 0.002;

  GeoLoc sourceGeoLoc, targetGeoLoc;
  // Image oceanData[years][stressors];
  // Image cloudData[num_cloud];
  double morphProgress{0.0};
  double morphDuration{5.0};
  const double defaultMorph{5.0};
  float hoverHeight{10.f};
  double hoverDuration{2.5};
  const double defaultHover{2.5};
  float earth_radius = 5;
  float point_dist = 1.01 * earth_radius;
  Color data_color;

  Light light;

  VAOMesh skyMesh, sphereMesh;
  Image skyImage, sphereImage;
  Texture skyTex, sphereTex;

  static const int sstCount{261};
  Texture sst[sstCount];

  Texture pic[years][stressors];
  Texture chipic[chiyears][stressors];

  bool loaded[years][stressors];
  Texture cloud[num_cloud];
  VAOMesh co2_mesh[num_county];
  // VAOMesh cloud[num_cloud];
  // Mesh co2_mesh[num_county];
  float nation_lat[num_county], nation_lon[num_county];
  float morph_year;

  gam::Buzz<> wave;
  gam::Sine<> env;
  gam::NoiseWhite<> mNoise;
  gam::Biquad<> mFilter{};
  Reverb<float> reverb;
  // osc::Recv server;

  ShaderProgram shaderDataset;

  std::string dataPath;

  string slurp(string fileName) {
    fstream file(fileName);
    string returnValue = "";
    while (file.good()) {
      string line;
      getline(file, line);
      returnValue += line + "\n";
    }
    return returnValue;
  }

  void onInit() {}

  void onCreate() {

    addSphereWithTexcoords(skyMesh, 50, 50, true);
    skyMesh.update();

    addSphereWithTexcoords(sphereMesh, 2, 50, false);
    sphereMesh.update();

    if (sphere::isSphereMachine()) {
      if (sphere::isRendererMachine()) {
        dataPath = "/data/Sensorium/";
      } else {
        dataPath = "/Volumes/Data/Sensorium/";
      }
    } else {
      // dataPath = "C:/Users/kenny/data/sensorium/";
      dataPath = "data/";
    }

    // visible earth, nasa
    // sphereImage = Image(dataPath + "blue_marble_brighter.jpg");
    sphereImage = Image(dataPath + "16k.jpg");
    if (sphereImage.array().size() == 0) {
      std::cerr << "failed to load sphere image" << std::endl;
    }

    // paulbourke.net
    skyImage = Image(dataPath + "Stellarium3.jpg");
    if (skyImage.array().size() == 0) {
      std::cerr << "failed to load background image" << std::endl;
    }

    sphereTex.create2D(sphereImage.width(), sphereImage.height());
    sphereTex.filter(Texture::LINEAR);
    sphereTex.submit(sphereImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

    skyTex.create2D(skyImage.width(), skyImage.height());
    skyTex.filter(Texture::LINEAR);
    skyTex.submit(skyImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

    // audio
    // filter
    mFilter.zero();
    mFilter.res(1);
    mFilter.type(gam::LOW_PASS);
    reverb.bandwidth(0.6f); // Low-pass amount on input, in [0,1]
    reverb.damping(0.5f);   // High-frequency damping, in [0,1]
    reverb.decay(0.6f);     // Tail decay factor, in [0,1]
  }

  void onAnimate(double dt, Nav &nav, State &state, bool isPrimary) {

    if (isPrimary) {
      if (state.co2Playing) {
        state.co2_clock += dt;
      }
    }

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

    if (isPrimary) {
      Vec3f point_you_want_to_see =
          Vec3f(0, 0, 0); // examplary point that you want to see
      if (faceTo)
        nav.faceToward(point_you_want_to_see, Vec3f(0, 1, 0), 0.1);

      if (animateCam.get()) {
        // easing on both in and out
        // EaseIn(value, target, speed)
        // if (animateCam == 1) {
        anim_speed += EaseIn(anim_speed, anim_target_speed, anim_target_speed);
        // } else {
        //   anim_speed += EaseIn(anim_speed, 0.05, 0.05);
        // }
        nav.set(nav.lerp(navTarget, anim_speed));

        // end animation when we get close enough
        if ((nav.pos() - navTarget.pos()).mag() <= .1) {
          anim_speed = 0;
          animateCam = 0;
        }
      }

      Vec3d pos = nav.pos();
      radius.setNoCalls(pos.mag());
      pos.normalize();
      lat.setNoCalls(asin(pos.y) * 180.0 / M_PI);
      lon.setNoCalls(atan2(-pos.x, -pos.z) * 180.0 / M_PI);

      // Set light position
      light.pos(nav.pos().x, nav.pos().y, nav.pos().z);
      Light::globalAmbient({lux, lux, lux});
      state.lux = lux;
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
      //  audio
      mFilter.freq(30 * (1 + 10 / (radius + 3)) * (year - 2000));
      // mFilter.res();
      mFilter.type(gam::LOW_PASS);
      reverb.decay(0.6f + 0.3 / (radius + 1)); // Tail decay factor, in [0,1]

      // Assign shared states for renderers
      state.global_pose.set(nav);
      state.year = year;
      state.chiyear = chiyear;
      state.radius = radius;
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
    } // prim end
    else // renderer
    {
      nav.set(state.global_pose);
      // light.pos(nav.pos().x, nav.pos().y, nav.pos().z);
      // Light::globalAmbient({state.lux, state.lux, state.lux});
    }
  }

  void onDraw(Graphics &g, Nav &nav, State &state) {
    g.clear(0);

    g.shader(shaderDataset);
    shaderDataset.uniform("blend", blend);
    shaderDataset.uniform("tex0", 0);
    shaderDataset.uniform("isVideo", 0);
    shaderDataset.uniform("mapFunction", -1);

    gl::depthFunc(GL_LEQUAL);
    g.culling(false);
    // g.lighting(true);
    g.light(light);
    // g.texture();
    g.depthTesting(true);
    g.blending(true);
    g.blendTrans();

    // sky
    g.pushMatrix();
    skyTex.bind();
    g.translate(nav.pos());
    g.rotate(nav.quat()); // keeps sky still
    g.draw(skyMesh);
    skyTex.unbind();

    g.popMatrix();

    // sphere (earth)
    sphereTex.bind();

    // inside sphere
    if (nav.pos().mag() < 2.01f) {
      g.pushMatrix();
      g.scale(-1, 1, 1);
      g.draw(sphereMesh); // only needed if we go inside the earth
    } else {              // outside
      g.draw(sphereMesh);
    }
    sphereTex.unbind();

    // Draw data
    // g.lighting(false);
    // g.blending(true);
    // g.blendTrans();

    for (int j = 0; j < stressors; j++) {
      if (state.swtch[j]) {
        int offset = 2013;
        float year = state.year;
        if (j >= 4) {
          offset = 2003;
          year = state.chiyear;
        }
        shaderDataset.uniform("mapFunction", j);
        pic[(int)year - offset][j].bind();
        g.draw(sphereMesh); // only needed if we go inside the earth
      }
    }

    // Draw CO2 frames
    if (state.co2Playing) {
      shaderDataset.uniform("texY", 0);
      shaderDataset.uniform("texU", 1);
      shaderDataset.uniform("texV", 2);
      shaderDataset.uniform("isVideo", 1);
      shaderDataset.uniform("mapFunction", 0);

      tex0Y.bind(0);
      tex0U.bind(1);
      tex0V.bind(2);
      g.draw(sphereMesh); // only needed if we go inside the earth
    }

    // draw cloud
    for (int j = 0; j < num_cloud; j++) {
      if (state.cloud_swtch[j]) {
        cloud[j].bind();
        g.draw(sphereMesh);
      }
    }

    if (nav.pos().mag() < 2.01f) {
      g.popMatrix(); // pop from push above
    }
  }

  void onSound(AudioIOData &io) {
    while (io()) {
      // wave.freq( (2 + mNoise()) * (1+10/radius) * (year-2000) ) ;
      env.freq(0.003 * (year - 1980));
      float wave_out = mFilter(mNoise() * gain.get() * env());
      float wet1, wet2;
      reverb(wave_out, wet1, wet2);
      io.out(0) = wet1;
      io.out(1) = wet2;
    }
  }

  float EaseIn(float _value, float _target, float _speed) {
    float d = _target - _value;
    float x = d * _speed;
    return x;
  }

  void setGeoTarget(float la, float lo, float r = 3.2, float duration = 4.0) {
    navTarget.pos(Vec3d(-r * cos(la / 180.0 * M_PI) * sin(lo / 180.0 * M_PI),
                        r * sin(la / 180.0 * M_PI),
                        -r * cos(la / 180.0 * M_PI) * cos(lo / 180.0 * M_PI)));

    navTarget.faceToward(Vec3d(0), Vec3d(0, 1, 0));
    animateCam = true;
    anim_speed = anim_speed / 5;
  }

  void registerParams(ControlGUI *gui, PresetHandler &presets,
                      PresetSequencer &seq, SequenceRecorder &rec, Nav &nav,
                      State &state) {
    std::string displayText = "AlloOcean. Ocean stressor (2012-2023)";
    // "AlloOcean. Ocean stressor from Cumulative Human Impacts (2003-2013)"
    *gui << lat << lon << radius << blend; // << lux << year << gain;
    *gui << year << chiyear;               // << frame;
    *gui << s_years;                       // << s_frames;
    *gui << s_nav << faceTo << animateCam;
    *gui << s_carbon << s_slr << s_chl << s_flh << s_oa << s_sst;
    *gui << s_fish << s_shp << s_plastics << s_resiliency;
    *gui << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid << lux;
    // *gui << s_cf_dd << a_f // currently we don't have this data
    // *gui << s_ci << s_oc << s_carbon;

    // *gui << lat << lon << radius << lux << year << trans << gain;

    presets << year << camPose << blend;
    presets << s_years << s_nav;
    presets << s_carbon << s_slr << s_chl << s_flh << s_oa << s_sst;
    presets << s_fish << s_shp << s_plastics << s_resiliency;
    presets << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid
            << lux;

    seq << llr << s_years << s_nav << camPose << faceTo << blend;
    seq << s_carbon << s_slr << s_shp << s_chl << s_flh << s_oa << s_sst;
    seq << s_fish << s_plastics << s_resiliency;
    seq << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << s_co2_vid << lux;

    camPose.registerChangeCallback([&](Pose p) { nav.set(p); });
    llr.registerChangeCallback([&](Vec3f v) { setGeoTarget(v.x, v.y, v.z); });

    lat.registerChangeCallback(
        [&](float value) { setGeoTarget(value, lon, radius); });

    lon.registerChangeCallback(
        [&](float value) { setGeoTarget(lat, value, radius); });

    radius.registerChangeCallback(
        [&](float value) { setGeoTarget(lat, lon, value); });

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

    // s_frames.registerChangeCallback([&](int value) {
    //   if (value) {
    //     state.molphFrame = true; //! state.molph;
    //     // frame = 0;
    //     // s_years.set(0);
    //   } else {
    //     state.molphFrame = false;
    //   }
    // });
  }

  void loadDataset(std::string pathPrefix, int stressorIndex) {
    Image oceanData;
    std::cout << "Start loading stressorIndex: " << stressorIndex << std::endl;
    for (int d = 0; d < years; d++) {
      ostringstream ostr;
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

  void loadChiDataset(std::string prefix, std::string postfix,
                      int stressorIndex) {
    Image oceanData;
    std::cout << "Start loading stressorIndex: " << stressorIndex << std::endl;
    for (int d = 0; d < chiyears; d++) {
      ostringstream ostr;
      ostr << dataPath << prefix << d + 2003 << postfix;
      ;
      oceanData = Image(ostr.str());

      pic[d][stressorIndex].create2D(oceanData.width(), oceanData.height());
      pic[d][stressorIndex].submit(oceanData.array().data(), GL_RGBA,
                                   GL_UNSIGNED_BYTE);
      pic[d][stressorIndex].wrap(Texture::REPEAT);
      pic[d][stressorIndex].filter(Texture::LINEAR);
      loaded[d][stressorIndex] = true;
    }
  }

  void loadCO2Dataset(std::string video) {
    std::string path = dataPath + video;

    videoDecoder = new VideoDecoder();
    videoDecoder->enableAudio(false);
    videoDecoder->loop(true);

    if (!videoDecoder->load(path.c_str())) {
      std::cerr << "Error loading video file: " << path << std::endl;
    }
    // loadVideo = false;

    videoDecoder->start();

    tex0Y.create2D(videoDecoder->lineSize()[0], videoDecoder->height(),
                   Texture::RED, Texture::RED, Texture::UBYTE);
    tex0U.create2D(videoDecoder->lineSize()[1], videoDecoder->height() / 2,
                   Texture::RED, Texture::RED, Texture::UBYTE);
    tex0V.create2D(videoDecoder->lineSize()[2], videoDecoder->height() / 2,
                   Texture::RED, Texture::RED, Texture::UBYTE);
  }

  void loadChiData() {
    // 0. SST
    loadDataset("nasa/sst/", 0);
    // data_color = HSV(0.55 + log(pixel.r / 90. + 1), 0.65 + pixel.r / 60, 0.6
    // + atan(pixel.r / 300));

    // 1. Carbon
    loadDataset("nasa/carbon/", 1);
    // data_color = HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 +
    // pixel.r / 90);

    // 2. Chlorophyll
    loadDataset("nasa/chl/", 2);
    // data_color = HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 +
    // pixel.r / 90);

    // 3. Fluroscene Line Height
    loadDataset("nasa/flh/", 3);
    // data_color = HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 +
    // pixel.r / 90);

    // 0. SST
    // loadChiDataset("chi/sst/sst_05_", 0);
    // data_color = HSV(0.55 + log(pixel.r / 90. + 1), 0.65 + pixel.r / 60, 0.6
    // + atan(pixel.r / 300));

    // 1. Nutrients
    // loadChiDataset("chi/nutrient/nutrient_pollution_impact_5_", 1);
    // data_color = HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 +
    // pixel.r / 90);

    // 2. Shipping
    loadChiDataset("chi/ship/ship_impact_10_", "_equi.png", 5);
    // data_color = HSV(1 - log(pixel.r / 30. + 1), 0.6 + pixel.r / 100, 0.6 +
    // pixel.r / 60);

    // 3. Ocean Acidification
    loadChiDataset("chi/oa/oa_10_", "_impact.png", 6);
    // data_color = HSV(0.7 - 0.6 * log(pixel.r / 100. + 1), 0.5 + log(pixel.r /
    // 100. + 1), 1);

    // 4. Sea level rise
    loadChiDataset("chi/slr/slr_impact_5_", "_equi.png", 7);
    // data_color = HSV(0.6 + 0.2 * log(pixel.r / 100. + 1), 0.6 + log(pixel.r
    // / 60. + 1), 0.6 + log(pixel.r / 60. + 1));

    // 5. Fishing demersal low
    loadChiDataset("chi/fish/fph_100_", "_impact.png", 4);
    // data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);

    // 6. Fishing demersal high
    // loadChiDataset("chi/fish/fdh_10_", 6);
    // data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);

    // 7. Fishing pelagic low
    // loadChiDataset("chi/fish/fpl_10_", 7);
    // data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);

    // 8. Fishing pelagic high
    // loadChiDataset("chi/fish/fph_100_", 8);
    // data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);

    // 9. Direct human
    // loadChiDataset("chi/dh/dh_10_", 9);
    // data_color = HSV(log(pixel.r / 120. + 1), 0.9, 1);

    // 10. Organic chemical
    // loadChiDataset("chi/oc/oc_10_", 10);
    // data_color = HSV(log(pixel.r / 120. + 1), 0.9, 1);

    // 11. Cumulative human impacts
    // loadChiDataset("chi/chi/cumulative_impact_10_", 11);
    // data_color = HSV(log(pixel.r / 120. + 1), 0.9, 1);

    // Co2 Video data
    loadCO2Dataset(
        "co2/"
        "SOS_TaggedCO2_4-1-2024a_co2_FF_quality_ScienceOnASphere_1024p30.mp4");

    std::cout << "Loaded Ocean data." << std::endl;
  }
};

} // namespace al

#endif