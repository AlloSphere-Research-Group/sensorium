#include <iostream>

#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Image.hpp"

using namespace al;

struct State {
  Pose pose;
};

struct MyApp : DistributedAppWithState<State> {
  VAOMesh sphereMesh;
  Texture sphereTex;
  double phase = 0;
  float aspectRatio{1.f};

  void onCreate() override {
    addSphereWithTexcoords(sphereMesh, 2, 50, false);
    sphereMesh.update();
    lens().near(0.1).far(25).fovy(45);
    nav().pos(0, 0, 4);
    nav().quat().fromAxisAngle(0. * M_2PI, 0, 1, 0);

    std::string filename =
        "C:/Users/kenny/data/sensorium/land_shallow_topo_21600_brighter.jpg";
    auto imageData = Image(filename);

    if (imageData.array().size() == 0) {
      std::cout << "failed to load image " << filename << std::endl;
    }
    std::cout << "loaded image size: " << imageData.width() << ", "
              << imageData.height() << std::endl;

    sphereTex.create2D(imageData.width(), imageData.height());
    sphereTex.submit(imageData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

    sphereTex.filter(Texture::LINEAR);
    aspectRatio = imageData.width() / (float)imageData.height();
  }

  void onAnimate(double dt) override {
    double period = 100;
    phase += dt / period;
    if (phase >= 1.)
      phase -= 1.;
  }

  void onDraw(Graphics &g) override {
    g.clear(0, 0, 0);
    g.culling(true);
    // g.cullFaceFront();
    g.texture();
    // g.polygonLine();
    g.pushMatrix();
    g.rotate(phase * 360, 0, 1, 0);
    sphereTex.bind();
    g.cullFaceFront();
    g.draw(sphereMesh); // only needed if we go inside the earth
    g.cullFaceBack();
    g.draw(sphereMesh);
    sphereTex.unbind();
    g.popMatrix();
  }
};

int main() {
  MyApp app;
  app.dimensions(1920, 1080);
  app.start();
}
