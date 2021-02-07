#include "settings.h"
#include <QDebug>

bool TextGroup::enabled() const {
  return Conf::MarinerParams::textGrouping().contains(m_group);
}

void TextGroup::setEnabled(bool v) {
  auto values = Conf::MarinerParams::textGrouping();
  if (values.contains(m_group) && v) return;
  if (!values.contains(m_group) && !v) return;

  if (values.contains(m_group)) {
    values.removeOne(m_group);
  } else {
    values.append(m_group);
  }
  Conf::MarinerParams::setTextGrouping(values);
  emit enabledChanged();
}


Settings* Settings::instance() {
  static Settings* s = new Settings();
  return s;
}

Settings::Settings(QObject *parent)
  : QObject(parent)
  , m_categories({"Base", "Standard", "Other", "Mariners"})
  , m_colorTables({"Day Bright", "Day Black/Bg", "Day White/Bg", "Dusk", "Night"})
{
  m_textGroups << new TextGroup(10, "Important text");
  m_textGroups << new TextGroup(11, "Vertical clearances etc.",
                                "Vertical clearance of bridges, overhead cable, pipe or conveyor, "
                                "bearing of navline, recommended route, deep water route centreline, "
                                "name and communications channel of radio calling-in point.");
  m_textGroups << new TextGroup(20, "Other text");
  m_textGroups << new TextGroup(21, "Position names",
                                "Names for position reporting: "
                                "name or number of buoys, beacons, daymarks, "
                                "light vessel, light float, offshore platform.");
  m_textGroups << new TextGroup(23, "Light descriptions");
  m_textGroups << new TextGroup(24, "Notes", "Note on chart data or nautical publication.");
  m_textGroups << new TextGroup(25, "Nature of seabed");
  m_textGroups << new TextGroup(26, "Geographic names");
  m_textGroups << new TextGroup(27, "Other values", "Value of magnetic variation or swept depth.");
  m_textGroups << new TextGroup(28, "Other heights", "Height of islet or land feature");
  m_textGroups << new TextGroup(29, "Berth numbers");
  m_textGroups << new TextGroup(31, "National language text");

  for (auto obj: m_textGroups) {
    auto group = qobject_cast<TextGroup*>(obj);
    connect(group, &TextGroup::enabledChanged, this, &Settings::settingsChanged);
  }
}


Settings::~Settings() {
  qDeleteAll(m_textGroups);
}
