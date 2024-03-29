#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PROJ;

uniform int hasNormals;
uniform int hasTextures;

out vec2 uv;

void main() {
    uv = aTexCoord;
    gl_Position = PROJ * VIEW * MODEL * vec4(aPos, 1.0f);
}
