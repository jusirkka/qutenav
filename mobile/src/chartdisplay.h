#pragma once

#include <QtQuick/QQuickFramebufferObject>
#include <QOpenGLVertexArrayObject>
#include "types.h"
#include <QSGGeometry>

class DetailMode;
class Camera;
class QOpenGLContext;
class QOffscreenSurface;

class AttributeObject: public QObject {

  Q_OBJECT

public:

  AttributeObject(const QString& n, const QString& v, QObject* parent = nullptr);

  Q_PROPERTY(QString name
             MEMBER name
             WRITE setValue
             NOTIFY nameChanged)

  Q_PROPERTY(QString value
             MEMBER value
             WRITE setValue
             NOTIFY valueChanged)

  QString name;
  QString value;

  void setName(const QString&) {/* noop */}
  void setValue(const QString&) {/* noop */}

signals:

  void nameChanged();
  void valueChanged();
};

class ObjectObject: public QObject {

  Q_OBJECT

public:

  ObjectObject(const QString& n, QObject* parent = nullptr);

  Q_PROPERTY(QString name
             MEMBER name
             WRITE setName
             NOTIFY nameChanged)

  Q_PROPERTY(QList<QObject*> attributes
             MEMBER attributes
             WRITE setAttributes
             NOTIFY attributesChanged)

  QString name;
  QList<QObject*> attributes;

  void setName(const QString&) {/* noop */}
  void setAttributes(const QList<QObject*>&) {/* noop */}

signals:

  void nameChanged();
  void attributesChanged();
};


class ChartDisplay: public QQuickFramebufferObject {

  Q_OBJECT

public:

  ChartDisplay();
  ~ChartDisplay();

  Q_INVOKABLE void advanceNMEALog(int secs) const;

  Q_INVOKABLE void zoomIn();
  Q_INVOKABLE void zoomOut();
  Q_INVOKABLE void panStart(qreal x, qreal y);
  Q_INVOKABLE void pan(qreal x, qreal y);
  Q_INVOKABLE void northUp();
  Q_INVOKABLE void rotate(qreal degrees);
  Q_INVOKABLE void infoQuery(const QPointF& p);
  Q_INVOKABLE void setEye(qreal lng, qreal lat);
  Q_INVOKABLE QPointF position(qreal lng, qreal lat) const;
  Q_INVOKABLE QPointF advance(qreal lng, qreal lat, qreal distance, qreal heading) const;

  Q_PROPERTY(QStringList chartSets
             READ chartSets
             NOTIFY chartSetsChanged)

  Q_PROPERTY(QString chartSet
             READ chartSet
             WRITE setChartSet
             NOTIFY chartSetChanged)

  Q_PROPERTY(qreal scaleBarLength
             READ scaleBarLength
             NOTIFY scaleBarLengthChanged)

  Q_PROPERTY(QString scaleBarText
             READ scaleBarText)

  using Point2DVector = QVector<QSGGeometry::Point2D>;
  void syncPositions(const WGS84PointVector& positions, Point2DVector& vertices) const;

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

  WGS84Point location(const QPointF& pos) const;
  QPointF position(const WGS84Point& wp) const;


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

  void handleInfoResponse(const S57::InfoType& info);

signals:

  void updateViewport(const Camera* cam, quint32 flags = 0);
  void chartSetsChanged(const QStringList& chartSets);
  void chartSetChanged(const QString& chartSet);
  void scaleBarLengthChanged(qreal len);
  void infoQueryReady(const QList<QObject*>& info);
  void infoRequest(const WGS84Point& p);

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

  QObjectList m_info;

};
