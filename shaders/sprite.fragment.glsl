#version 460 core

uniform sampler2D albedoMap;

in vec2 fragTexCoord;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(albedoMap, fragTexCoord);

    // no transparent fragments in depth buffer
    if (texColor.a < 0.001) discard;

    fragColor = texColor;
}
