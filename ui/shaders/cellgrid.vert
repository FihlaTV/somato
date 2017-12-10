uniform mat3x4 modelView;
uniform vec4   viewFrustum;

in vec4 position;

smooth out float varIntensity;

const float gridIntensity = 0.3;

void main()
{
  vec3 posCamSpace = position * modelView;
  float fadeIntensity = clamp(0.08 * posCamSpace.z + 1., 0., 1.);

  varIntensity = fadeIntensity * gridIntensity;
  gl_Position  = vec4(posCamSpace.xy * viewFrustum.xy,
                      posCamSpace.z  * viewFrustum.z + viewFrustum.w,
                     -posCamSpace.z);
}
