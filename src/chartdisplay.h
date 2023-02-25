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
#include <QGeoCoordinate>

class DetailMode;
class Camera;
class UpdaterInterface;
class QTimer;

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
  Q_INVOKABLE void infoQuery(const QPointF& p, bool full = false);
  Q_INVOKABLE void setEye(const QGeoCoordinate& q);
  Q_INVOKABLE QPointF position(const QGeoCoordinate& q) const;
  Q_INVOKABLE QGeoCoordinate tocoord(const QPointF& p) const;
  Q_INVOKABLE QPointF advance(const QGeoCoordinate& q, qreal distance, qreal heading) const;
  Q_INVOKABLE void updateChartDB(bool fullUpdate);
  Q_INVOKABLE QString displayBearing(const QGeoCoordinate& q1, const QGeoCoordinate& q2, bool swap) const;
  Q_INVOKABLE QPointF infoPosition() const;
  Q_INVOKABLE void setEulaShown(const QString& path) const;

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

  Q_PROPERTY(QStringList scaleBarTexts
             READ scaleBarTexts
             NOTIFY scaleBarTextsChanged)

  Q_PROPERTY(QString scaleBarFontFamily
             READ scaleBarFontFamily
             CONSTANT)

  Q_PROPERTY(int scaleBarFontPointSize
             READ scaleBarFontPointSize
             CONSTANT)

  Q_PROPERTY(bool scaleBarFontBold
             READ scaleBarFontBold
             CONSTANT)

  Q_PROPERTY(bool processingCharts
             READ processingCharts
             NOTIFY processingChartsChanged)

  using PointVector = QVector<QPointF>;
  void syncPositions(const WGS84PointVector& positions, PointVector& vertices) const;
  void syncPositions(const KV::EventString& events, PointVector& vertices) const;

  QStringList chartSets() const;
  QString chartSet() const;
  qreal scaleBarLength() const {return m_scaleBarLength;}
  QStringList scaleBarTexts() const {return m_scaleBarText;}
  QString scaleBarFontFamily() const {return scale_bar_font_family;}
  int scaleBarFontPointSize() const {return scale_bar_font_point_size;}
  bool scaleBarFontBold() const {return scale_bar_font_bold;}
  bool processingCharts() const {return m_processingCharts;}

  void setChartSet(const QString& s);

  Renderer* createRenderer() const override;

  void setCamera(Camera* cam);
  void checkOutlines() const;
  const Camera* camera() const {return m_camera;}
  bool consume(quint32 flag);
  const QRectF& viewArea() const {return m_viewArea;}

  WGS84Point location(const QPointF& pos) const;
  QPointF position(const WGS84Point& wp) const;

  void indicateBusy(bool busy);

  static const quint32 ChartsUpdated = 1;
  static const quint32 EnteringChartMode = 2;
  static const quint32 LeavingChartMode = 4;
  static const quint32 ChartSetChanged = 8;
  static const quint32 ColorTableChanged = 16;
  static const quint32 GlyphAtlasChanged = 32;
  static const quint32 ProxyChanged = 64;


private slots:

  void handleWindowChanged(QQuickWindow *win);
  void initializeSG();
  void finalizeSG();
  void resize(int l = 0); // dummy argument
  void orient(Qt::ScreenOrientation orientation);

  void handleFullInfoResponse(const S57::InfoType& info);
  void updateChartSet();
  void requestChartDBUpdate();
  void requestChartDBFullUpdate();
  void handleUpdaterStatus(const QString& msg);

signals:

  void updateViewport(const Camera* cam, quint32 flags = 0);
  void chartSetsChanged();
  void chartSetChanged(const QString& chartSet);
  void scaleBarLengthChanged(qreal len);
  void infoQueryReady(const QString& objectId, const QString& info);
  void infoQueryFullReady(const QList<QObject*>& info);
  void infoRequest(const WGS84Point& p, bool full);
  void chartDBStatus(const QString& msg);
  void scaleBarTextsChanged();
  void processingChartsChanged();
  void showEula(const QString& path, const QString& eula);

private:

  static const inline QString scale_bar_font_family = "sans-serif";
  static const inline int scale_bar_font_point_size = 18;
  static const inline bool scale_bar_font_bold = true;


  //% "Full sync ready"
  static const inline char* status_full_sync_ready = QT_TRID_NOOP("qutenav-status-full-sync-ready");
  //% "Sync ready"
  static const inline char* status_sync_ready = QT_TRID_NOOP("qutenav-status-sync-ready");
  //% "Inserted %n"
  static const inline char* status_insert_1 = QT_TRID_NOOP("qutenav-status-insert-1");
  //% "/%n charts"
  static const inline char* status_insert_2 = QT_TRID_NOOP("qutenav-status-insert-2");
  //% "Updated %n"
  static const inline char* status_update_1 = QT_TRID_NOOP("qutenav-status-update-1");
  //% "/%n charts"
  static const inline char* status_update_2 = QT_TRID_NOOP("qutenav-status-update-2");

  QString defaultChartSet() const;
  void computeScaleBar();

  Camera* m_camera;
  bool m_initialized;
  bool m_processingCharts;
  QTimer* m_busyTimer;
  QRectF m_viewArea;
  quint32 m_flags;

  QSize m_size;
  QSize m_orientedSize;
  Qt::ScreenOrientation m_orientation;
  qreal m_scaleBarLength;
  QStringList m_scaleBarText;
  QPointF m_lastPos;

  QObjectList m_info;
  WGS84Point m_lastInfoPos {};

  UpdaterInterface* m_updater;

};
