#version 450 core

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in mat3 vTBN;

// Material textures
layout(binding = 0) uniform sampler2D uAlbedoMap;
layout(binding = 1) uniform sampler2D uNormalMap;
layout(binding = 2) uniform sampler2D uMetallicMap;
layout(binding = 3) uniform sampler2D uRoughnessMap;
layout(binding = 4) uniform sampler2D uEmissiveMap;

layout(std140, binding = 2) uniform MaterialUniforms {
    vec4 uAlbedoFactor;
    vec3 uEmissiveFactor;
    float uMetallicFactor;
    float uRoughnessFactor;
    float uNormalScale;
    float uOcclusionStrength;
    float uEmissiveStrength;
};

layout(std140, binding = 3) uniform ObjectUniforms {
    uint uMaterialId;    // Material ID for lighting model selection
    uint uObjectId;      // Object ID for picking/outlining
    float uPadding1;
    float uPadding2;
};

// G-Buffer outputs (4 targets + depth)
layout(location = 0) out vec4 gAlbedo;     // RGB: albedo, A: occlusion/alpha
layout(location = 1) out vec4 gNormal;     // RGB: world normal, A: material ID (0-255)
layout(location = 2) out vec4 gMetallic;   // R: metallic, G: roughness, B: emissive intensity, A: object ID
layout(location = 3) out vec4 gEmissive;   // RGB: emissive color, A: unused

void main() {
  if (gl_FragCoord.z > 0.999) discard;

    // Sample textures
    vec4 albedoSample = texture(uAlbedoMap, vTexCoord);
    vec3 normalSample = texture(uNormalMap, vTexCoord).rgb;
    float metallicSample = texture(uMetallicMap, vTexCoord).r;
    float roughnessSample = texture(uRoughnessMap, vTexCoord).r;
    vec3 emissiveSample = texture(uEmissiveMap, vTexCoord).rgb;

    // Calculate final material properties
    vec3 albedo = albedoSample.rgb * uAlbedoFactor.rgb;
    float metallic = metallicSample * uMetallicFactor;
    float roughness = roughnessSample * uRoughnessFactor;
    vec3 emissive = emissiveSample * uEmissiveFactor * uEmissiveStrength;

    // Normal mapping
    vec3 normal = normalize(vWorldNormal);
    if (length(normalSample - 0.5) > 0.01) {
        normalSample = normalSample * 2.0 - 1.0;
        normalSample.xy *= uNormalScale;
        normal = normalize(vTBN * normalSample);
    }

    // Pack outputs efficiently
    gAlbedo = vec4(albedo, albedoSample.a * uOcclusionStrength);

    // Material ID in alpha (0-255 range, packed as 0.0-1.0)
    float materialIdPacked = float(uMaterialId & 0xFFu) / 255.0;
    gNormal = vec4(normal * 0.5 + 0.5, materialIdPacked);

    // Object ID in alpha (supports up to 65535 objects)
    float objectIdPacked = float(uObjectId & 0xFFFFu) / 65535.0;
    gMetallic = vec4(metallic, roughness, length(emissive), objectIdPacked);

    gEmissive = vec4(emissive, 1.0);
}
