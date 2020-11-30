#include "glthread.h"
#include <QOpenGLContext>
#include <QOffscreenSurface>

void GL::Thread::run() {
  QScopedPointer<QOpenGLContext> ctx(new QOpenGLContext);
  ctx->setFormat(QSurfaceFormat::defaultFormat());
  ctx->setShareContext(m_mainContext);
  ctx->setScreen(m_mainContext->screen());
  ctx->create();

  QScopedPointer<QOffscreenSurface> surface(new QOffscreenSurface);
  surface->setFormat(ctx->format());
  surface->setScreen(ctx->screen());
  surface->create();

  ctx->makeCurrent(surface.data());

  exec();
}
