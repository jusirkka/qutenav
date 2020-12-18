#pragma once

#include <QGuiApplication>
#include <QScreen>

#define dots_per_mm_x (QGuiApplication::primaryScreen()->physicalDotsPerInchX() / 25.4)
#define dots_per_mm_y (QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4)


