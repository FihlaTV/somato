uniform mat4 modelToCameraMatrix;
uniform mat4 cameraToClipMatrix;

in vec3 position;

smooth out float interpIntensity;

const float gridIntensity = 0.3;

void main()
{
  vec4 posCamSpace = modelToCameraMatrix * vec4(position, 1.);

  gl_Position = cameraToClipMatrix * posCamSpace;
  interpIntensity = clamp(0.08 * posCamSpace.z + 1., 0., 1.) * gridIntensity;
}