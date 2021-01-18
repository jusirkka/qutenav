#pragma once

#include <QtQuick/QQuickItem>

class DetailMode;
class Camera;
class ChartRenderer;

class ChartDisplay: public QQuickItem {

  Q_OBJECT

public:

  ChartDisplay();
  ~ChartDisplay();

  Q_INVOKABLE void zoomIn();
  Q_INVOKABLE void zoomOut();
  Q_INVOKABLE void panStart(qreal x, qreal y);
  Q_INVOKABLE void pan(qreal x, qreal y);
  Q_INVOKABLE void northUp();

  Q_PROPERTY(QStringList chartSets READ chartSets NOTIFY chartSetsChanged)
  Q_PROPERTY(QString chartSet READ chartSet WRITE setChartSet NOTIFY chartSetChanged)

  QStringList chartSets() const;
  QString chartSet() const;
  void setChartSet(const QString& s);

private slots:

  void handleWindowChanged(QQuickWindow *win);
  void initializeGL();
  void cleanup();
  void resize(int);

signals:

  void updateViewport(const Camera* cam, bool force = false);
  void chartSetsChanged(const QStringList& chartSets);
  void chartSetChanged(const QString& chartSet);

protected:

  QSGNode* updatePaintNode(QSGNode* prev, UpdatePaintNodeData*) override;

private:

  QString defaultChartSet() const;

  ChartRenderer *m_renderer;
  Camera* m_camera;
  bool m_initialized;

  bool m_leavingChartMode;
  bool m_enteringChartMode;
  bool m_chartsUpdated;
  QRectF m_viewArea;
  bool m_chartSetChanged;

  QSize m_size;
  QPointF m_lastPos;

};
