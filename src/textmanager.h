#pragma once

#include "types.h"
#include "s57object.h"
#include "glyphmanager.h"
#include <QRectF>
#include <QHash>
#include <QOpenGLBuffer>
#include <QMutex>
#include <QSharedData>

class QThread;
class TextShaper;
class QOpenGLTexture;
class QTimer;


struct TextSharedData: public QSharedData {
  TextSharedData()
    : bbox() {}

  TextSharedData(const TextSharedData& d)
    : QSharedData(d)
    , bbox(d.bbox)
    , elements(d.elements)
    , vertexOffset(d.vertexOffset) {}

  QRectF bbox;
  S57::ElementData elements;
  GLsizei vertexOffset;

};

class TextData {
public:


  TextData()
    : d(new TextSharedData) {}

  TextData(const QRectF& box, const S57::ElementData data, GLsizei off)
    : d(new TextSharedData) {
    d->bbox = box;
    d->elements = data;
    d->vertexOffset = off;
  }

  bool isValid() const {return d->bbox.isValid();}
  const QRectF& bbox() const {return d->bbox;}
  const S57::ElementData& elements() const {return d->elements;}
  GLsizei offset() const {return d->vertexOffset;}

private:

  QSharedDataPointer<TextSharedData> d;

};


namespace GL {
struct Mesh;

struct GlyphData {
  int width;
  int height;
  const uchar* data;
};

}

Q_DECLARE_METATYPE(GL::GlyphData)

struct TextKey {

  TextKey(const QString& txt, TXT::Weight w, TXT::HJust h, TXT::VJust v,
          quint8 b, qint8 x, qint8 y)
    : text(txt)
    , weight(w)
    , hjust(h)
    , vjust(v)
    , bodySize(b)
    , offsetX(x)
    , offsetY(y) {}


  TextKey() = default;

  QString text;
  TXT::Weight weight;
  TXT::HJust hjust;
  TXT::VJust vjust;
  quint8 bodySize;
  qint8 offsetX;
  qint8 offsetY;

};

Q_DECLARE_METATYPE(TextKey)

inline bool operator== (const TextKey& k1, const TextKey& k2) {
  if (k1.text != k2.text) return false;
  if (k1.weight != k2.weight) return false;
  if (k1.hjust != k2.hjust) return false;
  if (k1.vjust != k2.vjust) return false;
  if (k1.bodySize != k2.bodySize) return false;
  if (k1.offsetX != k2.offsetX) return false;
  return k1.offsetY == k2.offsetY;
}

inline uint qHash(const TextKey& key) {
  const QVector<quint64> parts {
    as_numeric(key.weight),
    as_numeric(key.hjust),
    as_numeric(key.vjust),
    key.bodySize,
    static_cast<quint8>(key.offsetX),
    static_cast<quint8>(key.offsetY),
  };

  int shift = 0;
  quint64 k = std::accumulate(parts.begin(), parts.end(), 0, [&shift] (quint64 s, quint64 d) {
      const quint64 r = s + (d << shift);
      shift += 8;
      return r;
  });
  return qHash(qMakePair(key.text, k));
}

class TextManager: public QObject {

  Q_OBJECT

public:

  TextManager();

  static TextManager* instance();

  int ticket(const QString& txt,
             TXT::Weight weight,
             TXT::HJust hjust,
             TXT::VJust vjust,
             quint8 bodySize,
             qint8 offsetX,
             qint8 offsetY);

  void createTexture(int w, int h);

  int atlasWidth() const;
  int atlasHeight() const;

  GL::VertexVector shapeData(int ticket) const {return m_shapeData[ticket];}

  void bind();

  ~TextManager();


signals:

  void newStrings();

private slots:

  void handleShape(const TextKey& key, GL::Mesh* mesh, bool newGlyphs);
  void requestUpdate();

private:

  using TextMap = QHash<TextKey, int>;
  using DataVector = QVector<GL::VertexVector>;

  QMutex m_mutex;
  QThread* m_thread;
  TextShaper* m_worker;
  TextMap m_tickets;
  DataVector m_shapeData;
  QOpenGLTexture* m_glyphTexture;
  QTimer* m_shapeTimer;
};


class TextShaper: public QObject {

  Q_OBJECT

public:

  TextShaper(QMutex* mutex);

  GL::GlyphData atlas() const;

public slots:

  void shape(const TextKey& key);

signals:

  void done(const TextKey& key, GL::Mesh* mesh, bool newGlyphs);

private:

  static const inline QMap<TXT::HJust, float> m_pivotHMap {
    {TXT::HJust::Left, 0.f}, {TXT::HJust::Centre, -.5f}, {TXT::HJust::Right, -1.f}
  };

  static const inline QMap<TXT::VJust, float> m_pivotVMap {
    {TXT::VJust::Top, 0.f}, {TXT::VJust::Centre, .5f}, {TXT::VJust::Bottom, 1.f}
  };


  GlyphManager m_manager;

};
