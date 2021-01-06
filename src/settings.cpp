#include "settings.h"
#include "conf_marinerparams.h"

Settings* Settings::instance() {
  static Settings* s = new Settings();
  return s;
}

Settings::Settings(QObject *parent)
  : QObject(parent)
  , m_twoShades(Conf::MarinerParams::twoShades())
  , m_safetyContour(Conf::MarinerParams::safetyContour())
  , m_deepContour(Conf::MarinerParams::deepContour())
  , m_shallowContour(Conf::MarinerParams::shallowContour())
  , m_safetyDepth(Conf::MarinerParams::safetyDepth())
  , m_simplifiedSymbols(Conf::MarinerParams::simplifiedSymbols())
  , m_plainBoundaries(Conf::MarinerParams::plainBoundaries())
  , m_showMeta(Conf::MarinerParams::showMeta())
  , m_fullSectors(Conf::MarinerParams::fullLengthSectors())
  , m_colorTable(Conf::MarinerParams::colorTable())
{
  const QMap<Conf::MarinerParams::EnumMaxCategory::type, S52::Lookup::Category> cats{
    {Conf::MarinerParams::EnumMaxCategory::Base, S52::Lookup::Category::Base},
    {Conf::MarinerParams::EnumMaxCategory::Standard, S52::Lookup::Category::Standard},
    {Conf::MarinerParams::EnumMaxCategory::Other, S52::Lookup::Category::Other},
    {Conf::MarinerParams::EnumMaxCategory::Mariners, S52::Lookup::Category::Mariners},
  };

  m_maxCat = cats[Conf::MarinerParams::maxCategory()];

  for (auto i: Conf::MarinerParams::textGrouping()) m_textGrouping.insert(i);
}


Settings::~Settings() {

  Conf::MarinerParams::setTwoShades(m_twoShades);
  Conf::MarinerParams::setSafetyContour(m_safetyContour);
  Conf::MarinerParams::setDeepContour(m_deepContour);
  Conf::MarinerParams::setShallowContour(m_shallowContour);
  Conf::MarinerParams::setSafetyDepth(m_safetyDepth);
  Conf::MarinerParams::setSimplifiedSymbols(m_simplifiedSymbols);
  Conf::MarinerParams::setPlainBoundaries(m_plainBoundaries);
  Conf::MarinerParams::setColorTable(m_colorTable);
  Conf::MarinerParams::setShowMeta(m_showMeta);

  const QMap<S52::Lookup::Category, Conf::MarinerParams::EnumMaxCategory::type> cats{
    {S52::Lookup::Category::Base, Conf::MarinerParams::EnumMaxCategory::Base},
    {S52::Lookup::Category::Standard, Conf::MarinerParams::EnumMaxCategory::Standard},
    {S52::Lookup::Category::Other, Conf::MarinerParams::EnumMaxCategory::Other},
    {S52::Lookup::Category::Mariners, Conf::MarinerParams::EnumMaxCategory::Mariners},
  };

  Conf::MarinerParams::setMaxCategory(cats[m_maxCat]);

  Conf::MarinerParams::self()->save();
}
