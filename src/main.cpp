#include <iostream>

#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Image.hpp"

using namespace al;

struct GeoLoc {
  float lat;
  float lon;
  float radius;
};

class SensoriumApp : public DistributedApp {
public:
  VAOMesh skyMesh, sphereMesh;
  Image skyImage, sphereImage;
  Texture skyTex, sphereTex;

  ControlGUI *gui;
  Parameter lat{"lat", "", 0.0, -90.0, 90.0};
  Parameter lon{"lon", "", 0.0, -180.0, 180.0};
  Parameter radius{"radius", "", 5.0, 2.1, 50.0};

  ParameterPose pose{"pose", ""};

  GeoLoc sourceGeoLoc, targetGeoLoc;
  double morphProgress{0.0}, morphDuration{2.0};

  void onCreate() override {
    lens().near(0.1).far(100).fovy(45);
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
      dataPath = "C:/Users/kenny/data/sensorium/";
    }

    sphereImage = Image(dataPath + "land_shallow_topo_21600_brighter.jpg");
    skyImage = Image(dataPath + "milkyway.png");

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

    parameterServer() << lat << lon << radius << pose;

    lat.registerChangeCallback([&](float value) {
      nav().pos(Vec3d(radius.get() * cos(value / 180.0 * M_PI) *
                          sin(lon.get() / 180.0 * M_PI),
                      radius.get() * sin(value / 180.0 * M_PI),
                      -radius.get() * cos(value / 180.0 * M_PI) *
                          cos(lon.get() / 180.0 * M_PI)));

      nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });

    lon.registerChangeCallback([&](float value) {
      nav().pos(Vec3d(radius.get() * cos(lat.get() / 180.0 * M_PI) *
                          sin(value / 180.0 * M_PI),
                      radius.get() * sin(lat.get() / 180.0 * M_PI),
                      -radius.get() * cos(lat.get() / 180.0 * M_PI) *
                          cos(value / 180.0 * M_PI)));
      nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });

    radius.registerChangeCallback([&](float value) {
      nav().pos(Vec3d(value * cos(lat.get() / 180.0 * M_PI) *
                          sin(lon.get() / 180.0 * M_PI),
                      value * sin(lat.get() / 180.0 * M_PI),
                      -value * cos(lat.get() / 180.0 * M_PI) *
                          cos(lon.get() / 180.0 * M_PI)));
      nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
    });
  }

  void onAnimate(double dt) override {
    if (isPrimary()) {
      if (morphProgress > 0) {
        morphProgress -= dt;
        if (morphProgress < 0)
          morphProgress = 0;

        lat.set(targetGeoLoc.lat + (sourceGeoLoc.lat - targetGeoLoc.lat) *
                                       (morphProgress / morphDuration));
        lon.set(targetGeoLoc.lon + (sourceGeoLoc.lon - targetGeoLoc.lon) *
                                       (morphProgress / morphDuration));
        radius.set(targetGeoLoc.radius +
                   (sourceGeoLoc.radius - targetGeoLoc.radius) *
                       (morphProgress / morphDuration));
      } else {
        Vec3d pos = nav().pos();
        radius.setNoCalls(pos.mag());
        pos.normalize();
        lat.setNoCalls(asin(pos.y) * 180.0 / M_PI);
        lon.setNoCalls(atan2(pos.x, -pos.z) * 180.0 / M_PI);
        // if (radius.get() > 2.0)
        // nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
      }

      pose.set(nav());
    } else {
      nav().set(pose.get());
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
    if (k.key() == '1') {
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 53.54123998879464;
      targetGeoLoc.lon = 9.950943100405375;
      targetGeoLoc.radius = 2.2;
      morphProgress = morphDuration;
      return true;
    }
    return false;
  }
};

int main() {
  SensoriumApp app;
  app.dimensions(1920, 1080);
  app.start();
}
