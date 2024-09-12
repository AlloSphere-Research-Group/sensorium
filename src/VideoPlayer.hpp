
#ifndef VIDEO_PLAYER_HPP
#define VIDEO_PLAYER_HPP

#include <memory>

#include "al/sound/al_SpeakerAdjustment.hpp"
#include "al/sphere/al_SphereUtils.hpp"
#include "al_ext/video/al_VideoDecoder.hpp"

namespace al {

struct VideoPlayer {

  const std::string pano_vert = R"(
  #version 330
  uniform mat4 al_ModelViewMatrix;
  uniform mat4 al_ProjectionMatrix;

  layout (location = 0) in vec3 position;
  layout (location = 2) in vec2 texcoord;

  uniform float eye_sep;
  uniform float foc_len;

  out vec2 texcoord_;

  vec4 stereo_displace(vec4 v, float e, float f) {
    // eye to vertex distance
    float l = sqrt((v.x - e) * (v.x - e) + v.y * v.y + v.z * v.z);
    // absolute z-direction distance
    float z = abs(v.z);
    // x coord of projection of vertex on focal plane when looked from eye
    float t = f * (v.x - e) / z;
    // x coord of displaced vertex to make displaced vertex be projected on focal plane
    // when looked from origin at the same point original vertex would be projected
    // when looked form eye
    v.x = z * (e + t) / f;
    // set distance from origin to displaced vertex same as eye to original vertex
    v.xyz = normalize(v.xyz);
    v.xyz *= l;
    return v;
  }

  void main() {
    if (eye_sep == 0) {
      gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * vec4(position, 1.0);
    }
    else {
      gl_Position = al_ProjectionMatrix * stereo_displace(al_ModelViewMatrix * vec4(position, 1.0), eye_sep, foc_len);
    }

    texcoord_ = texcoord;
  }
  )";

  const std::string pano_frag = R"(
  #version 330
  uniform sampler2D texY;
  uniform sampler2D texU;
  uniform sampler2D texV;
  uniform float blend0;
  uniform float blend1;
  uniform float brightness;

  in vec2 texcoord_;
  out vec4 frag_color;

  // can apply filters here
  void main() {
    vec3 yuv0;
    yuv0.r = texture(tex0Y, texcoord_).r - 0.0625;
    yuv0.g = texture(tex0U, texcoord_).r - 0.5;
    yuv0.b = texture(tex0V, texcoord_).r - 0.5;

    vec4 rgba0;
    rgba0.r = yuv0.r + 1.596 * yuv0.b;
    rgba0.g = yuv0.r - 0.813 * yuv0.b - 0.391 * yuv0.g;
    rgba0.b = yuv0.r + 2.018 * yuv0.g;
    rgba0.a = 1.0;

    vec4 c0 = blend0 * vec4(0);
    vec4 c1 = blend1 * rgba0;
    frag_color = brightness * (c0 + c1);
  }
  )";

  int addSphereWithEquirectTex(Mesh &m, double radius, int bands) {
    m.primitive(Mesh::TRIANGLES);

    double &r = radius;

    for (int lat = 0; lat <= bands; ++lat) {
      double theta = lat * M_PI / bands;
      double sinTheta = sin(theta);
      double cosTheta = cos(theta);

      for (int lon = 0; lon <= bands; ++lon) {
        double phi = lon * M_2PI / bands;
        double sinPhi = sin(phi);
        double cosPhi = cos(phi);

        double x = sinPhi * sinTheta;
        double y = cosTheta;
        double z = cosPhi * sinTheta;

        // inversed longitude when looked from inside
        double u = 1.0 - (double)lon / bands;
        double v = (double)lat / bands;

        m.vertex(r * x, r * y, r * z);
        m.texCoord(u, v);
        // inversed normal
        m.normal(-x, -y, -z);
      }
    }

    for (int lat = 0; lat < bands; ++lat) {
      for (int lon = 0; lon < bands; ++lon) {
        int first = lat * (bands + 1) + lon;
        int second = first + bands + 1;

        m.index(first);
        m.index(first + 1);
        m.index(second);

        m.index(second);
        m.index(first + 1);
        m.index(second + 1);
      }
    }

    return m.vertices().size();
  }

  ShaderProgram pano_shader;

  Texture texY, texU, texV;
  VAOMesh quad, sphere;

  std::unique_ptr<VideoDecoder> videoDecoder;
  bool loadVideo{false};

  std::string dataPath;

  ParameterString videoToLoad{"videoToLoad", ""};

  ParameterBool playingVideo{"playingVideo", "", 0.0};
  ParameterBool renderVideoInSim{"renderVideoInSim", "", 0.0};
  Parameter brightness{"brightness", "", 1.0, 0.0, 2.0};
  Parameter blend0{"blend0", "", 0.0, 0.0, 1.0};
  Parameter blend1{"blend1", "", 1.0, 0.0, 1.0};

  Trigger playWater{"Play_Water", ""};
  Trigger playAerialImages{"Play_AerialImages", ""};
  Trigger playSF{"Play_SF", ""};
  Trigger playBoardwalk{"Play_Boardwalk", ""};
  Trigger playCoral{"Play_Coral", ""};
  Trigger playOverfishing{"Play_Overfishing", ""};
  Trigger playAcidification{"Play_Acidification", ""};
  Trigger playBoat{"Play_Boat", ""};
  Trigger swapVideo{"swapVideo", ""};

  ParameterPose renderPose{"renderPose", "", Pose(Vec3d(0, 0, 0))};
  ParameterVec3 renderScale{"renderScale", "", Vec3f(1, 1, 1)};

  void registerParams(ControlGUI *gui, PresetHandler &presets,
                      PresetSequencer &seq, SequenceRecorder &rec,
                      State &state) {
    *gui << renderVideoInSim << playingVideo << brightness << blend0 << blend1;
    *gui << playWater << playAerialImages << playSF;
    *gui << playBoardwalk << playCoral;
    *gui << playOverfishing << playAcidification << playBoat;
    *gui << renderPose << renderScale;

    presets << renderVideoInSim << playingVideo << brightness << blend0
            << blend1;
    seq << videoToLoad << blend0 << blend1 << playingVideo;
    seq << playBoardwalk << playCoral << playOverfishing << playAerialImages
        << playAcidification << playSF << playBoat
        << playWater; //<< renderPose << renderScale;

    // these change callbacks should run only on primary
    playingVideo.registerChangeCallback([&](float value) {
      if (value == 1.0) {
        // state.global_clock = 0.0;
        // state.global_clock_next = 0.0;
        state.videoPlaying = true;
      } else {
        state.videoPlaying = false;
      }
    });

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
      // videoToLoad.set("Sensorium_Mono_Final_Comped_02.mov");
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
    playWater.registerChangeCallback([&](float value) {
      videoToLoad.set("out3r2x.mp4");
      // videoToLoad.set("Iron_Man-Trailer_HD.mp4");
      playingVideo.set(1.0);
    });
  }

  void loadVideoFile(State &state, bool isPrimary) {
    std::string path = dataPath + videoToLoad.get();

    if (videoDecoder != nullptr) {
      videoDecoder->stop();
      videoDecoder.reset(nullptr);
    }
    videoDecoder = std::make_unique<VideoDecoder>();
    videoDecoder->enableAudio(false);

    if (!videoDecoder->load(path.c_str())) {
      std::cerr << "Error loading video file: " << path << std::endl;
      return;
    }
    loadVideo = false;

    videoDecoder->start();

    texY.create2D(videoDecoder->lineSize()[0], videoDecoder->height(),
                  Texture::RED, Texture::RED, Texture::UBYTE);
    texU.create2D(videoDecoder->lineSize()[1], videoDecoder->height() / 2,
                  Texture::RED, Texture::RED, Texture::UBYTE);
    texV.create2D(videoDecoder->lineSize()[2], videoDecoder->height() / 2,
                  Texture::RED, Texture::RED, Texture::UBYTE);

    if (isPrimary)
      state.global_clock_next = 0.0;
  };

  void onInit() {}

  void onCreate(State &state, bool isPrimary) {
    if (sphere::isSphereMachine()) {
      if (sphere::isRendererMachine()) {
        dataPath = "/data/Sensorium/video/";
      } else {
        dataPath = "/Volumes/Data/Sensorium/video/";
      }
    } else {
      dataPath = "data/video/";
    }

    // compile & initialize shader
    pano_shader.compile(pano_vert, pano_frag);
    pano_shader.begin();
    pano_shader.uniform("texY", 0);
    pano_shader.uniform("texU", 1);
    pano_shader.uniform("texV", 2);
    pano_shader.uniform("blend0", 1.0);
    pano_shader.uniform("blend1", 1.0);
    pano_shader.uniform("brightness", 1.0);
    pano_shader.end();

    // generate texture
    texY.filter(Texture::LINEAR);
    texY.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);
    texU.filter(Texture::LINEAR);
    texU.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);
    texV.filter(Texture::LINEAR);
    texV.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);

    // generate mesh
    addTexRect(quad, -1, 1, 2, -2);
    quad.update();

    addSphereWithEquirectTex(sphere, 10, 50);
    sphere.update();

    if (isPrimary) {
      state.global_clock = 0;
      state.global_clock_next = 0;
      state.videoPlaying = false;
    }
  }

  void onAnimate(al_sec dt, State &state, bool isPrimary) {

    if (loadVideo)
      loadVideoFile(state, isPrimary);

    if (isPrimary) {
      if (state.videoPlaying) {
        state.global_clock += dt;
        state.global_clock_next += dt;
      }
    }

    if (state.videoPlaying) {
      MediaFrame *frame;
      if (videoDecoder != nullptr) {
        frame = videoDecoder->getVideoFrame(state.global_clock);
        if (frame) {
          texY.submit(frame->dataY.data());
          texU.submit(frame->dataU.data());
          texV.submit(frame->dataV.data());
          videoDecoder->gotVideoFrame();
        } else if (videoDecoder->finished() && videoDecoder->isLooping()) {
          state.global_clock = 0;
          videoDecoder->seek(0);
        }
      }
    }
  }

  void onDraw(Graphics &g, Nav &nav, State &state, bool isPrimary) {

    if (state.videoPlaying) {
      // nav.quat().fromAxisAngle(0.5 * M_2PI, 0, 1, 0);
      // nav.pos().set(0);
      nav.setIdentity();

      g.clear();

      if (renderVideoInSim || !isPrimary) {
        g.pushMatrix();
        g.shader(pano_shader);
        g.shader().uniform("brightness", brightness);
        g.shader().uniform("blend0", blend0);
        g.shader().uniform("blend1", blend1);
        texY.bind(0);
        texU.bind(1);
        texV.bind(2);
        g.translate(renderPose.get().pos());
        g.rotate(renderPose.get().quat());
        g.scale(renderScale.get());
        g.draw(sphere);
        texY.unbind(0);
        texU.unbind(1);
        texV.unbind(2);

        g.popMatrix();
      }
    }
  }
};

} // namespace al

#endif