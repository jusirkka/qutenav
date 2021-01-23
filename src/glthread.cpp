#include "glthread.h"
#include <QOpenGLContext>
#include <QOffscreenSurface>


GL::Thread::Thread(QOpenGLContext *ctx)
  : QThread()
{
  m_context = new QOpenGLContext;
  m_context->setFormat(QSurfaceFormat::defaultFormat());
  m_context->setShareContext(ctx);
  m_context->setScreen(ctx->screen());
  m_context->create();
  m_context->moveToThread(this);

  m_surface = new QOffscreenSurface;
  m_surface->setFormat(m_context->format());
  m_surface->create();

  moveToThread(this);
}

void GL::Thread::run() {
  m_context->makeCurrent(m_surface);
  exec();
}

GL::Thread::~Thread() {
  m_context->doneCurrent();
  delete m_context;
  // schedule this to be deleted only after we're done cleaning up
  m_surface->deleteLater();
}
