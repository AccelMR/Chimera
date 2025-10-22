#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(binding = 0) uniform ProjectionViewModel {
    mat4 projection;
    mat4 view;
    mat4 model;
} pvm;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosition;

void main() {
    vec4 worldPos = pvm.model * vec4(inPosition, 1.0);
    
    fragPosition = worldPos.xyz;
    
    gl_Position = pvm.projection * pvm.view * worldPos;
    
    fragNormal = mat3(pvm.model) * inNormal;
    
    fragTexCoord = inTexCoord;
}