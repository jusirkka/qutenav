#pragma once

class QWindow;
class QOpenGLContext;

namespace GL {

class Context {
public:
  Context() = default;

  static Context* instance();
  void makeCurrent();
  void doneCurrent();
  void initializeContext(QOpenGLContext* c, QWindow* w);

private:

  QOpenGLContext* m_context;
  QWindow* m_window;

};

} // namespace GL

