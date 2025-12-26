#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in mat4 modelMatrix;

uniform mat4 texTransform;

out vec2 fragTexCoord;

void main() {
    gl_Position = modelMatrix *  vec4(position, 1.0);
    fragTexCoord =  (texTransform * vec4(texCoord.xy, 0.0, 1.0)).xy;
}
