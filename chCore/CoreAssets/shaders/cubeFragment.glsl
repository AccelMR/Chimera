// Fragment Shader
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosition;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    vec3 ambient = vec3(0.1, 0.1, 0.1);
    
    // Obtener el color de la textura
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Combinar iluminación con textura
    outColor = vec4((ambient + diffuse), 1.0) * texColor;
}