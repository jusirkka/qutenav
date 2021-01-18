#pragma once

#include "configgroup.h"

namespace Conf {

class MainWindow : public ConfigGroup {
public:

  static MainWindow *self();
  ~MainWindow();

  CONF_DECL(chartset, Chartset, chartset, QString, toString)

private:

  MainWindow();

};

}


