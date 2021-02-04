#pragma once

#include <QtQuick/QQuickFramebufferObject>
#include <QOpenGLVertexArrayObject>

class DetailMode;
class Camera;
class QOpenGLContext;
class QOffscreenSurface;


class ChartDisplay: public QQuickFramebufferObject {

  Q_OBJECT

public:

  ChartDisplay();
  ~ChartDisplay();

  Q_INVOKABLE void zoomIn();
  Q_INVOKABLE void zoomOut();
  Q_INVOKABLE void panStart(qreal x, qreal y);
  Q_INVOKABLE void pan(qreal x, qreal y);
  Q_INVOKABLE void northUp();
  Q_INVOKABLE void rotate(qreal degrees);

  Q_PROPERTY(QStringList chartSets READ chartSets NOTIFY chartSetsChanged)
  Q_PROPERTY(QString chartSet READ chartSet WRITE setChartSet NOTIFY chartSetChanged)

  QStringList chartSets() const;
  QString chartSet() const;
  void setChartSet(const QString& s);

  Renderer* createRenderer() const override;

  void setCamera(Camera* cam);
  void checkChartSet() const;
  const Camera* camera() const {return m_camera;}
  bool consume(quint32 flag);
  const QRectF& viewArea() const {return m_viewArea;}

  static const quint32 ChartsUpdated = 1;
  static const quint32 EnteringChartMode = 2;
  static const quint32 LeavingChartMode = 4;
  static const quint32 ChartSetChanged = 8;

private slots:

  void handleWindowChanged(QQuickWindow *win);
  void initializeGL(QOpenGLContext* ctx);
  void initializeSG();
  void finalizeSG();
  void resize(int);
  void orient(Qt::ScreenOrientation orientation);

signals:

  void updateViewport(const Camera* cam, bool force = false);
  void chartSetsChanged(const QStringList& chartSets);
  void chartSetChanged(const QString& chartSet);

private:

  QString defaultChartSet() const;

  Camera* m_camera;
  bool m_initialized;
  QRectF m_viewArea;
  quint32 m_flags;

  QSize m_size;
  QSize m_orientedSize;
  Qt::ScreenOrientation m_orientation;
  QPointF m_lastPos;

  QOpenGLContext* m_context;
  QOffscreenSurface* m_surface;
  QOpenGLVertexArrayObject m_vao;

};
