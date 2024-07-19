
#ifndef OCEAN_DATA_VIEWER_HPP
#define OCEAN_DATA_VIEWER_HPP

#include <iostream>
#include <string.h>

#include "al/graphics/al_Image.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Light.hpp"

#include "Gamma/Filter.h"
#include "Gamma/Noise.h"
#include "Gamma/Oscillator.h"
#include "al/sound/al_Reverb.hpp"

#include "al/io/al_CSVReader.hpp"
#include "al/math/al_Random.hpp"

namespace al {
struct OceanDataViewer {

  struct GeoLoc {
    float lat;
    float lon;
    float radius;
  };

  struct CO2Types {
    double val[years + 2];
  };

  struct Particle {
    Vec3f pos, vel, acc;
    Color col;
    int age = 0;

    void update(int ageInc) {
      vel += acc;
      pos += vel;
      age += ageInc;
    }
  };

  template <int N> struct Emitter {
    Particle particles[N];
    int tap = 0;

    Emitter() {
      for (auto &p : particles)
        p.age = N;
    }

    template <int M> void update() {
      for (auto &p : particles)
        p.update(M);

      for (int i = 0; i < M; ++i) {
        auto &p = particles[tap];
        { // co2 particle spread speed
          p.vel.set(al::rnd::gaussian() * 0.003, al::rnd::gaussian() * 0.003,
                    al::rnd::gaussian() * 0.003);
          p.acc.set(al::rnd::gaussian() * 0.0003, al::rnd::gaussian() * 0.0003,
                    al::rnd::gaussian() * 0.0003);
        }
        p.pos.set(al::rnd::gaussian() * 1, al::rnd::gaussian() * 1, 0);
        p.age = 0;
        ++tap;
        if (tap >= N)
          tap = 0;
      }
    }

    int size() { return N; }
  };

  const char *vertex = R"(
  #version 400

  layout (location = 0) in vec3 vertexPosition;
  layout (location = 1) in vec4 vertexColor;

  uniform mat4 al_ModelViewMatrix;
  uniform mat4 al_ProjectionMatrix;

  out Vertex {
    vec4 color;
  } vertex;

  void main() {
    gl_Position = al_ModelViewMatrix * vec4(vertexPosition, 1.0);
    vertex.color = vertexColor;
  }
  )";
  const char *fragment = R"(
  #version 400

  in Fragment {
    vec4 color;
    vec2 textureCoordinate;
  } fragment;

  uniform sampler2D alphaTexture;

  layout (location = 0) out vec4 fragmentColor;

  void main() {
    // use the first 3 components of the color (xyz is rgb), but take the alpha value from the texture
    //
    fragmentColor = vec4(fragment.color.xyz, texture(alphaTexture, fragment.textureCoordinate));
  }
  )";
  const char *geometry = R"(
  #version 400

  // take in a point and output a triangle strip with 4 vertices (aka a "quad")
  //
  layout (points) in;
  layout (triangle_strip, max_vertices = 4) out;

  uniform mat4 al_ProjectionMatrix;

  // this uniform is *not* passed in automatically by AlloLib; do it manually
  //
  uniform float halfSize;

  in Vertex {
    vec4 color;
  } vertex[];

  out Fragment {
    vec4 color;
    vec2 textureCoordinate;
  } fragment;

  void main() {
    mat4 m = al_ProjectionMatrix; // rename to make lines shorter
    vec4 v = gl_in[0].gl_Position; // al_ModelViewMatrix * gl_Position

    gl_Position = m * (v + vec4(-halfSize, -halfSize, 0.0, 0.0));
    fragment.textureCoordinate = vec2(0.0, 0.0);
    fragment.color = vertex[0].color;
    EmitVertex();

    gl_Position = m * (v + vec4(halfSize, -halfSize, 0.0, 0.0));
    fragment.textureCoordinate = vec2(1.0, 0.0);
    fragment.color = vertex[0].color;
    EmitVertex();

    gl_Position = m * (v + vec4(-halfSize, halfSize, 0.0, 0.0));
    fragment.textureCoordinate = vec2(0.0, 1.0);
    fragment.color = vertex[0].color;
    EmitVertex();

    gl_Position = m * (v + vec4(halfSize, halfSize, 0.0, 0.0));
    fragment.textureCoordinate = vec2(1.0, 1.0);
    fragment.color = vertex[0].color;
    EmitVertex();

    EndPrimitive();
  }
  )";

  const std::string fadevert = R"(
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

  const std::string fadefrag = R"(
  #version 330
  uniform sampler2D tex0;
  uniform sampler2D tex1;
  uniform float blend0;
  uniform float blend1;
  uniform float brightness;

  in vec2 texcoord_;
  out vec4 frag_color;

  // can apply filters here
  void main() {
    vec4 c0 = blend0 * texture(tex0, texcoord_);
    vec4 c1 = blend1 * texture(tex1, texcoord_);
    frag_color = brightness * (c0 + c1);
  }
  )";


  Parameter lat{"lat", "", 0.0, -90.0, 90.0};
  Parameter lon{"lon", "", 0.0, -180.0, 180.0};
  Parameter radius{"radius", "", 5.0, 0.0, 50.0};
  ParameterVec3 llr{"llr", ""};
  ParameterPose camPose{"camPose", ""};
  Parameter lux{"Light", 0.6, 0, 2.5};
  Parameter year{"Year", 2013, 2013, 2023};
  // Parameter trans{"Trans", 0.99, 0.1, 1};
  Parameter gain{"Audio", 0, 0, 2};
  ParameterBool s_carbon{"Ocean_Carbon", "", 0.0};
  ParameterBool s_slr{"Sea_level_rise", "", 0.0};
  ParameterBool s_oa{"Ocean_acidification", "", 0.0};
  ParameterBool s_sst{"Sea_surface_temperature", "", 0.0};
  ParameterBool s_fish{"Over-fishing", "", 0.0};
  ParameterBool s_plastics{"Plastics", "", 0.0};
  ParameterBool s_resiliency{"Resiliency_Map", "", 0.0};
  ParameterBool s_cloud{"Clouds", "", 0.0};
  ParameterBool s_cloud_storm{"Clouds_Storm", "", 0.0};
  ParameterBool s_cloud_eu{"Clouds_EU", "", 0.0};
  ParameterBool s_co2{"CO2", "", 0.0};
  ParameterBool s_nav{"Explore_Globe", "", 0.0};
  ParameterBool s_years{"2003_2013", "", 0.0};

  ParameterBool animateCam{"animateCam", "", 0.0};
  ParameterBool faceTo{"Face_Center", "", 1.0};


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
  CSVReader reader;
  Vec3f co2_pos[num_county];
  float co2_level[num_county][years];
  ShaderProgram pointShader;
  ShaderProgram lineShader;
  Texture pointTexture;
  Texture lineTexture;
  Texture texture;      // co2
  ShaderProgram shaderParticle; // co2
  ShaderProgram shaderDataset; // co2

  FBO renderTarget;
  Texture rendered;
  float timer = 0;
  // CO2
  Emitter<500> emission;
  Mesh emission_mesh;

  void updateFBO(int w, int h) {
    rendered.create2D(w, h);
    renderTarget.bind();
    renderTarget.attachTexture2D(rendered);
    renderTarget.unbind();
  }

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

  void onInit() {
    // Import CO2 data
    // CSV columms label: lat + long + years (2003~2013)
    for (int k = 0; k < years + 2; k++) {
      reader.addType(CSVReader::REAL);
    }
    reader.readFile("data/co2/2003_2013.csv");
    std::vector<CO2Types> co2_rows = reader.copyToStruct<CO2Types>();
    // to test the csv import . values supposed to be: co2_row.val[-]: first two
    // column (lat, long) + years data year data : co2_row.val[2 ~ :] for (auto
    // co2_row : co2_rows) {
    //   cout << co2_row.val[0] << endl;
    // }
    // convert co2 lat lon to xyz
    int nation_id = 0;
    for (auto co2_row : co2_rows) {
      float lat = co2_row.val[0];
      float lon = co2_row.val[1];
      co2_pos[nation_id] =
          Vec3f(-cos(lat / 180.0 * M_PI) * sin(lon / 180.0 * M_PI),
                sin(lat / 180.0 * M_PI),
                -cos(lat / 180.0 * M_PI) * cos(lon / 180.0 * M_PI));
      for (int i = 0; i < years; i++) {
        co2_level[nation_id][i] = co2_row.val[2 + i];
      }
      // addCube(co2_mesh[nation_id], false, 0.01);
      // co2_mesh[nation_id].decompress();
      // co2_mesh[nation_id].generateNormals();
      // co2_mesh[nation_id].update();
      nation_lat[nation_id] = lat;
      nation_lon[nation_id] = lon;
      nation_id++;
    }

    // Import Cloud figures
    Image cloudData;
    int cloud_W, cloud_H;
    for (int d = 0; d < num_cloud; d++) {
      ostringstream ostr;
      ostr << "data/cloud/" << d << ".jpg";
      cloudData = Image(ostr.str());
      cloud[d].create2D(cloudData.width(), cloudData.height());
      cloud[d].submit(cloudData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);
      cloud[d].filter(Texture::LINEAR);
    }
  }

  void onCreate() {

    addSphereWithTexcoords(skyMesh, 50, 50, true);
    skyMesh.update();

    addSphereWithTexcoords(sphereMesh, 2, 50, false);
    sphereMesh.update();
    std::string dataPath;

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

    //// Shader
    // use a texture to control the alpha channel of each particle
    //
    texture.create2D(250, 250, Texture::R8, Texture::RED, Texture::SHORT);
    int Nx = texture.width();
    int Ny = texture.height();
    std::vector<short> alpha;
    alpha.resize(Nx * Ny);
    for (int j = 0; j < Ny; ++j) {
      float y = float(j) / (Ny - 1) * 2 - 1;
      for (int i = 0; i < Nx; ++i) {
        float x = float(i) / (Nx - 1) * 2 - 1;
        float m = exp(-13 * (x * x + y * y));
        m *= std::pow(2, 15) - 1; // scale by the largest positive short int
        alpha[j * Nx + i] = m;
      }
    }
    texture.submit(&alpha[0]);

    // compile and link the three shaders
    //
    shaderParticle.compile(vertex, fragment, geometry);
    ////
  }

  void onAnimate(double dt, Nav &nav, State &state, bool isPrimary) {

    timer += dt;
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

      // CO2 Emission animate
      emission.update<5>();
      emission_mesh.reset();
      emission_mesh.primitive(Mesh::POINTS);
      for (int i = 0; i < emission.size(); ++i) {
        Particle &p = emission.particles[i];
        float age = float(p.age) / emission.size();

        emission_mesh.vertex(p.pos);
        // emission_mesh.color(HSV(al::rnd::uniform(1.), al::rnd::uniform(0.7),
        // (1 - age) * 0.8));
        p.col = HSV(al::rnd::uniform(0.2), al::rnd::uniform(0.1, 0.3), (1 - 0.1*age) * 0.5);
        emission_mesh.color(p.col);
      }

      // Set light position
      light.pos(nav.pos().x, nav.pos().y, nav.pos().z);
      Light::globalAmbient({lux, lux, lux});
      state.lux = lux;
      if (state.molph) {
        year = year + 3 * dt;
        if (year == 2013) {
          year = 2013;
          state.molph = false;
          s_years.set(1);
        }
      }
      if (s_nav) {
        nav.moveR(0.003*0.25);
      }
      //  audio
      mFilter.freq(30 * (1 + 10 / (radius + 3)) * (year - 2000));
      // mFilter.res();
      mFilter.type(gam::LOW_PASS);
      reverb.decay(0.6f + 0.3 / (radius + 1)); // Tail decay factor, in [0,1]


      // Assign shared states for renderers
      state.global_pose.set(nav);
      state.year = year;
      state.radius = radius;
      state.swtch[0] = s_sst;
      state.swtch[1] = s_carbon;
      state.swtch[2] = s_fish;
      state.swtch[3] = s_oa;
      state.swtch[4] = s_slr;
      state.swtch[5] = s_plastics;
      state.swtch[6] = s_resiliency;
      state.cloud_swtch[0] = s_cloud;
      state.cloud_swtch[1] = s_cloud_eu;
      state.cloud_swtch[2] = s_cloud_storm;
      state.co2_swtch = s_co2;
      for (int i = 0; i < emission.size(); ++i) {
        Particle& p = emission.particles[i];
        state.render_co2_pos[i] = p.pos;
        state.render_co2_col[i] = p.col;
      }
    }    // prim end
    else // renderer
    {
      nav.set(state.global_pose);
      light.pos(nav.pos().x, nav.pos().y, nav.pos().z);
      Light::globalAmbient({state.lux, state.lux, state.lux});
      emission_mesh.reset();
      emission_mesh.primitive(Mesh::POINTS);
      for (int i = 0; i < emission.size(); ++i) {
        Particle& p = emission.particles[i];
        float age = float(p.age) / emission.size();
        emission_mesh.vertex(state.render_co2_pos[i]);
        emission_mesh.color(state.render_co2_col[i]);        
      }
    }
  }

  void onDraw(Graphics &g, Nav &nav, State &state) {
    g.clear(0);
    g.culling(false);
    // g.cullFaceFront();
    // g.lighting(true);
    g.light(light);
    g.texture();
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

    // sphere
    g.pushMatrix();
    sphereTex.bind();
    // g.cullFaceFront();
    g.draw(sphereMesh); // only needed if we go inside the earth
    // g.cullFaceBack();
    // g.draw(sphereMesh);
    sphereTex.unbind();
    
    g.popMatrix();

    // Draw data
    g.lighting(false);
    // g.blending(true);
    // g.depthTesting(false);
    gl::depthFunc(GL_LEQUAL);
    // g.blendTrans();
    g.blendAdd();

    // g.shader(shaderDataset);
    // shaderDataset.uniform("tex0", 0);

    for (int j = 0; j < stressors; j++) {
      if (state.swtch[j]) {
        // g.clearDepth();

        // shaderDataset.uniform("mapFunction", j);

        pic[(int)state.year - 2013][j].bind();
        // g.cullFaceFront();
        g.draw(sphereMesh); // only needed if we go inside the earth
      }
    }
    // draw cloud
    for (int j = 0; j < num_cloud; j++) {
      if (state.cloud_swtch[j]) {
        // g.clearDepth();
        cloud[j].bind();
        g.draw(sphereMesh);
      }
    }
    // Draw CO2
    if (state.co2_swtch) {
      texture.bind();
      g.meshColor();
      g.blendTrans();
      g.blending(true);
      g.depthTesting(true);
      for (int nation = 0; nation < num_county; nation++) {
        float co2 = co2_level[nation][(int)state.year - 2003] *
                    0.000001; // precompute micro quantity since large
        g.pushMatrix();
        g.translate(co2_pos[nation] * 2.01);
        // g.translate(0,0,3);
        g.scale(co2 * 0.01, co2 * 0.01, co2 * 0.01);
        g.rotate(-90, Vec3f(0, 1, 0));
        g.rotate(nation_lon[nation], Vec3f(0, 1, 0));
        g.rotate(nation_lat[nation], Vec3f(1, 0, 1));
        g.pointSize(co2 * 0.1*log2(1+co2) / radius.get());
        g.shader(shaderParticle);
        float halfSize = 0.005 * co2;
        g.shader().uniform("halfSize", halfSize < 0.01 ? 0.01 : halfSize);
        g.pointSize(co2);
        g.draw(emission_mesh);
        g.popMatrix();
      }
      texture.unbind();
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
    // sourceGeoLoc.lat = lat.get();
    // sourceGeoLoc.lon = lon.get();
    // sourceGeoLoc.radius = radius.get();
    // targetGeoLoc.lat = la;   // 53.54123998879464;
    // targetGeoLoc.lon = lo;   // 9.950943100405375;
    // targetGeoLoc.radius = r; // 3.2;
    // morphDuration = duration;
    // morphProgress = morphDuration;
    // hoverDuration = 0;
    navTarget.pos(Vec3d(-r * cos(la / 180.0 * M_PI) *
                          sin(lo / 180.0 * M_PI),
                      r * sin(la / 180.0 * M_PI),
                      -r * cos(la / 180.0 * M_PI) *
                          cos(lo / 180.0 * M_PI)));

    navTarget.faceToward(Vec3d(0), Vec3d(0, 1, 0));
    animateCam = true;
    anim_speed = anim_speed / 5;
  }


  void registerParams(ControlGUI *gui, PresetHandler &presets, PresetSequencer &seq, SequenceRecorder &rec, Nav &nav, State &state) {
    std::string displayText =
        "AlloOcean. Ocean stressor (2012-2023)";
    *gui << lat << lon << radius; // << lux << year << gain;
    *gui << year;
    *gui << s_years;
    *gui << s_nav << faceTo << animateCam;
    *gui << s_carbon << s_slr << s_oa << s_sst;
    *gui << s_fish << s_plastics << s_resiliency;
    *gui << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << lux;
    // *gui << s_cf_dd << a_f // currently we don't have this data
    // *gui << s_ci << s_oc << s_carbon;

    // *gui << lat << lon << radius << lux << year << trans << gain;

  
    presets << year << camPose;
    presets << s_years << s_nav;
    presets << s_carbon << s_slr << s_oa << s_sst;
    presets << s_fish << s_plastics << s_resiliency;
    presets << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << lux;

    seq << llr << s_years << s_nav << camPose << faceTo;
    seq << s_carbon << s_slr << s_oa << s_sst;
    seq << s_fish << s_plastics << s_resiliency;
    seq << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << lux;

    // rec << year;
    // rec << s_years << s_nav;
    // rec << s_ci << s_oc << s_carbon << s_dh << s_slr << s_oa << s_sst;
    // rec << s_cf_pl << s_cf_ph << s_cf_dl << s_cf_dh << s_shp;
    // rec << s_cloud << s_cloud_storm << s_cloud_eu << s_co2 << lux;



    camPose.registerChangeCallback([&](Pose p) {
      nav.set(p);
    });
    llr.registerChangeCallback([&](Vec3f v) {
      setGeoTarget(v.x, v.y, v.z);
    });

    lat.registerChangeCallback([&](float value) {
      setGeoTarget(value, lon, radius);
      // nav.pos(Vec3d(-radius.get() * cos(value / 180.0 * M_PI) *
      //                     sin(lon.get() / 180.0 * M_PI),
      //                 radius.get() * sin(value / 180.0 * M_PI),
      //                 -radius.get() * cos(value / 180.0 * M_PI) *
      //                     cos(lon.get() / 180.0 * M_PI)));

      // nav.faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });

    lon.registerChangeCallback([&](float value) {
      setGeoTarget(lat, value, radius);
      // nav.pos(Vec3d(-radius.get() * cos(lat.get() / 180.0 * M_PI) *
      //                     sin(value / 180.0 * M_PI),
      //                 radius.get() * sin(lat.get() / 180.0 * M_PI),
      //                 -radius.get() * cos(lat.get() / 180.0 * M_PI) *
      //                     cos(value / 180.0 * M_PI)));
      // nav.faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });

    radius.registerChangeCallback([&](float value) {
      setGeoTarget(lat, lon, value);
      // nav.pos(Vec3d(-value * cos(lat.get() / 180.0 * M_PI) *
      //                     sin(lon.get() / 180.0 * M_PI),
      //                 value * sin(lat.get() / 180.0 * M_PI),
      //                 -value * cos(lat.get() / 180.0 * M_PI) *
      //                     cos(lon.get() / 180.0 * M_PI)));
      // nav.faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });

    s_years.registerChangeCallback([&](int value) {
      if (value) {
        state.molph = true; //!state.molph;
        year = 2003;
        // s_years.set(0);
      } else { 
        state.molph = false;
      }
    });
  }


  void loadChiDataset(std::string pathPrefix, int stressorIndex){
    Image oceanData;
    std::cout << "Start loading stressorIndex: " << stressorIndex << std::endl;
    for (int d = 0; d < years; d++) {
      ostringstream ostr;
      ostr << pathPrefix << d + 2012 << ".png"; 
      // char *filename = new char[ostr.str().length() + 1];
      // std::strcpy(filename, ostr.str().c_str());
      // read data with the ostr string name
      oceanData = Image(ostr.str());

      pic[d][stressorIndex].create2D(oceanData.width(), oceanData.height());
      // pic[d][stressorIndex].submit(oceanData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);
      pic[d][stressorIndex].submit(oceanData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);
      pic[d][stressorIndex].wrap(Texture::REPEAT); 
      pic[d][stressorIndex].filter(Texture::LINEAR);
      loaded[d][stressorIndex] = true;
    }   
  }

  void loadChiData() {
    // 0. SST
    loadChiDataset("data/nasa/sst/", 0);
    // data_color = HSV(0.55 + log(pixel.r / 90. + 1), 0.65 + pixel.r / 60, 0.6 + atan(pixel.r / 300));

    // 1. Nutrients
    loadChiDataset("data/nasa/carbon/", 1);
    // data_color = HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 + pixel.r / 90);


    std::cout << "Loaded CHI data." << std::endl;
  }

  

};

} // namespace al

#endif