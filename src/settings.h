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
#include "logging.h"

class TextGroup: public QObject {
  Q_OBJECT
public:
  TextGroup(int group, bool defval, const QString& text, const QString& desc = QString())
    : m_group(group)
    , m_text(text)
    , m_description(desc)
    , m_default(defval)
  {}

  Q_PROPERTY(bool enabled
             READ enabled
             WRITE setEnabled)

  bool enabled() const;
  void setEnabled(bool v);

  Q_PROPERTY(QString text
             READ text)

  const QString& text() const {return m_text;}

  Q_PROPERTY(QString description
             READ description)

  const QString& description() const {return m_description;}

signals:

  void enabledChanged();

private:

  int m_group;
  QString m_text;
  QString m_description;
  bool m_default;
};

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

  Q_PROPERTY(bool showMeta
             READ showMeta
             WRITE setShowMeta)

  bool showMeta() const {
    return Conf::MarinerParams::ShowMeta();
  }

  void setShowMeta(bool v) {
    if (v != showMeta()) {
      Conf::MarinerParams::setShowMeta(v);
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


  float displayLengthScaling() const;
  float displayTextSizeScaling() const;
  float displayLineWidthScaling() const;
  float displayRasterSymbolScaling() const;

signals:

  void colorTableChanged(quint8 t);
  void settingsChanged();
  void lookupUpdateNeeded();
  void unitsChanged();

  // dummies to keep qtquick from moaning about missing signals
  void maxCategoryNamesChanged(const QStringList&);
  void colorTableNamesChanged(const QStringList&);
  void textGroupsChanged(const QObjectList&);
  void locationUnitNamesChanged(const QStringList&);
  void depthUnitNamesChanged(const QStringList&);
  void distanceUnitNamesChanged(const QStringList&);
  void shortDistanceUnitNamesChanged(const QStringList&);
  void boatSpeedUnitNamesChanged(const QStringList&);

private:

  Settings(QObject* parent = nullptr);

  QObjectList m_textGroups;

  static inline const float dpmm0 = 6.2;
  static inline const float delta_dpmm1 = 9.7;

};

