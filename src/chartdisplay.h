/* -*- coding: utf-8-unix -*-
 *
 * File: mobile/src/chartdisplay.h
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

#include <QtQuick/QQuickFramebufferObject>
#include <QOpenGLVertexArrayObject>
#include "types.h"
#include "event.h"
#include <QSGGeometry>

class DetailMode;
class Camera;
class QOpenGLContext;
class QOffscreenSurface;
class UpdaterInterface;

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
  Q_INVOKABLE void updateChartDB(bool fullUpdate);

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

  using PointVector = QVector<QPointF>;
  void syncPositions(const WGS84PointVector& positions, PointVector& vertices) const;
  void syncPositions(const KV::EventString& events, PointVector& vertices) const;

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
  static const quint32 ColorTableChanged = 16;


private slots:

  void handleWindowChanged(QQuickWindow *win);
  void initializeGL(QOpenGLContext* ctx);
  void initializeSG();
  void finalizeSG();
  void resize(int l = 0); // dummy argument
  void orient(Qt::ScreenOrientation orientation);

  void handleFullInfoResponse(const S57::InfoTypeFull& info);
  void updateChartSet();
  void requestChartDBUpdate();
  void requestChartDBFullUpdate();

signals:

  void updateViewport(const Camera* cam, quint32 flags = 0);
  void chartSetsChanged();
  void chartSetChanged(const QString& chartSet);
  void scaleBarLengthChanged(qreal len);
  void infoQueryReady(const QString& objectId, const QString& info);
  void infoQueryFullReady(const QList<QObject*>& info);
  void infoRequest(const WGS84Point& p);
  void chartDBStatus(const QString& msg);

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

  UpdaterInterface* m_updater;

};
