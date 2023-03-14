// Sensorium Main
// TODO :
// - gradually fade in-out stressors


#include "headers.hpp"
using namespace al;
using namespace std;
using namespace gam;

static const int years = 11;       // Total number of years (2003~2013)
static const int stressors = 12;   // Total number of stressors
static const int num_cloud = 3;    // Total number of stressors
static const int num_county = 187; // Total number of stressors


struct State
{
  Pose global_pose;
  bool swtch[stressors]{false};
  bool cloud_swtch[num_cloud]{false};
  bool molph{false};
  bool co2_swtch{false};
  float lux;
  float year;
  float radius;
  int osc_click[10];
  // Mesh emission_mesh;
  Vec3f render_co2_pos[500];
  Color render_co2_col[500];
};

struct GeoLoc
{
  float lat;
  float lon;
  float radius;
};

typedef struct
{
  double val[years + 2];
} co2Types;

// CO2 emission parts

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
template <int N>

struct Emitter {
  Particle particles[N];
  int tap = 0;

  Emitter() {
    for (auto& p : particles) p.age = N;
  }

  template <int M>
  void update() {
    for (auto& p : particles) p.update(M);

    for (int i = 0; i < M; ++i) {
      auto& p = particles[tap];
      { // co2 particle spread speed
        p.vel.set(al::rnd::gaussian()* 0.003, al::rnd::gaussian() * 0.003,
                  al::rnd::gaussian()* 0.003);
        p.acc.set(al::rnd::gaussian()*0.0003, al::rnd::gaussian()*0.0003, al::rnd::gaussian()*0.0003);
      } 
      p.pos.set(al::rnd::gaussian()*1, al::rnd::gaussian()*1, 0);
      p.age = 0;
      ++tap;
      if (tap >= N) tap = 0;
    }
  }

  int size() { return N; }
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
  Parameter lux{"Light", 0.6, 0, 2.5};
  Parameter year{"Year", 2003, 2003, 2013};
  // Parameter trans{"Trans", 0.99, 0.1, 1};
  Parameter gain{"Audio", 0, 0, 2};
  ParameterBool s_ci{"Cumulative impacts", "", 0.0};
  ParameterBool s_oc{"Organic chemical pollution", "", 0.0};
  ParameterBool s_np{"Nutrient pollution", "", 0.0};
  ParameterBool s_dh{"Direct human", "", 0.0};
  ParameterBool s_slr{"Sea level rise", "", 0.0};
  ParameterBool s_oa{"Ocean acidification", "", 0.0};
  ParameterBool s_sst{"Sea surface temperature", "", 0.0};
  ParameterBool s_cf_pl{"Fishing - Pelagic low-bycatch", "", 0.0};
  ParameterBool s_cf_ph{"Fishing - Pelagic high-bycatch", "", 0.0};
  ParameterBool s_cf_dl{"Fishing - Demersal non-destructive high-bycatch", "", 0.0};
  ParameterBool s_cf_dh{"Fishing - Demersal non-desctructive low-bycatch", "", 0.0};
  ParameterBool s_cf_dd{"Fishing - Demersal destructive", "", 0.0};
  ParameterBool a_f{"Artisanal fishing", "", 0.0};
  ParameterBool s_shp{"Shipping", "", 0.0};
  ParameterBool s_cloud{"Clouds", "", 0.0};
  ParameterBool s_cloud_storm{"Clouds - Storm", "", 0.0};
  ParameterBool s_cloud_eu{"Clouds - EU", "", 0.0};
  ParameterBool s_co2{"CO2", "", 0.0};
  ParameterBool s_nav{"Explore Globe", "", 0.0};
  ParameterBool s_years{"2003 - 2013", "", 0.0};
  bool faceTo = true;
  GeoLoc sourceGeoLoc, targetGeoLoc;
  // Image oceanData[years][stressors];
  // Image cloudData[num_cloud];
  double morphProgress{0.0};
  double morphDuration{5.0};
  const double defaultMorph{5.0};
  float hoverHeight{10.f};
  double hoverDuration{2.5};
  const double defaultHover{2.5};
  Light light;
  float earth_radius = 5;
  float point_dist = 1.01 * earth_radius;
  Color data_color;

  VAOMesh pic[years][stressors];
  VAOMesh cloud[num_cloud], co2_mesh[num_county];
  // VAOMesh cloud[num_cloud];
  // Mesh co2_mesh[num_county];
  float nation_lat[num_county], nation_lon[num_county];
  float morph_year;
  std::shared_ptr<CuttleboneDomain<State>> cuttleboneDomain;
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
  Texture texture; // co2
  ShaderProgram shader; //co2

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

  void onInit() override
  {
    cuttleboneDomain = CuttleboneDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain)
    {
      std::cerr << "ERROR: Could not start Cuttlebone" << std::endl;
      quit();
    }
    // OSC receiver
    // server.open(4444,"0.0.0.0", 0.05);
    // server.handler(oscDomain()->handler());
    // server.start();

// Import CO2 data
    // CSV columms label: lat + long + years (2003~2013)
    for (int k = 0; k < years + 2; k++)
    {
      reader.addType(CSVReader::REAL);
    }
    reader.readFile("data/co2/2003_2013.csv");
    std::vector<co2Types> co2_rows = reader.copyToStruct<co2Types>();
    // to test the csv import . values supposed to be: co2_row.val[-]: first two column (lat, long) + years data
    // year data : co2_row.val[2 ~ :]
    // for (auto co2_row : co2_rows) {
    //   cout << co2_row.val[0] << endl;
    // }
    // convert co2 lat lon to xyz
    int nation_id = 0;
    for (auto co2_row : co2_rows)
    {
      float lat = co2_row.val[0];
      float lon = co2_row.val[1];
      co2_pos[nation_id] = Vec3f(-cos(lat / 180.0 * M_PI) * sin(lon / 180.0 * M_PI),
                                 sin(lat / 180.0 * M_PI),
                                 -cos(lat / 180.0 * M_PI) * cos(lon / 180.0 * M_PI));
      for (int i = 0; i < years; i++){
        co2_level[nation_id][i] = co2_row.val[2+i];
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
    for (int d = 0; d < num_cloud; d++)
    {
      ostringstream ostr;
      ostr << "data/cloud/" << d << ".jpg"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      cloudData = Image(filename);
      cloud[d].primitive(Mesh::POINTS);
    }
    // Assign color for cloud
    for (int p = 0; p < num_cloud; p++)
    {
      cloud_W = cloudData.width();
      cloud_H = cloudData.height();
      point_dist = 2.015 + 0.001 * p;
      for (int row = 0; row < cloud_H; row++)
      {
        double theta = row * M_PI / cloud_H;
        double sinTheta = sin(theta);
        double cosTheta = cos(theta);
        for (int column = 0; column < cloud_W; column++)
        {
          auto pixel = cloudData.at(column, cloud_H - row - 1);
          if (pixel.r > 10)
          {
            // {
            double phi = column * M_2PI / cloud_W;
            double sinPhi = sin(phi);
            double cosPhi = cos(phi);

            double x = sinPhi * sinTheta;
            double y = -cosTheta;
            double z = cosPhi * sinTheta;

            cloud[p].vertex(x * point_dist, y * point_dist, z * point_dist);
            // init color config
            // end of assigning colors for data
            cloud[p].color(Color(log(pixel.r / 50. + 3)));
          }
        }
      }
      cloud[p].update();
    }
  }

  void onCreate() override
  {
    Image oceanData;
    int data_W, data_H;

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
    // sphereImage = Image(dataPath + "blue_marble_brighter.jpg");
    sphereImage = Image(dataPath + "16k.jpg");
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

      std::string displayText = "AlloOcean. Ocean stressor from Cumulative Human Impacts (2003-2013)";
      // *gui << lat << lon << radius << lux << year << gain;
      *gui << year; 
      *gui << s_years << s_nav; 
      *gui << s_ci << s_oc << s_np << s_dh << s_slr << s_oa << s_sst;
      *gui << s_cf_pl << s_cf_ph << s_cf_dl << s_cf_dh << s_shp;
      *gui << s_cloud << s_cloud_storm << s_cloud_eu << s_co2;
      // *gui << s_cf_dd << a_f // currently we don't have this data
      // *gui << s_ci << s_oc << s_np;

      // *gui << lat << lon << radius << lux << year << trans << gain;
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

                                 nav().faceToward(Vec3d(0), Vec3d(0, 1, 0)); });

    lon.registerChangeCallback([&](float value)
                               {
                                 nav().pos(Vec3d(-radius.get() * cos(lat.get() / 180.0 * M_PI) *
                                                     sin(value / 180.0 * M_PI),
                                                 radius.get() * sin(lat.get() / 180.0 * M_PI),
                                                 -radius.get() * cos(lat.get() / 180.0 * M_PI) *
                                                     cos(value / 180.0 * M_PI)));
                                 nav().faceToward(Vec3d(0), Vec3d(0, 1, 0)); });

    radius.registerChangeCallback([&](float value)
                                  {
                                    nav().pos(Vec3d(-value * cos(lat.get() / 180.0 * M_PI) *
                                                        sin(lon.get() / 180.0 * M_PI),
                                                    value * sin(lat.get() / 180.0 * M_PI),
                                                    -value * cos(lat.get() / 180.0 * M_PI) *
                                                        cos(lon.get() / 180.0 * M_PI)));
                                    nav().faceToward(Vec3d(0), Vec3d(0, 1, 0)); });

    s_years.registerChangeCallback([&](int value){
      if (value){
          state().molph = !state().molph;
          year = 2003;
          s_years.set(0);
      } 
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
      oceanData = Image(filename);
      pic[d][0].primitive(Mesh::POINTS);

      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(0.55 + log(pixel.r / 90. + 1), 0.65 + pixel.r / 60, 0.6 + atan(pixel.r / 300));
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 1. Nutrients
    stress = 1;
    std::cout << "Start loading 1. Nutrients" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/nutrient/nutrient_pollution_impact_5_" << d + 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 + pixel.r / 90);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 2. Shipping
    stress = 2;
    std::cout << "Start loading 2. Shipping" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/ship/ship_impact_10_" << d + 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(1 - log(pixel.r / 30. + 1), 0.6 + pixel.r / 100, 0.6 + pixel.r / 60);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 3. Ocean Acidification
    stress = 3;
    std::cout << "Start loading 3. Ocean Acidification " << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/oa/oa_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(0.7 - 0.6 * log(pixel.r / 100. + 1), 0.5 + log(pixel.r / 100. + 1), 1);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 4. Sea level rise
    stress = 4;
    std::cout << "Start loading 4. Sea level rise" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/slr/slr_impact_5_" << d + 2003 << "_equi.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(0.6 + 0.2 * log(pixel.r / 100. + 1), 0.6 + log(pixel.r / 60. + 1), 0.6 + log(pixel.r / 60. + 1));
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 5. Fishing demersal low
    stress = 5;
    std::cout << "Start loading 5. Fishing demersal low " << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/fish/fdl_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 6. Fishing demersal high
    stress = 6;
    std::cout << "Start loading 6. Fishing demersal high " << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/fish/fdh_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 7. Fishing pelagic low
    stress = 7;
    std::cout << "Start loading 7. Fishing pelagic low" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/fish/fpl_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 8. Fishing pelagic high
    stress = 8;
    std::cout << "Start loading 8. Fishing pelagic high" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/fish/fph_100_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 9. Direct human
    stress = 9;
    std::cout << "Start loading 9. Direct human" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/dh/dh_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(log(pixel.r / 120. + 1), 0.9, 1);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 10. Organic chemical
    stress = 10;
    std::cout << "Start loading 10. Organic chemical" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/oc/oc_10_" << d + 2003 << "_impact.png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(log(pixel.r / 120. + 1), 0.9, 1);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    // 11. Cumulative human impacts
    stress = 11;
    std::cout << "Start loading 11. Cumulative human impacts" << std::endl;
    for (int d = 0; d < years; d++)
    {
      ostringstream ostr;
      ostr << "data/chi/chi/cumulative_impact_10_" << d + 2003 << ".png"; // ** change stressor
      char *filename = new char[ostr.str().length() + 1];
      strcpy(filename, ostr.str().c_str());
      oceanData = Image(filename);
      pic[d][stress].primitive(Mesh::POINTS);
      data_W = oceanData.width();
      data_H = oceanData.height();
      point_dist = 2.002 + 0.001 * stress;
      for (int row = 0; row < data_H; row++)
        {
          float theta = row * M_PI / data_H;
          float sinTheta = sin(theta);
          float cosTheta = cos(theta);
          for (int column = 0; column < data_W; column++)
          {
            auto pixel = oceanData.at(column, data_H - row - 1);
            if (pixel.r > 0)
            {
              // {
              float phi = column * M_2PI / data_W;
              float sinPhi = sin(phi);
              float cosPhi = cos(phi);

              float x = sinPhi * sinTheta;
              float y = -cosTheta;
              float z = cosPhi * sinTheta;
              // TODO: This can be preprocessed for shorting the load time - ML
              pic[d][stress].vertex(x * point_dist, y * point_dist, z * point_dist);
              // init color config
              data_color = HSV(log(pixel.r / 120. + 1), 0.9, 1);
              pic[d][stress].color(data_color);
            }
          }
        }
        pic[d][stress].update();
    }
    std::cout << "Loaded CHI data. Preprocessed" << std::endl;
    
    // audio
    // filter
    mFilter.zero();
    mFilter.res(1);
    mFilter.type(LOW_PASS);
    reverb.bandwidth(0.6f); // Low-pass amount on input, in [0,1]
    reverb.damping(0.5f);   // High-frequency damping, in [0,1]
    reverb.decay(0.6f);     // Tail decay factor, in [0,1]

//////////////// Shader
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
        m *= pow(2, 15) - 1;  // scale by the largest positive short int
        alpha[j * Nx + i] = m;
      }
    }
    texture.submit(&alpha[0]);

    // compile and link the three shaders
    //
    shader.compile(vertex, fragment, geometry);
////////////


  }

  void onAnimate(double dt) override
  {
    timer += dt;
    if (isPrimary())
    {
      Vec3f point_you_want_to_see = Vec3f(0, 0, 0); // examplary point that you want to see
      if(faceTo)
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
      // To smooth transition between the years, use alpha value to be updated using varying time
      // morph_year = year - floor(year);
      // if (morph_year)
      // {
      //   for (int p = 0; p < stressors; p++)
      //   {
      //     for (int d = 0; d < years; d++)
      //     {
      //       data_color[d][p].a = year - floor(year);
      //       pic[d][p].color(data_color[d][p]);
      //     }
      //   }
      // }
      // CO2 Emission animate
      emission.update<5>();
      emission_mesh.reset();
      emission_mesh.primitive(Mesh::POINTS);
      for (int i = 0; i < emission.size(); ++i) {
        Particle& p = emission.particles[i];
        float age = float(p.age) / emission.size();

        emission_mesh.vertex(p.pos);
        // emission_mesh.color(HSV(al::rnd::uniform(1.), al::rnd::uniform(0.7), (1 - age) * 0.8));
        p.col = HSV(al::rnd::uniform(0.2), al::rnd::uniform(0.1, 0.3), (1 - 0.1*age) * 0.5);
        emission_mesh.color(p.col);
      }

      // Set light position
      light.pos(nav().pos().x, nav().pos().y, nav().pos().z);
      Light::globalAmbient({lux, lux, lux});
      state().lux = lux;
      if (state().molph)
      {
        year = year + 3 * dt;
        if (year == 2013)
        {
          year = 2013;
          state().molph = false;
          s_years.set(0);
        }
      }
      if (s_nav){
        nav().moveR(0.003);        
      }
      //  audio
      mFilter.freq(30 * (1 + 10 / (radius + 3)) * (year - 2000));
      // mFilter.res();
      mFilter.type(LOW_PASS);
      reverb.decay(0.6f + 0.3 / (radius + 1)); // Tail decay factor, in [0,1]

      // Assign shared states for renderers
      state().global_pose.set(nav());
      state().year = year;
      state().radius = radius;
      state().swtch[0] = s_sst;
      state().swtch[1] = s_np;
      state().swtch[2] = s_shp;
      state().swtch[3] = s_oa;
      state().swtch[4] = s_slr;
      state().swtch[5] = s_cf_dl;
      state().swtch[6] = s_cf_dh;
      state().swtch[7] = s_cf_pl;
      state().swtch[8] = s_cf_ph;
      state().swtch[9] = s_dh;
      state().swtch[10] = s_oc;
      state().swtch[11] = s_ci;
      state().cloud_swtch[0] = s_cloud;
      state().cloud_swtch[1] = s_cloud_eu;
      state().cloud_swtch[2] = s_cloud_storm;
      state().co2_swtch = s_co2;
      for (int i = 0; i < emission.size(); ++i) {
        Particle& p = emission.particles[i];
        state().render_co2_pos[i] = p.pos;
        state().render_co2_col[i] = p.col;
      }
    }    // prim end
    else // renderer
    {
      nav().set(state().global_pose);
      light.pos(nav().pos().x, nav().pos().y, nav().pos().z);
      Light::globalAmbient({state().lux, state().lux, state().lux});
      emission_mesh.reset();
      emission_mesh.primitive(Mesh::POINTS);
      for (int i = 0; i < emission.size(); ++i) {
        Particle& p = emission.particles[i];
        float age = float(p.age) / emission.size();
        emission_mesh.vertex(state().render_co2_pos[i]);
        emission_mesh.color(state().render_co2_col[i]);        
      }

    }
  }
  void onSound(AudioIOData &io) override
  {
    while (io())
    {
      // wave.freq( (2 + mNoise()) * (1+10/radius) * (year-2000) ) ;
      env.freq(0.003 * (year - 1980));
      float wave_out = mFilter(mNoise() * gain.get() * env());
      float wet1, wet2;
      reverb(wave_out, wet1, wet2);
      io.out(0) = wet1;
      io.out(1) = wet2;
    }
  }

  void onDraw(Graphics &g) override
  {
    g.clear(0);
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
        // g.blendTrans();
        g.pushMatrix();
        float ps = 50 / nav().pos().magSqr();
        if (ps > 7)
        {
          ps = 7;
        }
        g.pointSize(ps);
        // Update data pose when nav is inside of the globe
        if (state().radius < 2)
        {
          g.scale(0.9);
        }
        else
        {
          g.scale(1);
        }
        g.draw(pic[(int)state().year - 2003][j]); // only needed if we go inside the earth
        g.popMatrix();
      }
    }
    // draw cloud
    for (int j = 0; j < num_cloud; j++)
    {
      if (state().cloud_swtch[j])
      {
        g.meshColor();
        // g.blendTrans();
        g.pushMatrix();
        float ps = 100 / nav().pos().magSqr();
        if (ps > 7)
        {
          ps = 7;
        }
        g.pointSize(0.4);
        g.draw(cloud[j]); // only needed if we go inside the earth
        g.popMatrix();
      }
    }
    // Draw CO2
    if (state().co2_swtch)
    {
      texture.bind();
      g.meshColor();
      g.blendTrans();
      g.blending(true);
      g.depthTesting(true);
      for (int nation = 0; nation < num_county; nation++)
      {
        float co2 = co2_level[nation][(int)state().year - 2003] * 0.000001; // precompute micro quantity since it's large number
        g.pushMatrix();
        g.translate(co2_pos[nation]*2.01);
        // g.translate(0,0,3);
        g.scale(co2*0.01, co2*0.01 , co2*0.01);
        g.rotate(-90, Vec3f(0,1,0));
        g.rotate(nation_lon[nation], Vec3f(0,1,0));
        g.rotate(nation_lat[nation], Vec3f(1,0,1));
        g.pointSize(co2 * 0.1*log2(1+co2) / radius.get());
        g.shader(shader);
        float halfSize = 0.005*co2;
        g.shader().uniform("halfSize", halfSize < 0.01 ? 0.01 : halfSize);
        g.pointSize(co2);
        g.draw(emission_mesh);
        g.popMatrix();
      }
      texture.unbind();
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
    case '=':
      faceTo = !faceTo;
      return true;
    case '9':
      state().molph = !state().molph;
      year = 2003;
      return true;
    case '8':
      for (int i = 0; i < stressors; i++)
        state().swtch[i] = false;
      return true;
    default:
      return false;
    }
  }

  // void onMessage(osc::Message& m) override {
  // }
};

int main()
{
  SensoriumApp app;
  app.dimensions(1200, 800);
  app.start();
}
