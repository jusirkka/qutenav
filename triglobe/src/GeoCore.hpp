#pragma once

// Useful geometry utilities

#include "precision.h"

/**
 * \brief Computes the tangent space of a normal N
 *
 * \param N Normal (must be normalized)
 * \param T Tangent
 * \param B Binormal
 */
inline void TangentSpace(const vec3& N, vec3& T, vec3& B) {
  if (N.x > 0.5f || N.y > 0.5f) {
    T = vec3(N.y, -N.x, 0.f);
  } else {
    T = vec3(-N.z, 0.f, N.x);
  }
  T = glm::normalize(T);
  B = glm::cross(N, T);
}
