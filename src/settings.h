#pragma once

#include <QObject>
#include "conf_marinerparams.h"

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

  void setDefault();
  bool getDefault() const;

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
    return Conf::MarinerParams::plainBoundaries();
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
    return Conf::MarinerParams::simplifiedSymbols();
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
    return Conf::MarinerParams::fullLengthSectors();
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
    return Conf::MarinerParams::showMeta();
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
    return Conf::MarinerParams::twoShades();
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
    return static_cast<quint8>(Conf::MarinerParams::maxCategory());
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
    return static_cast<quint8>(Conf::MarinerParams::colorTable());
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
    return Conf::MarinerParams::safetyDepth();
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
    return Conf::MarinerParams::safetyContour();
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
    return Conf::MarinerParams::shallowContour();
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
    return Conf::MarinerParams::deepContour();
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

  void setDefault(const QString& prop);
  QVariant getDefault(const QString& prop) const;

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

};

