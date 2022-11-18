#include <iostream>
#include <string.h>
#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Image.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Light.hpp"

using namespace al;
using namespace std;

struct State {
  Pose global_pose;
};

struct GeoLoc {
  float lat;
  float lon;
  float radius;
};

struct SensoriumApp : public DistributedAppWithState<State> {
  VAOMesh skyMesh, sphereMesh;
  Image skyImage, sphereImage;
  Texture skyTex, sphereTex;

  ControlGUI *gui;
  Parameter lat{"lat", "", 0.0, -90.0, 90.0};
  Parameter lon{"lon", "", 0.0, -180.0, 180.0};
  Parameter radius{"radius", "", 5.0, 0.0, 50.0};
  Parameter lux{"Light", 1, 0, 1};

  GeoLoc sourceGeoLoc, targetGeoLoc;
  static const int years = 7;
  static const int stressors = 2;  
  Image oceanData[years][stressors];
  VAOMesh pic[years][stressors];

  double morphProgress{0.0};
  double morphDuration{5.0};
  const double defaultMorph{5.0};
  float hoverHeight{10.f};
  double hoverDuration{2.5};
  const double defaultHover{2.5};
  Light light;

  std::shared_ptr<CuttleboneDomain<State>> cuttleboneDomain;

  void onInit() override {
    cuttleboneDomain = CuttleboneDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone" << std::endl;
      quit();
    }
  }

  void onCreate() override {
    lens().fovy(45).eyeSep(0);
    nav().pos(0, 0, -5);
    nav().quat().fromAxisAngle(0.5 * M_2PI, 0, 1, 0);

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
    sphereImage = Image(dataPath + "blue_marble_brighter.jpg");
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

    if (isPrimary()) {
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      gui = &guiDomain->newGUI();

      *gui << lat << lon << radius;
    }

    // enable if parameter needs to be shared
    // parameterServer() << lat << lon << radius;

    lat.registerChangeCallback([&](float value) {
      nav().pos(Vec3d(-radius.get() * cos(value / 180.0 * M_PI) *
                          sin(lon.get() / 180.0 * M_PI),
                      radius.get() * sin(value / 180.0 * M_PI),
                      -radius.get() * cos(value / 180.0 * M_PI) *
                          cos(lon.get() / 180.0 * M_PI)));

      nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });

    lon.registerChangeCallback([&](float value) {
      nav().pos(Vec3d(-radius.get() * cos(lat.get() / 180.0 * M_PI) *
                          sin(value / 180.0 * M_PI),
                      radius.get() * sin(lat.get() / 180.0 * M_PI),
                      -radius.get() * cos(lat.get() / 180.0 * M_PI) *
                          cos(value / 180.0 * M_PI)));
      nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });

    radius.registerChangeCallback([&](float value) {
      nav().pos(Vec3d(-value * cos(lat.get() / 180.0 * M_PI) *
                          sin(lon.get() / 180.0 * M_PI),
                      value * sin(lat.get() / 180.0 * M_PI),
                      -value * cos(lat.get() / 180.0 * M_PI) *
                          cos(lon.get() / 180.0 * M_PI)));
      nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });

    // Bring ocean data (image)
    // SST
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/sst/sst_05_" << d + 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      std::strcpy(filename, ostr.str().c_str());
      oceanData[d][0] = Image(filename);
      if (oceanData[d][0].array().size() == 0)
      {
        std::cout << "failed to load image" << std::endl;
        exit(1);
      }
      std::cout << "loaded image size: " << oceanData[d][0].width() << ", "
           << oceanData[d][0].height() << std::endl;
      pic[d][0].primitive(Mesh::POINTS);
      // pic[d][1].update();
    }

    // Nutrients
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/nutrient/nutrient_pollution_impact_5_" << 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][1] = Image(filename);
      if (oceanData[d][1].array().size() == 0)
      {
        std::cout << "failed to load image" << std::endl;
        exit(1);
      }
      std::cout << "loaded image size: " << oceanData[d][1].width() << ", "
           << oceanData[d][1].height() << std::endl;
      pic[d][1].primitive(Mesh::POINTS);
      // pic[d][1].update();
    }
  }

  void onAnimate(double dt) override {
    if (isPrimary()) {
      if (morphProgress > 0) {
        morphProgress -= dt;
        if (morphProgress < 0) {
          morphProgress = 0;
          morphDuration = defaultMorph;
          hoverDuration = defaultHover;
        }

        lat.set(targetGeoLoc.lat + (sourceGeoLoc.lat - targetGeoLoc.lat) *
                                       (morphProgress / morphDuration));
        lon.set(targetGeoLoc.lon + (sourceGeoLoc.lon - targetGeoLoc.lon) *
                                       (morphProgress / morphDuration));
        if (hoverDuration > 0) {
          if (morphProgress + hoverDuration > morphDuration) {
            radius.set(hoverHeight +
                       (sourceGeoLoc.radius - hoverHeight) *
                           (morphProgress - morphDuration + hoverDuration) /
                           hoverDuration);
          } else {
            radius.set(targetGeoLoc.radius +
                       (hoverHeight - targetGeoLoc.radius) *
                           (morphProgress / (morphDuration - hoverDuration)));
          }
        } else {
          radius.set(targetGeoLoc.radius +
                     (sourceGeoLoc.radius - targetGeoLoc.radius) *
                         (morphProgress / morphDuration));
        }

      } else {
        Vec3d pos = nav().pos();
        radius.setNoCalls(pos.mag());
        pos.normalize();
        lat.setNoCalls(asin(pos.y) * 180.0 / M_PI);
        lon.setNoCalls(atan2(-pos.x, -pos.z) * 180.0 / M_PI);
      }
      // Set light position
      light.pos(nav().pos().x, nav().pos().y, nav().pos().z);
      Light::globalAmbient({lux, lux, lux});

      state().global_pose.set(nav());
    } else {
      nav().set(state().global_pose);
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(0, 0, 0);
    g.culling(true);
    // g.cullFaceFront();
    g.texture();

    g.pushMatrix();

    skyTex.bind();
    g.translate(nav().pos());
    g.draw(skyMesh);
    skyTex.unbind();

    g.popMatrix();

    g.pushMatrix();

    sphereTex.bind();
    g.cullFaceFront();
    g.draw(sphereMesh); // only needed if we go inside the earth
    g.cullFaceBack();
    g.draw(sphereMesh);
    sphereTex.unbind();

    g.popMatrix();
  }

  bool onKeyDown(const Keyboard &k) override {
    switch (k.key()) {
    case '1':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 53.54123998879464;
      targetGeoLoc.lon = 9.950943100405375;
      targetGeoLoc.radius = 2.2;
      morphDuration = 4.0;
      morphProgress = morphDuration;
      hoverDuration = 0;
      return true;
    case '2':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 54.40820774011447;
      targetGeoLoc.lon = 12.593582740321027;
      targetGeoLoc.radius = 2.2;
      morphProgress = morphDuration;
      return true;
    case '3':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 53.91580380807132;
      targetGeoLoc.lon = 9.531183185073399;
      targetGeoLoc.radius = 2.2;
      morphProgress = morphDuration;
      return true;
    case '4':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 53.78527983917765;
      targetGeoLoc.lon = 9.409205631903566;
      targetGeoLoc.radius = 2.2;
      morphProgress = morphDuration;
      return true;
    case '5':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 0;
      targetGeoLoc.lon = -80;
      targetGeoLoc.radius = 30;
      morphDuration = 6.0;
      morphProgress = morphDuration;
      hoverDuration = 0;
      return true;
    case '0':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 0;
      targetGeoLoc.lon = 138;
      targetGeoLoc.radius = 0.01;
      morphProgress = morphDuration;
      hoverDuration = 0;
      return true;
    case 'r':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 0;
      targetGeoLoc.lon = 0;
      targetGeoLoc.radius = 5;
      morphProgress = morphDuration;
      hoverDuration = 0;
      return true;
    default:
      return false;
    }
  }
};

int main() {
  SensoriumApp app;
  app.dimensions(1200, 800);
  app.start();
}
