#pragma once

#include <QThread>

class QOpenGLContext;

namespace GL {

class Thread: public QThread {

  Q_OBJECT

public:

  Thread(QOpenGLContext* ctx)
    : QThread()
    , m_mainContext(ctx) {}

private:

  void run() override;

  QOpenGLContext* m_mainContext;

};

} // namespace GL

