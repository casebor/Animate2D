#version 330 core
in vec2 uv;

out vec4 FragColor;

uniform int hasNormals;
uniform int hasTextures;

uniform sampler2D texImage;

void main() {
    // Texture with colour
    FragColor = vec4(0.56, 0.46, 0.22, 1.0);

    // Texture with image
    //texture(texImage,uv).rgba;
}
