/* -*- coding: utf-8-unix -*-
 *
 * File: src/settings.h
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <QObject>
#include "conf_marinerparams.h"
#include "conf_mainwindow.h"
#include "conf_units.h"
#include "conf_quick.h"
#include "logging.h"
#include <QMap>


class Settings: public QObject {
  Q_OBJECT

public:

  ~Settings();

  static Settings* instance();

  Q_PROPERTY(bool plainBoundaries
             READ plainBoundaries
             WRITE setPlainBoundaries)

  bool plainBoundaries() const {
    return Conf::MarinerParams::PlainBoundaries();
  }

  void setPlainBoundaries(bool v) {
    if (v != plainBoundaries()) {
      Conf::MarinerParams::setPlainBoundaries(v);
      emit lookupUpdateNeeded();
    }
  }

  Q_PROPERTY(bool simplifiedSymbols
             READ simplifiedSymbols
             WRITE setSimplifiedSymbols)

  bool simplifiedSymbols() const {
    return Conf::MarinerParams::SimplifiedSymbols();
  }

  void setSimplifiedSymbols(bool v) {
    if (v != simplifiedSymbols()) {
      Conf::MarinerParams::setSimplifiedSymbols(v);
      emit lookupUpdateNeeded();
    }
  }

  Q_PROPERTY(bool fullLengthSectors
             READ fullLengthSectors
             WRITE setFullLengthSectors)

  bool fullLengthSectors() const {
    return Conf::MarinerParams::FullLengthSectors();
  }

  void setFullLengthSectors(bool v) {
    if (v != fullLengthSectors()) {
      Conf::MarinerParams::setFullLengthSectors(v);
      emit settingsChanged();
    }
  }

  Q_PROPERTY(bool twoShades
             READ twoShades
             WRITE setTwoShades)

  bool twoShades() const {
    return Conf::MarinerParams::TwoShades();
  }

  void setTwoShades(bool v) {
    if (v != twoShades()) {
      Conf::MarinerParams::setTwoShades(v);
      emit settingsChanged();
    }
  }

  Q_PROPERTY(QStringList maxCategoryNames
             READ maxCategoryNames
             NOTIFY maxCategoryNamesChanged)

  QStringList maxCategoryNames() const {
    return Conf::MarinerParams::EnumMaxCategory::names;
  }

  Q_PROPERTY(quint8 maxCategory
             READ maxCategory
             WRITE setMaxCategory)

  quint8 maxCategory() const {
    return static_cast<quint8>(Conf::MarinerParams::MaxCategory());
  }

  void setMaxCategory(quint8 v) {
    if (v != maxCategory()) {
      Conf::MarinerParams::setMaxCategory(static_cast<Conf::MarinerParams::EnumMaxCategory::type>(v));
      emit settingsChanged();
    }
  }

  Q_PROPERTY(QStringList colorTableNames
             READ colorTableNames
             NOTIFY colorTableNamesChanged)

  QStringList colorTableNames() const {
    return Conf::MarinerParams::EnumColorTable::names;
  }

  Q_PROPERTY(quint8 colorTable
             READ colorTable
             WRITE setColorTable)

  quint8 colorTable() const {
    return static_cast<quint8>(Conf::MarinerParams::ColorTable());
  }

  void setColorTable(quint8 v) {
    if (v != colorTable()) {
      Conf::MarinerParams::setColorTable(static_cast<Conf::MarinerParams::EnumColorTable::type>(v));
      emit colorTableChanged(v);
      emit settingsChanged();
    }
  }

  Q_PROPERTY(bool shallowPattern
             READ shallowPattern
             WRITE setShallowPattern)

  bool shallowPattern() const {
    return Conf::MarinerParams::ShallowPattern();
  }

  void setShallowPattern(bool v) {
    if (v != shallowPattern()) {
      Conf::MarinerParams::setShallowPattern(v);
      emit settingsChanged();
    }
  }

  Q_PROPERTY(qreal safetyDepth
             READ safetyDepth
             WRITE setSafetyDepth)

  qreal safetyDepth() const {
    return Conf::MarinerParams::SafetyDepth();
  }

  void setSafetyDepth(qreal v) {
    if (v != safetyDepth()) {
      Conf::MarinerParams::setSafetyDepth(v);
      emit settingsChanged();
    }
  }

  Q_PROPERTY(qreal safetyContour
             READ safetyContour
             WRITE setSafetyContour)

  qreal safetyContour() const {
    return Conf::MarinerParams::SafetyContour();
  }

  void setSafetyContour(qreal v) {
    if (v != safetyContour()) {
      Conf::MarinerParams::setSafetyContour(v);
      emit settingsChanged();
    }
  }

  Q_PROPERTY(qreal shallowContour
             READ shallowContour
             WRITE setShallowContour)

  qreal shallowContour() const {
    return Conf::MarinerParams::ShallowContour();
  }

  void setShallowContour(qreal v) {
    if (v != shallowContour()) {
      Conf::MarinerParams::setShallowContour(v);
      emit settingsChanged();
    }
  }

  Q_PROPERTY(qreal deepContour
             READ deepContour
             WRITE setDeepContour)

  qreal deepContour() const {
    return Conf::MarinerParams::DeepContour();
  }

  void setDeepContour(qreal v) {
    if (v != deepContour()) {
      Conf::MarinerParams::setDeepContour(v);
      emit settingsChanged();
    }
  }

  Q_PROPERTY(QList<QObject*> textGroups
             READ textGroups
             NOTIFY textGroupsChanged)

  QList<QObject*> textGroups() const {return m_textGroups;}

  Q_PROPERTY(QList<QObject*> disabledClasses
             READ disabledClasses
             NOTIFY disabledClassesChanged)

  QList<QObject*> disabledClasses() const {return m_disabledClasses;}

  Q_PROPERTY(QSizeF windowGeom
             READ windowGeom
             WRITE setWindowGeom)

  QSizeF windowGeom() const {
    return Conf::MainWindow::WindowGeom();
  }

  void setWindowGeom(const QSizeF& v) {
    Conf::MainWindow::setWindowGeom(v);
  }

  Q_PROPERTY(QSizeF lastGeom
             READ lastGeom
             WRITE setLastGeom)

  QSizeF lastGeom() const {
    return Conf::MainWindow::LastGeom();
  }

  void setLastGeom(const QSizeF& v) {
    Conf::MainWindow::setLastGeom(v);
  }

  Q_PROPERTY(bool fullScreen
             READ fullScreen
             WRITE setFullScreen)

  bool fullScreen() const {
    return Conf::MainWindow::FullScreen();
  }

  void setFullScreen(bool v) {
    Conf::MainWindow::setFullScreen(v);
  }

  Q_PROPERTY(QStringList chartFolders
             READ chartFolders
             WRITE setChartFolders)

  QStringList chartFolders() const {
    return Conf::MainWindow::ChartFolders();
  }

  void setChartFolders(const QStringList& v) {
    Conf::MainWindow::setChartFolders(v);
  }

  Q_PROPERTY(quint32 cacheSize
             READ cacheSize
             WRITE setCacheSize)

  quint32 cacheSize() const {
    return Conf::MainWindow::CacheSize();
  }

  void setCacheSize(quint32 v) {
    Conf::MainWindow::setCacheSize(v);
  }

  Q_PROPERTY(QStringList locationUnitNames
             READ locationUnitNames
             NOTIFY locationUnitNamesChanged)

  QStringList locationUnitNames() const {
    return Conf::Units::EnumLocation::names;
  }

  Q_PROPERTY(quint8 locationUnits
             READ locationUnits
             WRITE setLocationUnits)

  quint8 locationUnits() const {
    return static_cast<quint8>(Conf::Units::Location());
  }

  void setLocationUnits(quint8 v) {
    if (v != locationUnits()) {
      Conf::Units::setLocation(static_cast<Conf::Units::EnumLocation::type>(v));
      // Note: order is important
      emit unitsChanged();
      emit settingsChanged();
    }
  }

  Q_PROPERTY(QStringList depthUnitNames
             READ depthUnitNames
             NOTIFY depthUnitNamesChanged)

  QStringList depthUnitNames() const {
    return Conf::Units::EnumDepth::names;
  }

  Q_PROPERTY(quint8 depthUnits
             READ depthUnits
             WRITE setDepthUnits)

  quint8 depthUnits() const {
    return static_cast<quint8>(Conf::Units::Depth());
  }

  void setDepthUnits(quint8 v) {
    if (v != depthUnits()) {
      Conf::Units::setDepth(static_cast<Conf::Units::EnumDepth::type>(v));
      // Note: order is important
      emit unitsChanged();
      emit settingsChanged();
    }
  }

  Q_PROPERTY(QStringList distanceUnitNames
             READ distanceUnitNames
             NOTIFY distanceUnitNamesChanged)

  QStringList distanceUnitNames() const {
    return Conf::Units::EnumDistance::names;
  }

  Q_PROPERTY(quint8 distanceUnits
             READ distanceUnits
             WRITE setDistanceUnits)

  quint8 distanceUnits() const {
    return static_cast<quint8>(Conf::Units::Distance());
  }

  void setDistanceUnits(quint8 v) {
    if (v != distanceUnits()) {
      Conf::Units::setDistance(static_cast<Conf::Units::EnumDistance::type>(v));
      // Note: order is important
      emit unitsChanged();
      emit settingsChanged();
    }
  }

  Q_PROPERTY(QStringList shortDistanceUnitNames
             READ shortDistanceUnitNames
             NOTIFY shortDistanceUnitNamesChanged)

  QStringList shortDistanceUnitNames() const {
    return Conf::Units::EnumShortDistance::names;
  }

  Q_PROPERTY(quint8 shortDistanceUnits
             READ shortDistanceUnits
             WRITE setShortDistanceUnits)

  quint8 shortDistanceUnits() const {
    return static_cast<quint8>(Conf::Units::ShortDistance());
  }

  void setShortDistanceUnits(quint8 v) {
    if (v != shortDistanceUnits()) {
      Conf::Units::setShortDistance(static_cast<Conf::Units::EnumShortDistance::type>(v));
      // Note: order is important
      emit unitsChanged();
      emit settingsChanged();
    }
  }

  Q_PROPERTY(QStringList boatSpeedUnitNames
             READ boatSpeedUnitNames
             NOTIFY boatSpeedUnitNamesChanged)

  QStringList boatSpeedUnitNames() const {
    return Conf::Units::EnumBoatSpeed::names;
  }

  Q_PROPERTY(quint8 boatSpeedUnits
             READ boatSpeedUnits
             WRITE setBoatSpeedUnits)

  quint8 boatSpeedUnits() const {
    return static_cast<quint8>(Conf::Units::BoatSpeed());
  }

  void setBoatSpeedUnits(quint8 v) {
    if (v != boatSpeedUnits()) {
      Conf::Units::setBoatSpeed(static_cast<Conf::Units::EnumBoatSpeed::type>(v));
      // Note: order is important
      emit unitsChanged();
      emit settingsChanged();
    }
  }

  Q_PROPERTY(bool indicateScales
             READ indicateScales
             WRITE setIndicateScales)

  bool indicateScales() const {
    return Conf::Quick::IndicateScales();
  }

  void setIndicateScales(bool v) {
    if (v != indicateScales()) {
      Conf::Quick::setIndicateScales(v);
      emit settingsChanged();
    }
  }

signals:

  void colorTableChanged(quint8 t);
  void settingsChanged();
  void lookupUpdateNeeded();
  void unitsChanged();

  // dummies to keep qtquick from moaning about missing signals
  void maxCategoryNamesChanged(const QStringList&);
  void colorTableNamesChanged(const QStringList&);
  void textGroupsChanged(const QObjectList&);
  void disabledClassesChanged(const QObjectList&);
  void locationUnitNamesChanged(const QStringList&);
  void depthUnitNamesChanged(const QStringList&);
  void distanceUnitNamesChanged(const QStringList&);
  void shortDistanceUnitNamesChanged(const QStringList&);
  void boatSpeedUnitNamesChanged(const QStringList&);

private:

  Settings(QObject* parent = nullptr);

  QObjectList m_textGroups;
  QObjectList m_disabledClasses;


  static inline const QMap<int, const char*> texts = {
    //% "Important text"
    {10, QT_TRID_NOOP("qtnav-txt-10")},
    //% "Vertical clearances etc."
    {11, QT_TRID_NOOP("qtnav-txt-11")},
    //% "Soundings"
    {12, QT_TRID_NOOP("qtnav-txt-12")},
    //% "Other Text"
    {20, QT_TRID_NOOP("qtnav-txt-20")},
    //% "Position names"
    {21, QT_TRID_NOOP("qtnav-txt-21")},
    //% "Light descriptions"
    {23, QT_TRID_NOOP("qtnav-txt-23")},
    //% "Notes"
    {24, QT_TRID_NOOP("qtnav-txt-24")},
    //% "Nature of seabed"
    {25, QT_TRID_NOOP("qtnav-txt-25")},
    //% "Geographic names"
    {26, QT_TRID_NOOP("qtnav-txt-26")},
    //% "Other values"
    {27, QT_TRID_NOOP("qtnav-txt-27")},
    //% "Other heights"
    {28, QT_TRID_NOOP("qtnav-txt-28")},
    //% "Berth numbers"
    {29, QT_TRID_NOOP("qtnav-txt-29")},
    //% "National language text"
    {29, QT_TRID_NOOP("qtnav-txt-31")},
  };

  static inline const QMap<int, const char*> descriptions = {
    //% "Vertical clearance of bridges, overhead cable, pipe or conveyor, bearing of navline, recommended route, deep water route centreline, name and communications channel of radio calling-in point."
    {11, QT_TRID_NOOP("qtnav-txt-11-desc")},
    //% "Names for position reporting: name or number of buoys, beacons, daymarks, light vessel, light float, offshore platform."
    {21, QT_TRID_NOOP("qtnav-txt-21-desc")},
    //% "Note on chart data or nautical publication."
    {24, QT_TRID_NOOP("qtnav-txt-24-desc")},
    //% "Value of magnetic variation or swept depth."
    {27, QT_TRID_NOOP("qtnav-txt-27-desc")},
    //% "Height of islet or land feature"
    {28, QT_TRID_NOOP("qtnav-txt-28-desc")},
  };
};


class SettingsGroup: public QObject {
  Q_OBJECT
public:
  SettingsGroup(int group, const QString& text, const QString& desc)
    : m_group(group)
    , m_text(text)
    , m_description(desc)
  {}

  virtual ~SettingsGroup() = default;

  Q_PROPERTY(bool enabled
             READ enabled
             WRITE setEnabled)

  Q_PROPERTY(QString text
             READ text)

  Q_PROPERTY(QString description
             READ description)

  virtual QList<int> getter() const = 0;
  virtual void setter(const QList<int>& values) const = 0;

  bool enabled() const {
    return getter().contains(m_group);
  }

  void setEnabled(bool v) {
    auto values = getter();
    if (values.contains(m_group) && v) return;
    if (!values.contains(m_group) && !v) return;

    if (values.contains(m_group)) {
      values.removeOne(m_group);
    } else {
      values.append(m_group);
    }
    setter(values);
    emit enabledChanged();
  }

  const QString& text() const {return m_text;}
  const QString& description() const {return m_description;}

signals:

  void enabledChanged();

private:

  int m_group;
  QString m_text;
  QString m_description;
};

class TextGroup: public SettingsGroup {
  Q_OBJECT
public:
  TextGroup(int group, const QString& text, const QString& desc = QString())
    : SettingsGroup(group, text, desc)
  {}

  QList<int> getter() const override {return Conf::MarinerParams::TextGrouping();}
  void setter(const QList<int>& values) const override {Conf::MarinerParams::setTextGrouping(values);}
};

class DisabledClass: public SettingsGroup {
  Q_OBJECT
public:
  DisabledClass(int code, const QString& text, const QString& desc = QString())
    : SettingsGroup(code, text, desc)
  {}

  QList<int> getter() const override {return Conf::MarinerParams::DisabledClasses();}
  void setter(const QList<int>& values) const override {Conf::MarinerParams::setDisabledClasses(values);}
};


