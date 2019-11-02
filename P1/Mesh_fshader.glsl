#version 330 core
// When you edit these shaders, Clear CMake Configuration so they are copied to build folders
in vec2 uv;

out vec4 FragColor;

uniform int hasNormals;
uniform int hasTextures;

uniform sampler2D texImage;

void main() {
    FragColor = vec4(0.83, 0.69, 0.22, 1.0);
            //texture(texImage,uv).rgba;
}
