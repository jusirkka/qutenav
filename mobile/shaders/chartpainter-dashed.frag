#version 310 es

uniform int pattern; // an integer representing the bitwise pattern
uniform int patlen; // pattern length
uniform int factor;
uniform vec4 base_color;

in float texCoord;
out vec4 color;

void main(void) {
  // create a filter with period patlen * factor. factor consecutive
  // pixels are mapped to the same bitpos.
  uint bitpos = (uint(round(texCoord)) % (patlen * factor)) / factor;
  uint bit = (1U << bitpos);

  // discard the bit if it doesn't match the masking pattern
  uint up = uint(pattern);
  if ((up & bit) == 0U) {
    color = vec4(base_color.rgb, 0.);
  } else {
    color = base_color;
  }
}
