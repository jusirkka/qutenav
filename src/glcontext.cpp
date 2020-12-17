#include "glcontext.h"
#include <QOpenGLWindow>

GL::Context* GL::Context::instance() {
  static Context* ctx = new Context;
  return ctx;
}

void GL::Context::makeCurrent() {
  m_window->makeCurrent();
}

void GL::Context::doneCurrent() {
  m_window->doneCurrent();
}

void GL::Context::initializeContext(QOpenGLWindow* w) {
  m_window = w;
}
