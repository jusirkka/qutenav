#pragma once

#include <QObject>
#include "s52presentation.h"
#include <QSet>


class Settings: public QObject {
  Q_OBJECT

public:

  using TextGrouping = QSet<quint8>;

  static Settings* instance();

  S52::Lookup::Category maxCategory() const {return m_maxCat;}
  bool twoShades() const {return m_twoShades;}
  double safetyContour() const {return m_safetyContour;}
  double safetyDepth() const {return m_safetyDepth;}
  double deepContour() const {return m_deepContour;}
  double shallowContour() const {return m_shallowContour;}
  const TextGrouping& textGrouping() const {return m_textGrouping;}
  bool showMetaObjects() const {return m_showMeta;}
  bool fullLengthLightSectors() const {return m_fullSectors;}
  bool plainBoundaries() const {return m_plainBoundaries;}

  enum ColorTable {
    DayBright,
    DayBlackBack,
    DayWhiteBack,
    Dusk,
    Night
  };

  ~Settings();

signals:

  void simplifiedSymbolsChaged(bool);
  void plainBoundariesChanged(bool);
  void colorTableChanged(ColorTable t);

private:

  Settings(QObject *parent = nullptr);

  bool m_twoShades;
  double m_safetyContour;
  double m_deepContour;
  double m_shallowContour;
  double m_safetyDepth;
  S52::Lookup::Category m_maxCat;
  bool m_simplifiedSymbols;
  bool m_plainBoundaries;
  bool m_showMeta;
  bool m_fullSectors;
  ColorTable m_colorTable;
  TextGrouping m_textGrouping;

};

