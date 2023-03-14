
#ifndef VIDEO_PLAYER_HPP
#define VIDEO_PLAYER_HPP

#include "al_ext/video/al_VideoDecoder.hpp"
#include "al/sphere/al_SphereUtils.hpp"
#include "al/sound/al_SpeakerAdjustment.hpp"


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
  uniform sampler2D tex0;
  uniform float exposure;

  in vec2 texcoord_;
  out vec4 frag_color;

  mat4 exposureMat (float value) {
    return mat4(value, 0, 0, 0,
                0, value, 0, 0,
                0, 0, value, 0,
                0, 0, 0, 1);
  }

  // can apply filters here
  void main() {
    frag_color = exposureMat(exposure) * texture(tex0, texcoord_);

    // frag_color = texture(tex0, texcoord_);
  }
  )";

  int addSphereWithEquirectTex(Mesh &m, double radius, int bands){
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


  struct MappedAudioFile {
    // std::unique_ptr<SoundFileBuffered> soundfile;
    std::vector<size_t> outChannelMap;
    std::string fileInfoText;
    std::string fileName;
    float gain;
    bool mute{false};
  };

  Texture tex;
  VAOMesh quad, sphere;
  bool equirectangular{true};

  ShaderProgram pano_shader;
  float exposure;
  bool uniformChanged{false};

  VideoDecoder videoDecoder;
  std::string videoFileToLoad;

  bool playing{true};

  std::vector<MappedAudioFile> soundfiles;
  uint64_t samplesPlayed{0};
  int32_t audioDelay{0};

  ParameterBool renderVideo{"renderVideo", "", 1.0};
  ParameterBool windowed{"windowed", "", 0.0};
  ParameterPose renderPose{"renderPose", "", Pose(Vec3d(0, 0, -4))};
  ParameterVec3 renderScale{"renderScale", "", Vec3f(1, 1, 1)};

  void onInit(){
    exposure = 1.0f;

    // parameterServer() << renderPose << renderScale << windowed;
    // configureAudio();
    // for (const auto &sf : soundfiles) {
    //   sf.soundfile->seek(-audioDelay);
    // }
    samplesPlayed = -audioDelay;
    // if (sphere::isRendererMachine()) {
    //   gainComp.configure(AlloSphereSpeakerLayoutCompensated());
    //   audioIO().append(gainComp);
    // }
    // Doesn't work to avoid stereo display
    // if (!isPrimary() && omniRendering) {
    //   // Disable stereo
    //   omniRendering->stereo(false);
    //   displayMode(Window::DEFAULT_BUF);
    // }
  }

  void onCreate(State &state, bool isPrimary){
    // compile & initialize shader
    pano_shader.compile(pano_vert, pano_frag);

    pano_shader.begin();
    pano_shader.uniform("tex0", 0);
    pano_shader.uniform("exposure", exposure);
    pano_shader.end();

    // TODO: temporarily disabled audio
    // if (!isPrimary()) {
    videoDecoder.enableAudio(false);
    // }

    // if (isPrimary()) {
    // videoDecoder.setMasterSync(MasterSync::AV_SYNC_AUDIO);
    // }

    // TODO: check audio startup
    if (!videoDecoder.load(videoFileToLoad.c_str())) {
      if (videoFileToLoad.size() > 0) {
        std::cerr << "Error loading video file" << std::endl;
        // quit();
      }
    }

    // // TODO: this can probably be extracted from video file metadata
    equirectangular = true;

    videoDecoder.start();

    // generate texture
    tex.filter(Texture::LINEAR);
    tex.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE, Texture::CLAMP_TO_EDGE);
    tex.create2D(videoDecoder.width(), videoDecoder.height(), Texture::RGBA8,
                Texture::RGBA, Texture::UBYTE);

    // generate mesh
    addTexRect(quad, -1, 1, 2, -2);
    quad.update();

    // addSphereWithTexcoords(sphere, 5, 20);
    addSphereWithEquirectTex(sphere, 10, 50);
    sphere.update();

    // TODO: review high fps option with manual timing control
    // set fps
    if (isPrimary) {
      // fps(videoDecoder.fps());
      // mtcReader.TCframes.setCurrent("30");
      state.global_clock = 0;
      // renderVideo.set(0.0);
      // showHUD = true;
    }
    // if (!isPrimary() && omniRendering) {
    //   // Disable stereo
    //   omniRendering->stereo(false);
    //   displayMode(Window::DEFAULT_BUF);
    // }

    // start GUI
    // if (hasCapability(Capability::CAP_2DGUI)) {
      // imguiInit();
    // }
  }

  void onAnimate(al_sec dt, State &state, bool isPrimary){
    // nav().pos().set(0);

    if (isPrimary) {
      // uint8_t hour{0}, minute{0}, second{0}, frame{0};

      // if (syncToMTC.get() == 1.0) {
        // TODO: review drift + handle offset (won't start at 0)
        // mtcReader.getMTC(hour, minute, second, frame);

        // state().global_clock = ((int32_t)hour * 360) + ((int32_t)minute * 60) +
        //                       second + (frame / mtcReader.fps()) +
        //                       mtcReader.frameOffset;
      // } else 
      if (playing) {
        state.global_clock += dt;
      }

      // if (hasCapability(Capability::CAP_2DGUI)) {
      //   imguiBeginFrame();

      //   ImGui::Begin("MIDI Time Code");

        // ParameterGUI::draw(&renderVideo);
      //   ParameterGUI::draw(&syncToMTC);
      //   ParameterGUI::drawMIDIIn(&mtcReader.midiIn);
      //   ParameterGUI::draw(&mtcReader.TCframes);
      //   ParameterGUI::draw(&mtcReader.frameOffset);

      //   ImGui::Text("%02i:%02i:%02i:%02i", hour, minute, second,
      //               (int)(frame / mtcReader.fps() * 100));
      //   ParameterGUI::draw(&renderPose);
      //   ParameterGUI::draw(&renderScale);
      //   ParameterGUI::draw(&windowed);

      //   ImGui::End();
      //   imguiEndFrame();
      // }
    }

    if (playing && renderVideo.get() == 1.0) {
      uint8_t *frame = videoDecoder.getVideoFrame(state.global_clock);

      if (frame) {
        tex.submit(frame);
        videoDecoder.gotVideoFrame();
      }
    }
  }

  void onDraw(Graphics &g, bool isPrimary){
    g.clear();

    if (renderVideo.get() == 1.0) {
      if (isPrimary) {
        if (windowed.get() == 1.0) {
          g.pushMatrix();
          g.translate(renderPose.get().pos());
          g.rotate(renderPose.get().quat());
          g.scale(renderScale.get());
          g.scale((float)videoDecoder.width() / (float)videoDecoder.height(), 1,
                  1);
          tex.bind();
          g.texture();
          g.draw(quad);
          tex.unbind();

          g.popMatrix();
        } else {
          g.pushMatrix();
          g.translate(renderPose.get().pos());
          g.rotate(renderPose.get().quat());
          g.scale(renderScale.get());
          // g.scale((float)videoDecoder.width() / (float)videoDecoder.height(), 1, 1);
          tex.bind();
          g.texture();
          g.draw(sphere);
          tex.unbind();

          g.popMatrix();
        }
      } else {
        g.pushMatrix();
        if (windowed.get() == 1.0) {
          // TODO there is likely a better way to set the pose.
          g.translate(renderPose.get().pos());
          g.rotate(renderPose.get().quat());
          g.scale(renderScale.get());
          g.texture();
          tex.bind();
          g.scale((float)videoDecoder.width() / (float)videoDecoder.height(), 1,
                  1);
          g.draw(quad);
          tex.unbind();
        } else {

          // Renderer
          g.shader(pano_shader);

          // TODO: add exposure control
          if (uniformChanged) {
            g.shader().uniform("exposure", exposure);
            uniformChanged = false;
          }

          tex.bind();
          // TODO there is likely a better way to set the pose.
          //      g.translate(renderPose.get().pos());
          g.rotate(renderPose.get().quat());
          g.scale(renderScale.get());
          g.draw(sphere);
          tex.unbind();
        }
        g.popMatrix();
      }
    }
  }

  // void onSound(AudioIOData &io){}
  // bool onKeyDown(const Keyboard &k){}
  // void onExit(){}

  
  // void configureAudio();

  // bool loadAudioFile(std::string name, std::vector<size_t> channelMap,
  //                    float gain, bool loop);
  // void setAudioDelay(int32_t delaySamples) { audioDelay = delaySamples; }
  // double wallTime{0};

  void setVideoFile(std::string videoFileUrl) {
    // videoFileToLoad = dataRoot + videoFileUrl;
    videoFileToLoad = videoFileUrl;
  };

  void setWindowed(Pose pose, Vec3f scale){
    windowed = 1.0;
    renderPose.set(pose);
    renderScale.set(scale);
  }


  
  
};

} // namespace al


#endif