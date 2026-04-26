#include <cstdlib>

#include "initialization.h"
#include "iomanager.h"
#include "lidaryml.h"
#include "log.h"
#include "shape.h"
#include "visualize.h"
#include "yml.h"

struct BookshelfConfig
{
  int rowNum           = 100;
  int siteNum          = 1000;
  int coordinateOrigin = 459;
  int SubrowOrigin     = 459;
  int siteHeight       = 12;
  int siteWidth        = 1;
  int siteSpacing      = 1;
  int halo             = 0;
  int halo_x           = 0;
  int halo_y           = 0;
  int legalizeSpacing  = 20;
};

void loadBookshelfConfig(const std::string& path, BookshelfConfig& config)
{
  Yml configYml  = Yml(path);
  config.rowNum  = configYml.getNode("placement.rowNum").as<int>();
  config.siteNum = configYml.getNode("placement.siteNum").as<int>();
  config.coordinateOrigin
      = configYml.getNode("placement.coordinateOrigin").as<int>();
  config.SubrowOrigin = configYml.getNode("placement.SubrowOrigin").as<int>();
  config.siteHeight   = configYml.getNode("placement.siteHeight").as<int>();
  config.siteWidth    = configYml.getNode("placement.siteWidth").as<int>();
  config.siteSpacing  = configYml.getNode("placement.siteSpacing").as<int>();
  config.halo         = configYml.getNode("placement.halo").as<int>();
  config.halo_x       = configYml.getNode("placement.halo_x").as<int>();
  config.halo_y       = configYml.getNode("placement.halo_y").as<int>();
  config.legalizeSpacing
      = configYml.getNode("placement.legalizeSpacing").as<int>();
}

void checkBookshelfConfig(const std::shared_ptr<Design>& design,
                          const BookshelfConfig&         config)
{
  std::shared_ptr<Rect> rect
      = std::dynamic_pointer_cast<Rect>(design->getShape());
  if (rect) {
    int lowerLeftX  = rect->getShape().min_corner().x();
    int lowerLeftY  = rect->getShape().min_corner().y();
    int upperRightX = rect->getShape().max_corner().x();
    int upperRightY = rect->getShape().max_corner().y();

    int llx = config.SubrowOrigin;
    int lly = config.coordinateOrigin;
    int urx = llx + config.siteNum * config.siteWidth;
    int ury = lly + config.rowNum * config.siteHeight;
    OPDB::logger->info(
        "[BookshelfConfig][Debug] designRect: LL=({}, {}), UR=({}, {}) | "
        "bookshelf: LL=({}, {}), UR=({}, {})",
        lowerLeftX,
        lowerLeftY,
        upperRightX,
        upperRightY,
        llx,
        lly,
        urx,
        ury);
    bool condition1 = lowerLeftX > llx;
    bool condition2 = lowerLeftY > lly;
    bool condition3 = upperRightX < urx;
    bool condition4 = upperRightY < ury;
    if (condition1 || condition2 || condition3 || condition4) {
      OPDB::logger->error(
          "[BookshelfConfig] The provided bookshelf configuration is invalid.");
      exit(1);
    } else {
      OPDB::logger->info(
          "[BookshelfConfig] The provided bookshelf configuration is valid.");
    }
  } else {
    OPDB::logger->info(
        "[BookshelfConfig] The shape of the design is not rectangle.");
    exit(1);
  }
}

void callDREAMPlace(const std::string& configPath)
{
  std::string cmd
      = "python ../third-party/DREAMPlace/install/dreamplace/Placer.py "
        + configPath;
  int ret = std::system(cmd.c_str());
}

void callLiDAR(const std::string& ymlPath,
               const std::string& configPath,
               const std::string& outputPath)
{
  std::string cmd
      = "python ../third-party/LiDAR/src/picroute/main/picroute.py "
        "--benchmark="
        + ymlPath + " --config=" + configPath
        + " --run.output_layout_gds_path=" + outputPath;
  int ret = std::system(cmd.c_str());
}

void exportTopoMap(
    const std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells,
    const std::string&                                     path)
{
  YAML::Emitter out;

  out << YAML::BeginMap;
  out << YAML::Key << "circuit_levels";
  out << YAML::Value << YAML::BeginSeq; 

  for (const auto& cells : levelized_cells) {
    out << YAML::BeginSeq; 
    for (const auto& cell : cells) {
      out << cell->getName();
    }
    out << YAML::EndSeq;  
  }

  out << YAML::EndSeq;  
  out << YAML::EndMap;  

  std::ofstream fout(path);
  fout << out.c_str();
  fout.close();

  std::cout << "YAML file saved to " << path << std::endl;
}

void legalizeYAxis(
    std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells,
    double                                           spacing = 20)
{
  if (spacing < 0) {
    return;
  }
  for (const auto& column : levelized_cells) {
    if (column.size() < 2)
      continue;

    std::vector<std::shared_ptr<Cell>> sorted_column = column;

    std::sort(
        sorted_column.begin(),
        sorted_column.end(),
        [](const std::shared_ptr<Cell>& a, const std::shared_ptr<Cell>& b) {
          return a->getPosition().y() < b->getPosition().y();
        });

    for (size_t i = 0; i < sorted_column.size() - 1; ++i) {
      auto& cell_bottom = sorted_column[i];
      auto& cell_top    = sorted_column[i + 1];

      double y_bot = cell_bottom->getPosition().y();
      double h_bot = cell_bottom->getBoundingBox().second;
      double y_top = cell_top->getPosition().y();
      double h_top = cell_top->getBoundingBox().second;

      double min_dist    = (h_bot + h_top) / 2.0 + spacing;
      double actual_dist = y_top - y_bot;

      if (actual_dist < min_dist) {
        double new_y_top = y_bot + min_dist;

        cell_top->setPosition(
            Point(cell_top->getPosition().x(), std::round(new_y_top)));
      }
    }
  }
}
int main(int argc, char* argv[])
{
  // 1. Initialization
  OPDB::initLogger();
  IOManager manager;
  manager.setFlowControl(std::stoi(argv[1]))
      .setLefInPath(argv[2])
      .setDefInPath(argv[3])
      .setDefOutPath(argv[4])
      .setPlaceConfigPath(argv[5])
      .setRouteConfigPath(argv[6])
      .setEnableIncremental(std::stoi(argv[7]));
  std::shared_ptr<Database> db          = manager.getDatabase();
  int                       flowControl = manager.getFlowControl();
  std::vector<std::vector<std::shared_ptr<Cell>>> levelized_cells;
  std::unordered_map<std::shared_ptr<Cell>, std::pair<double, double>>
       calculated_coords;
  auto dp = buildGraphAndFindPaths(
      db->getDesign(), levelized_cells, calculated_coords);

  int level_count = 0;
  for (const auto& cells : levelized_cells) {
    std::cout << "Level: " << level_count++ << std::endl;
    for (const auto& cell : cells) {
      std::cout << cell->getName() << " ";
    }
    std::cout << std::endl;
  }
  exportTopoMap(levelized_cells, manager.getDefOutPath() + "_topology.yml");

  // db->setDesignName("init_placed");
  db->fromDesignToGdsfactoryYml(manager.getDefOutPath() + "_init.yml");
  OPDB::visualize(manager.getLefInPath(),
                  manager.getDefOutPath() + "_init.yml",
                  manager.getDefOutPath() + "_init",
                  true);

  // 2. Placement
  if (flowControl == IOManager::FlowControl::PLACEMENT
      || flowControl == IOManager::FlowControl::PR) {
    BookshelfConfig bookshelfConfig;
    loadBookshelfConfig(manager.getPlaceConfigPath() + ".yml", bookshelfConfig);
    checkBookshelfConfig(db->getDesign(), bookshelfConfig);
    db->fromDesignToBookshelf(bookshelfConfig.rowNum,
                              bookshelfConfig.siteNum,
                              bookshelfConfig.coordinateOrigin,
                              bookshelfConfig.SubrowOrigin,
                              bookshelfConfig.siteHeight,
                              bookshelfConfig.siteWidth,
                              bookshelfConfig.siteSpacing,
                              bookshelfConfig.halo,
                              bookshelfConfig.halo_x,
                              bookshelfConfig.halo_y,
                              db->getDesign()->getName());
    callDREAMPlace(manager.getPlaceConfigPath() + ".json");
    db->fromBookshelfPlaceToDesign("./results/" + db->getDesign()->getName()
                                   + "/" + db->getDesign()->getName()
                                   + ".gp.pl");
    db->setDesignName("placed");

    // 3. Legalization
    std::cout << "Legalize Y-axis" << std::endl;
    legalizeYAxis(levelized_cells, bookshelfConfig.legalizeSpacing);
    db->fromDesignToGdsfactoryYml(manager.getDefOutPath() + "_placed.yml");
    OPDB::visualize(manager.getLefInPath(),
                    manager.getDefOutPath() + "_placed.yml",
                    manager.getDefOutPath() + "_placed",
                    true);
  }

  int                     safeDistance = 100;
  std::shared_ptr<Design> design       = db->getDesign();
  auto rect = std::dynamic_pointer_cast<Rect>(design->getShape());
  auto lx   = rect->getShape().min_corner().x();
  auto ly   = rect->getShape().min_corner().y();
  auto ux   = rect->getShape().max_corner().x();
  auto uy   = rect->getShape().max_corner().y();
  design->setBoundingBox(
      {Point(0, 0),
       Point(ux - lx + 2 * safeDistance, uy - ly + 2 * safeDistance)});
  for (auto& cellPair : design->getCells()) {
    auto& cell = cellPair.second;
    cell->setPosition(
        Point(std::round(cell->getPosition().x() - lx + safeDistance),
              std::round(cell->getPosition().y() - ly + safeDistance)));
  }
  if (manager.getEnableIncremental() != 0) {
    RouteConfig rc = RouteConfig::loadFromYAML(manager.getRouteConfigPath());

    auto global_router = std::make_shared<GlobalRouter>(design);
    auto ilg           = std::make_shared<InsertionLossGraph>(rc);
    ilg->setDatapaths(dp);
    global_router->setInsertionLossGraph(ilg);
    global_router->initialRoute();
    ilg->buildInitialGraphLosses(global_router->getAllPaths(),
                                 global_router->getRouterGrid());

    global_router->exportToSVG("initial_route.svg");
    global_router->analyzeDatapathLosses(5);
    if (manager.getEnableIncremental() == 1) {
      auto start_time = std::chrono::high_resolution_clock::now();
      global_router->incrementalPlace(
          rc.incre_margin, rc.incre_radius, rc.incre_K);
      auto end_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
          end_time - start_time);
      std::cout << "Placement time: " << duration.count() << " ms" << std::endl;
    }
    global_router->exportToSVG("final_parallel.svg");
    global_router->analyzeDatapathLosses(5);
  }
  if (flowControl == IOManager::FlowControl::ROUTING
      || flowControl == IOManager::FlowControl::PR) {
    db->fromDesignToGdsfactoryYml(manager.getDefOutPath() + "_new.yml");
    OPDB::visualize(manager.getLefInPath(),
                    manager.getDefOutPath() + "_new.yml",
                    manager.getDefOutPath() + "_new",
                    true);
    db->fromDesignToLiDARYml(manager.getDefOutPath() + "_final.yml");

    callLiDAR(manager.getDefOutPath() + "_final.yml",
              manager.getRouteConfigPath(),
              manager.getDefOutPath() + "_final.gds");
  }
  return 0;
}
