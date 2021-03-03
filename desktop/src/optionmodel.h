#pragma once

#include <KPageModel>
#include <QSortFilterProxyModel>
#include <QWidget>

class QHBoxLayout;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QScrollArea;

namespace KV {

class OptionWidget;

class OptionModel: public KPageModel
{
  Q_OBJECT

public:

  OptionModel(QObject *parent = nullptr);

  QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &index = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void reset();

public slots:

  void checkOptions(const QModelIndex& pageIndex);

signals:

  void edited(OptionWidget* opt, bool yes);
  void defaulted(OptionWidget* opt, bool yes);

protected slots:

  void addOption(const QString& category,
                 const QString& name,
                 const QString& displayName,
                 const QString& description,
                 int index = -1);
  void addCategory(const QString& name);

protected:

  static OptionWidget* createOptionWidget(const QString& property,
                                          const QString& displayName,
                                          const QString& description,
                                          const int index);

  using PageMap = QMap<QString, QScrollArea*>;
  using PageIterator = QMapIterator<QString, QScrollArea*>;
  using IconNameMap = QMap<QString, QString>;

  PageMap m_categories;
  QStringList m_names;
  IconNameMap m_icons;
};


// CategoriesFilter

class CategoriesFilter: public QSortFilterProxyModel {

  Q_OBJECT

public:

  CategoriesFilter(QObject* parent = nullptr);
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
};


// OptionWidget interface

class OptionWidget: public QWidget {

  Q_OBJECT

public:

  OptionWidget(const QString& prop,
               const QString& descr,
               const QString& helpText,
               QObject* target,
               QWidget* parent = nullptr);

  const QString& description() const;
  bool highlight(const QRegularExpression& re);


  void reset(); // proxy to editor
  void apply(); // editor to proxy & options
  void updateIt(); // options to proxy & editor
  void defaultIt(); // defaults to proxy, editor & options
  bool defaultable() const;

protected:

  virtual void doReset() = 0;
  virtual void doApply() = 0;
  virtual void doDefault() = 0;
  virtual void doUpdate() = 0;
  virtual bool canDefault() const = 0;
  virtual bool canApply() const = 0;

signals:

  void edited(bool yes);
  void defaulted(bool yes);

protected:

  QHBoxLayout* m_lay;
  QLabel* m_label;
  QString m_description;
  QString m_property;
  QObject* m_target;

};

class BooleanWidget: public OptionWidget {

  Q_OBJECT

public:

  BooleanWidget(const QString& prop,
                const QString& descr,
                const QString& helpText,
                QWidget* parent = nullptr);

  BooleanWidget(int index,
                const QString& descr,
                const QString& helpText,
                QWidget* parent = nullptr);

protected:

  void doReset() override;
  void doApply() override;
  void doDefault() override;
  void doUpdate() override;
  bool canDefault() const override;
  bool canApply() const override;

private:

  QCheckBox* m_box;
  bool m_proxy;
};


class RealValueWidget: public OptionWidget {

  Q_OBJECT

public:

  RealValueWidget(const QString& prop,
                  const QString& descr,
                  const QString& helpText,
                  QWidget* parent = nullptr);

protected:

  void doReset() override;
  void doApply() override;
  void doDefault() override;
  void doUpdate() override;
  bool canDefault() const override;
  bool canApply() const override;

private:

  QLineEdit* m_line;
  double m_proxy;
};

class StringComboWidget: public OptionWidget {

  Q_OBJECT

public:

  StringComboWidget(const QString& prop,
                    const QString& descr,
                    const QString& helpText,
                    QWidget* parent = nullptr);

protected:

  void doReset() override;
  void doApply() override;
  void doDefault() override;
  void doUpdate() override;
  bool canDefault() const override;
  bool canApply() const override;

private:

  QComboBox* m_box;
  int m_proxy;
  QString m_indexProperty;

};

}
