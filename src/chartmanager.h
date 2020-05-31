#pragma once

#include <QObject>

class Camera;
class S57Chart;
class S57ChartOutline;

class ChartManager: public QObject {

  Q_OBJECT

public:

  using ChartVector = QVector<S57Chart*>;
  using ChartOutlineVector = QVector<S57ChartOutline*>;

  static ChartManager* instance();


  bool isValidScale(quint32 scale) const;


  const ChartVector& charts() const;
  const ChartOutlineVector& outlines() const;

signals:

  void idle();
  void active();
  void chartsUpdated();
  void objectsUpdated();

public slots:

  void updateCharts(const Camera* cam);

private:

  ChartManager(QObject *parent = nullptr);

  ChartVector m_charts;
};

