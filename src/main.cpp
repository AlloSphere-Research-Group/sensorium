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
static const int years = 11;    // Total number of years (2003~2013)
static const int stressors = 9; // Total number of stressors

struct State
{
  Pose global_pose;
  bool swtch[stressors]{false};
  bool molph{false};
  float lux;
  float year;
};

struct GeoLoc
{
  float lat;
  float lon;
  float radius;
};

struct SensoriumApp : public DistributedAppWithState<State>
{
  VAOMesh skyMesh, sphereMesh;
  Image skyImage, sphereImage;
  Texture skyTex, sphereTex;
  ControlGUI *gui;
  Parameter lat{"lat", "", 0.0, -90.0, 90.0};
  Parameter lon{"lon", "", 0.0, -180.0, 180.0};
  Parameter radius{"radius", "", 5.0, 0.0, 50.0};
  Parameter lux{"Light", 0.6, 0, 1};
  Parameter year{"Year", 2003, 2003, 2013};
  Parameter trans{"Trans", 0.99, 0.1, 1};
  GeoLoc sourceGeoLoc, targetGeoLoc;
  Image oceanData[years][stressors];
  double morphProgress{0.0};
  double morphDuration{5.0};
  const double defaultMorph{5.0};
  float hoverHeight{10.f};
  double hoverDuration{2.5};
  const double defaultHover{2.5};
  Light light;
  float earth_radius = 5;
  float point_dist = 1.01 * earth_radius;
  int data_W[stressors], data_H[stressors];
  Mesh pic[years][stressors];
  std::shared_ptr<CuttleboneDomain<State>> cuttleboneDomain;

  void onInit() override
  {
    cuttleboneDomain = CuttleboneDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain)
    {
      std::cerr << "ERROR: Could not start Cuttlebone" << std::endl;
      quit();
    }
  }

  void onCreate() override
  {
    lens().fovy(45).eyeSep(0);
    nav().pos(0, 0, -5);
    nav().quat().fromAxisAngle(0.5 * M_2PI, 0, 1, 0);

    addSphereWithTexcoords(skyMesh, 50, 50, true);
    skyMesh.update();

    addSphereWithTexcoords(sphereMesh, 2, 50, false);
    sphereMesh.update();

    std::string dataPath;

    if (sphere::isSphereMachine())
    {
      if (sphere::isRendererMachine())
      {
        dataPath = "/data/Sensorium/";
      }
      else
      {
        dataPath = "/Volumes/Data/Sensorium/";
      }
    }
    else
    {
      // dataPath = "C:/Users/kenny/data/sensorium/";
      dataPath = "data/";
    }

    // visible earth, nasa
    sphereImage = Image(dataPath + "blue_marble_brighter.jpg");
    if (sphereImage.array().size() == 0)
    {
      std::cerr << "failed to load sphere image" << std::endl;
    }

    // paulbourke.net
    skyImage = Image(dataPath + "Stellarium3.jpg");
    if (skyImage.array().size() == 0)
    {
      std::cerr << "failed to load background image" << std::endl;
    }

    sphereTex.create2D(sphereImage.width(), sphereImage.height());
    sphereTex.filter(Texture::LINEAR);
    sphereTex.submit(sphereImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

    skyTex.create2D(skyImage.width(), skyImage.height());
    skyTex.filter(Texture::LINEAR);
    skyTex.submit(skyImage.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

    if (isPrimary())
    {
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      gui = &guiDomain->newGUI();

      *gui << lat << lon << radius << lux << year << trans;
    }
    // enable if parameter needs to be shared
    // parameterServer() << lat << lon << radius;

    lat.registerChangeCallback([&](float value)
                               {
                                 nav().pos(Vec3d(-radius.get() * cos(value / 180.0 * M_PI) *
                                                     sin(lon.get() / 180.0 * M_PI),
                                                 radius.get() * sin(value / 180.0 * M_PI),
                                                 -radius.get() * cos(value / 180.0 * M_PI) *
                                                     cos(lon.get() / 180.0 * M_PI)));

                                 nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
                               });

    lon.registerChangeCallback([&](float value)
                               {
                                 nav().pos(Vec3d(-radius.get() * cos(lat.get() / 180.0 * M_PI) *
                                                     sin(value / 180.0 * M_PI),
                                                 radius.get() * sin(lat.get() / 180.0 * M_PI),
                                                 -radius.get() * cos(lat.get() / 180.0 * M_PI) *
                                                     cos(value / 180.0 * M_PI)));
                                 nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
                               });

    radius.registerChangeCallback([&](float value)
                                  {
                                    nav().pos(Vec3d(-value * cos(lat.get() / 180.0 * M_PI) *
                                                        sin(lon.get() / 180.0 * M_PI),
                                                    value * sin(lat.get() / 180.0 * M_PI),
                                                    -value * cos(lat.get() / 180.0 * M_PI) *
                                                        cos(lon.get() / 180.0 * M_PI)));
                                    nav().faceToward(Vec3d(0), Vec3d(0, 1, 0));
                                  });

    // Bring ocean data (image)
    // 0. SST
    std::cout << "Start loading CHI data " << std::endl;
    std::cout << "Start loading 0. SST" << std::endl;
    int stress = 0;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/sst/sst_05_" << d + 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      std::strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][0].primitive(Mesh::POINTS);
      // pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();

    // 1. Nutrients
    stress = 1;
    std::cout << "Start loading 1. Nutrients" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/nutrient/nutrient_pollution_impact_5_" << d + 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
 //     pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();

    // 2. Shipping
    stress = 2;
    std::cout << "Start loading 2. Shipping" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/ship/ship_impact_10_" << d + 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
 //     pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();

    // 3. Sea level rise
    stress = 3;
    std::cout << "Start loading 3. Sea level rise" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/slr/slr_impact_5_" << d + 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
 //     pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();
    // 4. Ocean Acidification
    stress = 4;
    std::cout << "Start loading 4. Ocean Acidification " << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/oa/oa_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
 //     pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();

    // 5. Fishing demersal low
    stress = 5;
    std::cout << "Start loading 5. Fishing demersal low " << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/fish/fdl_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
 //     pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();

    // 6. Fishing demersal high
    stress = 6;
    std::cout << "Start loading 6. Fishing demersal high " << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/fish/fdh_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
 //     pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();

    // 7. Fishing pelagic low
    stress = 7;
    std::cout << "Start loading 7. Fishing pelagic low" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/fish/fpl_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
 //     pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();

    // 8. Fishing pelagic high
    stress = 8;
    std::cout << "Start loading 8. Fishing pelagic high" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/fish/fph_100_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData[d][stress] = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
 //     pic[d][stress].update();
    }
    data_W[stress] = oceanData[0][stress].width();
    data_H[stress] = oceanData[0][stress].height();
    std::cout << "Loaded CHI data" << std::endl;

    // Assign color for data
    for (int p = 0; p < stressors; p++)
    {
      point_dist = 2.005 + 0.001 * p;
      for (int d = 0; d < years; d++)
      {
        for (int row = 0; row < data_H[p]; row++)
        {
          double theta = row * M_PI / data_H[p];
          double sinTheta = sin(theta);
          double cosTheta = cos(theta);
          for (int column = 0; column < data_W[p]; column++)
          {
            auto pixel = oceanData[d][p].at(column, data_H[p] - row - 1);

            if (pixel.r > 0)
            {
              // {
              double phi = column * M_2PI / data_W[p];
              double sinPhi = sin(phi);
              double cosPhi = cos(phi);

              double x = sinPhi * sinTheta;
              double y = -cosTheta;
              double z = cosPhi * sinTheta;

              pic[d][p].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              if (p == 0) // sst color
                pic[d][p].color(HSV(0.55 + log(pixel.r / 70. + 1), 0.65 + pixel.r / 60, 0.6 + atan(pixel.r / 300)));
              else if (p == 1) // nutrient pollution color
                pic[d][p].color(HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 + pixel.r / 90));
              else if (p == 2) // shipping color
                pic[d][p].color(HSV(1 - log(pixel.r / 30. + 1), 0.6 + pixel.r / 100, 0.6 + pixel.r / 60));
              else if (p == 3) // sea level rise color
                // pic[d][p].color(HSV(0.6 + log(pixel.r/60. + 1), 0.96+log(pixel.r/60.+0.1), 0.98+log(pixel.r/60.+0.1)));
                pic[d][p].color(HSV(0.6 + 0.2 * log(pixel.r / 100. + 1), 0.6 + log(pixel.r / 60. + 1), 0.6 + log(pixel.r / 60. + 1)));
              else if (p == 4) // Ocean Acidification
                // pic[d][p].color(HSV(0.7-log(pixel.r/100.+ 0.1), 0.6+log(pixel.r/100.+0.1), 0.8+log(pixel.r/200.+1)));
                pic[d][p].color(HSV(0.7 - 0.6 * log(pixel.r / 100. + 1), 0.5 + log(pixel.r / 100. + 1), 1));
              else if (p == 5) // Fishing demersal low
                pic[d][p].color(HSV(log(pixel.r / 90. + 1), 0.9, 1));
              else if (p == 6) // Fishing demersal high
                pic[d][p].color(HSV(log(pixel.r / 90. + 1), 0.9, 1));
              else if (p == 7) // Fishing pelagic low
                pic[d][p].color(HSV(log(pixel.r / 90. + 1), 0.9, 1));
              else if (p == 8) // Fishing pelagic high
                pic[d][p].color(HSV(log(pixel.r / 90. + 1), 0.9, 1));
            }
          }
        }
      }
    }
  }

  void onAnimate(double dt) override
  {
    if (isPrimary())
    {
      Vec3f point_you_want_to_see = Vec3f(0, 0, 0); // examplary point that you want to see
      nav().faceToward(point_you_want_to_see, Vec3f(0, 1, 0), 0.7);
      if (morphProgress > 0)
      {
        morphProgress -= dt;
        if (morphProgress < 0)
        {
          morphProgress = 0;
          morphDuration = defaultMorph;
          hoverDuration = defaultHover;
        }

        lat.set(targetGeoLoc.lat + (sourceGeoLoc.lat - targetGeoLoc.lat) *
                                       (morphProgress / morphDuration));
        lon.set(targetGeoLoc.lon + (sourceGeoLoc.lon - targetGeoLoc.lon) *
                                       (morphProgress / morphDuration));
        if (hoverDuration > 0)
        {
          if (morphProgress + hoverDuration > morphDuration)
          {
            radius.set(hoverHeight +
                       (sourceGeoLoc.radius - hoverHeight) *
                           (morphProgress - morphDuration + hoverDuration) /
                           hoverDuration);
          }
          else
          {
            radius.set(targetGeoLoc.radius +
                       (hoverHeight - targetGeoLoc.radius) *
                           (morphProgress / (morphDuration - hoverDuration)));
          }
        }
        else
        {
          radius.set(targetGeoLoc.radius +
                     (sourceGeoLoc.radius - targetGeoLoc.radius) *
                         (morphProgress / morphDuration));
        }
      }
      else
      {
        Vec3d pos = nav().pos();
        radius.setNoCalls(pos.mag());
        pos.normalize();
        lat.setNoCalls(asin(pos.y) * 180.0 / M_PI);
        lon.setNoCalls(atan2(-pos.x, -pos.z) * 180.0 / M_PI);
      }
      // Set light position
      light.pos(nav().pos().x, nav().pos().y, nav().pos().z);
      Light::globalAmbient({lux, lux, lux});
      state().lux = lux;
      if (state().molph)
      {
        year = year + 3 * dt;
        if (year > 2013)
        {
          year = 2013;
        }
      }
      state().global_pose.set(nav());
      state().year = year;
    }    // prim end
    else // renderer
    {
      nav().set(state().global_pose);
      light.pos(nav().pos().x, nav().pos().y, nav().pos().z);
      Light::globalAmbient({state().lux, state().lux, state().lux});
    }
  }

  void onDraw(Graphics &g) override
  {
    g.clear(0, 0, 0);
    g.culling(true);
    // g.cullFaceFront();
    g.lighting(true);
    g.light(light);
    g.texture();
    g.depthTesting(true);
    g.blending(true);
    g.blendTrans();

    // sky
    g.pushMatrix();
    skyTex.bind();
    g.translate(nav().pos());
    g.draw(skyMesh);
    skyTex.unbind();

    g.popMatrix();

    // sphere
    g.pushMatrix();
    sphereTex.bind();
    g.cullFaceFront();
    g.draw(sphereMesh); // only needed if we go inside the earth
    g.cullFaceBack();
    g.draw(sphereMesh);
    sphereTex.unbind();

    g.popMatrix();

    // Draw data
    for (int j = 0; j < stressors; j++)
    {
      if (state().swtch[j])
      {
        g.meshColor();
        g.blendTrans();
        g.pushMatrix();
        float ps = 50 / nav().pos().magSqr();
        if (ps > 5)
        {
          ps = 5;
        }
        g.pointSize(ps);
        g.draw(pic[(int)state().year - 2003][j]); // only needed if we go inside the earth
        g.popMatrix();
      }
    }
  }

  bool onKeyDown(const Keyboard &k) override
  {
    switch (k.key())
    {
    case '1':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 53.54123998879464;
      targetGeoLoc.lon = 9.950943100405375;
      targetGeoLoc.radius = 3.2;
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
      targetGeoLoc.radius = 3.2;
      morphProgress = morphDuration;
      return true;
    case '3':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 53.91580380807132;
      targetGeoLoc.lon = 9.531183185073399;
      targetGeoLoc.radius = 3.2;
      morphProgress = morphDuration;
      return true;
    case '4':
      sourceGeoLoc.lat = lat.get();
      sourceGeoLoc.lon = lon.get();
      sourceGeoLoc.radius = radius.get();
      targetGeoLoc.lat = 53.78527983917765;
      targetGeoLoc.lon = 9.409205631903566;
      targetGeoLoc.radius = 3.2;
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
    case 'u':
      state().swtch[0] = !state().swtch[0];
      return true;
    case 'i':
      state().swtch[1] = !state().swtch[1];
      return true;
    case 'o':
      state().swtch[2] = !state().swtch[2];
      return true;
    case 'j':
      state().swtch[3] = !state().swtch[3];
      return true;
    case 'k':
      state().swtch[4] = !state().swtch[4];
      return true;
    case 'l':
      state().swtch[5] = !state().swtch[5];
      return true;
    case 'm':
      state().swtch[6] = !state().swtch[6];
      return true;
    case ',':
      state().swtch[7] = !state().swtch[7];
      return true;
    case '.':
      state().swtch[8] = !state().swtch[8];
      return true;
    case '9':
      state().molph = !state().molph;
      state().year = 2003;
      return true;
    default:
      return false;
    }
  }
};

int main()
{
  SensoriumApp app;
  app.dimensions(1200, 800);
  app.start();
}
