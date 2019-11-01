// glsl/vshader.glsl
#version 330 core 
in vec3 vpoint;
out vec4 vertexColor;

void main(){
    gl_Position = vec4(vpoint, 1.0);
    //vertexColor = vec4(0.0, 1.0, 0.0, 1.0);

    // Gold
    vertexColor = vec4(0.83, 0.69, 0.22, 1.0);
}
