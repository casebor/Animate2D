#version 330 core
// glsl/fshader.glsl
in vec4 vertexColor;
out vec4 color;
void main(){

/// @TODO---set the in variable vertexColor to color 
    //color = vec4(1.0, 0.0, 0.0, 1.0);
    //vertexColor = color;

    color = vertexColor;
}
