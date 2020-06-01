#include "drawable.h"

Drawable::Drawable(QObject *parent): QObject(parent) {}

void Drawable::updateCharts() {
  updateBuffers();
  updateObjects();
}
