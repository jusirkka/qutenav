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
  Q_PROPERTY(qreal scaleBarLength READ scaleBarLength NOTIFY scaleBarLengthChanged)
  Q_PROPERTY(QString scaleBarText READ scaleBarText)

  QStringList chartSets() const;
  QString chartSet() const;
  qreal scaleBarLength() const {return m_scaleBarLength;}
  QString scaleBarText() const {return m_scaleBarText;}

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

  void updateViewport(const Camera* cam, quint32 flags = 0);
  void chartSetsChanged(const QStringList& chartSets);
  void chartSetChanged(const QString& chartSet);
  void scaleBarLengthChanged(qreal len);

private:

  QString defaultChartSet() const;
  void computeScaleBar();

  Camera* m_camera;
  bool m_initialized;
  QRectF m_viewArea;
  quint32 m_flags;

  QSize m_size;
  QSize m_orientedSize;
  Qt::ScreenOrientation m_orientation;
  qreal m_scaleBarLength;
  QString m_scaleBarText;
  QPointF m_lastPos;

  QOpenGLContext* m_context;
  QOffscreenSurface* m_surface;
  QOpenGLVertexArrayObject m_vao;

};
