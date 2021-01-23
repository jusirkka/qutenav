#pragma once

#include <QThread>

class QOpenGLContext;
class QOffscreenSurface;

namespace GL {

class Thread: public QThread {

  Q_OBJECT

public:

  Thread(QOpenGLContext* ctx);
  ~Thread();

private:

  void run() override;

  QOpenGLContext* m_context;
  QOffscreenSurface* m_surface;

};

} // namespace GL

