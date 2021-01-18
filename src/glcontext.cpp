#include "glcontext.h"
#include <QOpenGLWindow>

GL::Context* GL::Context::instance() {
  static Context* ctx = new Context;
  return ctx;
}

void GL::Context::makeCurrent() {
  m_context->makeCurrent(m_window);
}

void GL::Context::doneCurrent() {
  m_context->doneCurrent();
}

void GL::Context::initializeContext(QOpenGLContext* c, QWindow* w) {
  m_context = c;
  m_window = w;
}
