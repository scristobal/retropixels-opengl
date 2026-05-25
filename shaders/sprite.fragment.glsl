#version 460 core

layout (binding = 0) uniform sampler2D albedoMap;

layout (location = 0) in vec2 fragTexCoord;

layout (location = 0) out vec4 fragColor;

void main() {
    vec4 texColor = texture(albedoMap, fragTexCoord);

    // no transparent fragments in depth buffer
    if (texColor.a < 0.001) discard;

    fragColor = texColor;
}
