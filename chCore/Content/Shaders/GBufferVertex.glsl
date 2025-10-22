#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(std140, binding = 0) uniform ViewUniforms {
    mat4 uView;
    mat4 uProjection;
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

layout(std140, binding = 1) uniform ModelUniforms {
    mat4 uModel;
    mat4 uNormalMatrix;
};

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out mat3 vTBN;

void main() {
  vec4 worldPos = uModel * vec4(aPosition, 1.0);
  vWorldPos = worldPos.xyz;

  vWorldNormal = normalize(mat3(uNormalMatrix) * aNormal);
  vTexCoord = aTexCoord;

  // Build TBN matrix for normal mapping
  vec3 T = normalize(mat3(uNormalMatrix) * aTangent);
  vec3 B = normalize(mat3(uNormalMatrix) * aBitangent);
  vec3 N = vWorldNormal;
  vTBN = mat3(T, B, N);

  gl_Position = uViewProjection * worldPos;
}
