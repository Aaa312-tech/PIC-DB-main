#pragma once
#include "bookshelf.h"
#include "database.h"
#include "defyml.h"
#include "lefyml.h"
#include "log.h"

class IOManager
{
 public:
  IOManager() = default;

  std::shared_ptr<Database> getDatabase()
  {
    std::shared_ptr<Library> library = _lefyml.toLibrary();
    std::shared_ptr<Design>  design  = _defyml.toDesign(library);
    return std::make_shared<Database>(design, library);
  }

  LefYml& getLefYml() { return _lefyml; }
  DefYml& getDefYml() { return _defyml; }

  enum FlowControl
  {
    PR,
    PLACEMENT,
    ROUTING,
  };

  const int getFlowControl() const { return _flowControl; }

  IOManager& setFlowControl(int fc)
  {
    _flowControl = fc;
    return *this;
  }

  const std::string getDefInPath() const { return _defInPath; }

  IOManager& setDefInPath(const std::string& path)
  {
    _defyml.readDefYml(path);
    _defInPath = path;
    return *this;
  }

  const std::string getDefOutPath() const { return _defOutPath; }

  IOManager& setDefOutPath(const std::string& path)
  {
    _defOutPath = path;
    return *this;
  }

  const std::string getLefInPath() const { return _lefInPath; }

  IOManager& setLefInPath(const std::string& path)
  {
    _lefyml.readLefYml(path);
    _lefInPath = path;
    return *this;
  }

  const std::string getLefOutPath() const { return _lefOutPath; }

  IOManager& setLefOutPath(const std::string& path)
  {
    _lefOutPath = path;
    return *this;
  }

  const std::string getPlaceConfigPath() const { return _placeConfigPath; }

  IOManager& setPlaceConfigPath(const std::string& path)
  {
    _placeConfigPath = path;
    return *this;
  }

  const std::string getRouteConfigPath() const { return _routeConfigPath; }

  IOManager& setRouteConfigPath(const std::string& path)
  {
    _routeConfigPath = path;
    return *this;
  }

  const std::string getBookshelfOutPath() const { return _bookshelfOutPath; }

  IOManager& setBookshelfOutPath(const std::string& path)
  {
    _bookshelfOutPath = path;
    return *this;
  }

  const std::string getBookshelfPlaceInPath() const
  {
    return _bookshelfPlaceInPath;
  }

  IOManager& setBookshelfPlaceInPath(const std::string& path)
  {
    _bookshelfPlaceInPath = path;
    return *this;
  }

  const int getEnableIncremental() const { return _enableIncremental; }

  IOManager& setEnableIncremental(int enable)
  {
    _enableIncremental = enable;
    return *this;
  }

 private:
  LefYml _lefyml;
  DefYml _defyml;

  int         _flowControl;
  std::string _defInPath;
  std::string _defOutPath;
  std::string _lefInPath;
  std::string _lefOutPath;
  std::string _placeConfigPath;
  std::string _routeConfigPath;
  std::string _bookshelfOutPath;
  std::string _bookshelfPlaceInPath;
  int         _enableIncremental;
};