#pragma once

#include <QObject>
#include "types.h"
#include <QRectF>
#include "s57object.h"
#include "types.h"
#include "glyphmanager.h"
#include <QHash>
#include <QOpenGLBuffer>
#include <QMutex>
#include <QSharedData>

class QOpenGLContext;
class QThread;
class TextShaper;
class QOffscreenSurface;
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
  bool newGlyphs;
  int width;
  int height;
  const uchar* data;
};

}

Q_DECLARE_METATYPE(GL::GlyphData)

struct TextKey {

  TextKey(const QString& txt, TXT::Weight w)
    : text(txt)
    , weight(w) {}

  TextKey() = default;

  QString text;
  TXT::Weight weight;
};

Q_DECLARE_METATYPE(TextKey)

inline bool operator== (const TextKey& k1, const TextKey& k2) {
  if (k1.text != k2.text) return false;
  return k1.weight == k2.weight;
}

inline uint qHash(const TextKey& key, uint seed) {
  return qHash(qMakePair(key.text, as_numeric(key.weight)), seed);
}

class TextManager: public QObject {

  Q_OBJECT

public:

  TextManager();

  static TextManager* instance();
  void createContext(QOpenGLContext* ctx);

  TextData textData(const QString& txt, TXT::Weight weight) const;

  int atlasWidth() const;
  int atlasHeight() const;

  void bind();

  ~TextManager();

signals:

  void newStrings();

private slots:

  void handleShape(const TextKey& key, GL::Mesh* mesh, const GL::GlyphData& atlas);
  void requestUpdate();

private:

  using TextMap = QHash<TextKey, TextData>;

  QOpenGLContext* m_context;
  QOffscreenSurface* m_surface;
  QMutex m_mutex;
  QThread* m_thread;
  TextShaper* m_worker;
  TextMap m_textMap;
  TextData m_invalid;
  QOpenGLBuffer m_coordBuffer;
  QOpenGLBuffer m_indexBuffer;
  QOpenGLTexture* m_glyphTexture;
  GLsizei m_vertexOffset;
  GLsizei m_indexOffset;
  QTimer* m_shapeTimer;
};


class TextShaper: public QObject {

  Q_OBJECT

public:

  TextShaper(QMutex* mutex);

public slots:

  void shape(const TextKey& key);

signals:

  void done(const TextKey& key, GL::Mesh* mesh, const GL::GlyphData& atlas);

private:

  GlyphManager m_manager;

};
