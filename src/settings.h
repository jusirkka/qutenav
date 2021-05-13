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

  QStringList maxCategoryNames() const {return m_categories;}

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

  QStringList colorTableNames() const {return m_colorTables;}

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

  Q_PROPERTY(int mainWindowWidth
             READ mainWindowWidth
             WRITE setMainWindowWidth)

  int mainWindowWidth() const {
    return Conf::MainWindow::Width();
  }

  void setMainWindowWidth(int v) {
    Conf::MainWindow::setWidth(v);
  }

  Q_PROPERTY(int mainWindowHeight
             READ mainWindowHeight
             WRITE setMainWindowHeight)

  int mainWindowHeight() const {
    return Conf::MainWindow::Height();
  }

  void setMainWindowHeight(int v) {
    Conf::MainWindow::setHeight(v);
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

  float displayLengthScaling() const;
  float displayTextSizeScaling() const;
  float displayLineWidthScaling() const;
  float displayRasterSymbolScaling() const;

signals:

  void colorTableChanged(quint8 t);
  void settingsChanged();
  void lookupUpdateNeeded();

  // dummies to keep qtquick from moaning about missing signals
  void maxCategoryNamesChanged(const QStringList&);
  void colorTableNamesChanged(const QStringList&);
  void textGroupsChanged(const QObjectList&);

private:

  Settings(QObject* parent = nullptr);

  QObjectList m_textGroups;
  QStringList m_categories;
  QStringList m_colorTables;

  static inline const float dpmm0 = 6.2;
  static inline const float delta_dpmm1 = 9.7;

};

