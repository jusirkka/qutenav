#pragma once

#include <QObject>
#include "s52presentation.h"


class Settings: public QObject {
  Q_OBJECT

public:

  static Settings* instance();

  S52::Lookup::Category maxCategory() const {return m_maxCat;}
  bool twoShades() const {return m_twoShades;}
  double safetyContour() const {return m_safetyContour;}
  double deepContour() const {return m_deepContour;}
  double shallowContour() const {return m_shallowContour;}

  enum ColorTable {
    DayBright,
    DayBlackBack,
    DayWhiteBack,
    Dusk,
    Night
  };

  ~Settings();

signals:

  void simplifiedSymbols(bool);
  void plainBoundaries(bool);
  void colorTable(ColorTable t);

private:

  Settings(QObject *parent = nullptr);

  bool m_twoShades;
  double m_safetyContour;
  double m_deepContour;
  double m_shallowContour;
  S52::Lookup::Category m_maxCat;
  bool m_simplifiedSymbols;
  bool m_plainBoundaries;
  ColorTable m_colorTable;

};

