#pragma once
#include "bookshelf.h"
#include "defyml.h"
#include "design.h"
#include "lefyml.h"
#include "library.h"
#include "lidaryml.h"
#include "place.h"
#include "place_config.h"
#include "route.h"
#include "route_config.h"

class Database : public std::enable_shared_from_this<Database>
{
 public:
  Database() = default;
  Database(std::shared_ptr<Design> design, std::shared_ptr<Library> library)
      : _design(design), _library(library)
  {
  }
  std::shared_ptr<Design> getDesign() const { return _design; }
  void setDesign(std::shared_ptr<Design> design) { _design = design; }
  void setDesignName(const std::string& name) { _design->setName(name); }

  std::shared_ptr<Library> getLibrary() const { return _library; }
  void setLibrary(std::shared_ptr<Library> library) { _library = library; }
  void setLibraryName(const std::string& name) { _library->setName(name); }

  void placeAll(const PlaceConfig& config)
  {
    PlaceManager placer(shared_from_this(), config);
    placer.placeAll();
  }

  void routeAll(const RouteConfig& config)
  {
    RouteManager router(shared_from_this(), config);
    router.routeAll();
    router.writeBack();
  }

  void fromDesignToGdsfactoryYml(const std::string& path) const
  {
    DefYml::fromDesignToGdsfactoryYml(*_design, path);
  }

  void fromDesignToYml(const std::string& path) const
  {
    DefYml::fromDesignToYml(*_design, path);
  }

  void fromLibraryToYml(const std::string& path) const
  {
    LefYml::fromLibraryToYml(*_library, path);
  }

  void fromDesignToBookshelf(int                rowNum,
                             int                siteNum,
                             int                coordinateOrigin,
                             int                SubrowOrigin,
                             int                siteHeight,
                             int                siteWidth,
                             int                siteSpacing,
                             int                halo,
                             int                halo_x,
                             int                halo_y,
                             const std::string& path) const
  {
    Bookshelf::fromDesignToBookshelf(*_design,
                                     rowNum,
                                     siteNum,
                                     coordinateOrigin,
                                     SubrowOrigin,
                                     siteHeight,
                                     siteWidth,
                                     siteSpacing,
                                     halo,
                                     halo_x,
                                     halo_y,
                                     path);
  }

  void fromBookshelfPlaceToDesign(const std::string& path) const
  {
    Bookshelf::fromBookshelfPlaceToDesign(*_design, path);
  }

  void fromDesignToLiDARYml(const std::string& path) const
  {
    LiDARYml::fromDesignToLiDARYml(*_design, path);
  }

 private:
  std::shared_ptr<Design>  _design;
  std::shared_ptr<Library> _library;
};
