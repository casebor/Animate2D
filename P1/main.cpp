#include <OpenGP/GL/Application.h>
#include <OpenGP/external/LodePNG/lodepng.cpp>
#include "Mesh.h"

using namespace OpenGP;

const int width = 720, height = 720;
typedef Eigen::Transform<float, 3, Eigen::Affine> Transform;

const char* fb_vshader =
#include "fb_vshader.glsl"
;
const char* fb_fshader =
#include "fb_fshader.glsl"
;
const char* quad_vshader =
#include "quad_vshader.glsl"
;
const char* quad_fshader =
#include "quad_fshader.glsl"
;

const float SpeedFactor = 1;
void init();
void quadInit(std::unique_ptr<GPUMesh>& quad);
void loadTexture(std::unique_ptr<RGBA8Texture>& texture, const char* filename);
void drawCircle(GLfloat, GLfloat, GLfloat, GLfloat, GLint);
void drawPoly();
void drawScene(float timeCount);

std::unique_ptr<GPUMesh> quad;

std::unique_ptr<Shader> quadShader;
std::unique_ptr<Shader> fbShader;

std::unique_ptr<RGBA8Texture> cat;
std::unique_ptr<RGBA8Texture> night;
Mesh sun;

/// TODO: declare Framebuffer and color buffer texture

std::unique_ptr<Framebuffer> fb;
std::unique_ptr<RGBA8Texture> c_buf;


// ----- TRIANGLE
const GLfloat vpoint[] = {
       0.6f, 0.6f, 0.0f,
       0.8f, 0.6f, 0.0f,
       0.7f,  0.8f, 0.0f,};
GLuint programID;
GLuint vao;
GLuint vbo;

// ----- TRIANGLE FAN
GLuint circVboId;
const unsigned DIV_COUNT = 360;



// Bezier
int xi,yi,xf,yf,y1b,x1b,y2b,x2b;
float px, py, t;



int main(int, char**) {

	Application app;
	init();

	/// TODO: initialize framebuffer
	fb = std::unique_ptr<Framebuffer>(new Framebuffer());

	/// TODO: initialize color buffer texture, and allocate memory
	c_buf = std::unique_ptr<RGBA8Texture>(new RGBA8Texture());
	c_buf->allocate(width, height);

	/// TODO: attach color texture to framebuffer
	fb->attach_color_texture(*c_buf);

	Window& window = app.create_window([&](Window&) {
		glViewport(0, 0, width, height);

		/// TODO: First draw the scene onto framebuffer
		/// bind and then unbind framebuffer
		fb->bind();
		glClear(GL_COLOR_BUFFER_BIT);
		drawScene(glfwGetTime());
		fb->unbind();

		/// Render to Window, uncomment the lines and do TODOs
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		fbShader->bind();
		/// TODO: Bind texture and set uniforms
		glActiveTexture(GL_TEXTURE0);
		c_buf->bind();
		fbShader->set_uniform("tex", 0);
        fbShader->set_uniform("tex_width", float(width));
		fbShader->set_uniform("tex_height", float(height));
        quad->set_attributes(*fbShader);
        quad->draw();

        //drawCircle(width/2, height/2, 0, 120, 360);
        //drawPoly();

        // ----- DRAW SUN
        float time_s = glfwGetTime();
        float freq = 3.14 * time_s * 1;
        Transform sun_M = Transform::Identity();
        sun_M *= Eigen::Translation3f(0.5, 0.5, 0.0);
        //sun_M *= Eigen::AngleAxisf(-freq/30, Eigen::Vector3f::UnitZ());
        //scale_t: make the sun become bigger and smaller over the time!
        //float scale_t = 0.01*std::sin(freq);
        sun_M *= Eigen::AlignedScaling3f(0.2, 0.2, 1.0);
        sun.draw(sun_M.matrix());


		c_buf->unbind();
		fbShader->unbind();


        // ----- DRAW TRIANGLE
        //glUseProgram(programID);
        //glBindVertexArray(vao);
        //glDrawArrays(GL_TRIANGLES, 0, 3);


        // ----- DRAW TRIANGLE FAN
        glUseProgram(programID);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, DIV_COUNT + 2);



	});
    window.set_title("FrameBuffer");
	window.set_size(width, height);

	return app.run();
}

void init() {
	glClearColor(1, 1, 1, /*solid*/1.0);

	fbShader = std::unique_ptr<Shader>(new Shader());
	fbShader->verbose = true;
	fbShader->add_vshader_from_source(fb_vshader);
	fbShader->add_fshader_from_source(fb_fshader);
	fbShader->link();

    quadShader = std::unique_ptr<Shader>(new Shader());
	quadShader->verbose = true;
	quadShader->add_vshader_from_source(quad_vshader);
	quadShader->add_fshader_from_source(quad_fshader);
	quadShader->link();

	quadInit(quad);

    loadTexture(cat, "nyancat.png");
    loadTexture(night, "night.png");


    // ----- SUN
    sun.init();

    std::vector<OpenGP::Vec3> quadVert;
    quadVert.push_back(OpenGP::Vec3(-1.0f, -1.0f, 0.0f));
    quadVert.push_back(OpenGP::Vec3(1.0f, -1.0f, 0.0f));
    quadVert.push_back(OpenGP::Vec3(1.0f, 1.0f, 0.0f));
    quadVert.push_back(OpenGP::Vec3(-1.0f, 1.0f, 0.0f));
    std::vector<unsigned> quadInd;
    quadInd.push_back(0);
    quadInd.push_back(1);
    quadInd.push_back(2);
    quadInd.push_back(0);
    quadInd.push_back(2);
    quadInd.push_back(3);
    sun.loadVertices(quadVert, quadInd);

    std::vector<OpenGP::Vec2> quadTCoord;
    quadTCoord.push_back(OpenGP::Vec2(0.0f, 0.0f));
    quadTCoord.push_back(OpenGP::Vec2(1.0f, 0.0f));
    quadTCoord.push_back(OpenGP::Vec2(1.0f, 1.0f));
    quadTCoord.push_back(OpenGP::Vec2(0.0f, 1.0f));
    sun.loadTexCoords(quadTCoord);

    sun.loadTextures("sun.png");




    // ---- CIRCLE VIA TRIANGLE FAN
    glClearColor(0, 1, 0, /*solid*/1.0);
    programID = OpenGP::load_shaders("vshader.glsl", "fshader.glsl");
    glUseProgram(programID);

    vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);

    // MOVE TO FUNCTION & RETURN COORDA
    // Will use a triangle fan rooted at the origin to draw the circle. So one additional
    // point is needed for the origin, and another one because the first point is repeated
    // as the last one to close the circle.
    GLfloat* coordA = new GLfloat[(DIV_COUNT + 2) * 2];

    float offsetX = 0.5f;
    float offsetY = 0.2f;
    float radius = 0.2f;

    // Origin.
    unsigned coordIdx = 0;
    coordA[coordIdx++] = 0.0f;
    coordA[coordIdx++] = 0.0f;

    // Calculate angle increment from point to point, and its cos/sin.
    float angInc = 2.0f * M_PI / static_cast<float>(DIV_COUNT);
    float cosInc = cos(angInc);
    float sinInc = sin(angInc);

    // Start with vector (1.0f, 0.0f), ...
    coordA[coordIdx++] = radius;
    coordA[coordIdx++] = 0.0f;

    // ... and then rotate it by angInc for each point.
    float xc = radius;
    float yc = 0.0f;
    for (unsigned iDiv = 1; iDiv < DIV_COUNT; ++iDiv) {
        float xcNew = cosInc * xc - sinInc * yc;

        yc = sinInc * xc + cosInc * yc;
        xc = xcNew;

        coordA[coordIdx++] = xcNew;
        coordA[coordIdx++] = yc;
    }

    // Repeat first point as last point to close circle.
    coordA[coordIdx++] = radius;
    coordA[coordIdx++] = 0.0f;

    // Then shift
    coordIdx = 0;
    for (unsigned iDiv = 1; iDiv <= DIV_COUNT + 2; ++iDiv) {
        coordA[coordIdx++] += offsetX;
        coordA[coordIdx++] += offsetY;
    }

    //GLuint vboId = 0;
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (DIV_COUNT + 2) * 2 * sizeof(GLfloat), coordA, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    delete[] coordA;



}

void quadInit(std::unique_ptr<GPUMesh>& quad) {
	quad = std::unique_ptr<GPUMesh>(new GPUMesh());
	std::vector<Vec3> quad_vposition = {
		Vec3(-1, -1, 0),
		Vec3(-1,  1, 0),
		Vec3(1, -1, 0),
		Vec3(1,  1, 0)
	};
	quad->set_vbo<Vec3>("vposition", quad_vposition);
	std::vector<unsigned int> quad_triangle_indices = {
		0, 2, 1, 1, 2, 3
	};
	quad->set_triangles(quad_triangle_indices);
	std::vector<Vec2> quad_vtexcoord = {
		Vec2(0, 0),
		Vec2(0,  1),
		Vec2(1, 0),
		Vec2(1,  1)
	};
	quad->set_vtexcoord(quad_vtexcoord);
}

void loadTexture(std::unique_ptr<RGBA8Texture>& texture, const char* filename) {
	// Used snippet from https://raw.githubusercontent.com/lvandeve/lodepng/master/examples/example_decode.cpp
	std::vector<unsigned char> image; //the raw pixels
	unsigned width, height;
	//decode
	unsigned error = lodepng::decode(image, width, height, filename);
	//if there's an error, display it
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...

	// unfortunately they are upside down...lets fix that
	unsigned char* row = new unsigned char[4 * width];
	for (int i = 0; i < int(height) / 2; ++i) {
		memcpy(row, &image[4 * i * width], 4 * width * sizeof(unsigned char));
		memcpy(&image[4 * i * width], &image[image.size() - 4 * (i + 1) * width], 4 * width * sizeof(unsigned char));
		memcpy(&image[image.size() - 4 * (i + 1) * width], row, 4 * width * sizeof(unsigned char));
	}
	delete row;

	texture = std::unique_ptr<RGBA8Texture>(new RGBA8Texture());
	texture->upload_raw(width, height, &image[0]);
}



void bezier(int xi, int yi, int xf, int yf)
{

    // Coordinates for additional points of bezier curve
    x1b = xi + rand()%15;
    y1b = yi + rand()%5;
    x2b = xf + rand()%15;
    y2b = yf + rand()%5;

    //calculate and draw the curves
    for (t = 0; t <= 1; t += 0.001)
    {
        px =(1-t)*(1-t)*(1-t)*xi+3*t*(1-t)*(1-t)*x1b+3*t*t*(1-t)*x2b+t*t*t*xf;
        py =(1-t)*(1-t)*(1-t)*yi+3*t*(1-t)*(1-t)*y1b+3*t*t*(1-t)*y2b+t*t*t*yf;
        glPointSize(2);
        glBegin(GL_POINTS);
            //glColor3f(1,0,0);
            glVertex2d(px,py);
        glEnd();
        glFlush();
    }

}


/*
void drawPoly() {
    // Number of segments the circle is divided into.
    const unsigned DIV_COUNT = 32;

    // Will use a triangle fan rooted at the origin to draw the circle. So one additional
    // point is needed for the origin, and another one because the first point is repeated
    // as the last one to close the circle.
    GLfloat* coordA = new GLfloat[(DIV_COUNT + 2) * 2];

    // Origin.
    unsigned coordIdx = 0;
    coordA[coordIdx++] = 0.0f;
    coordA[coordIdx++] = 0.0f;

    // Calculate angle increment from point to point, and its cos/sin.
    float angInc = 2.0f * M_PI / static_cast<float>(DIV_COUNT);
    float cosInc = cos(angInc);
    float sinInc = sin(angInc);

    // Start with vector (1.0f, 0.0f), ...
    coordA[coordIdx++] = 1.0f;
    coordA[coordIdx++] = 0.0f;

    // ... and then rotate it by angInc for each point.
    float xc = 1.0f;
    float yc = 0.0f;
    for (unsigned iDiv = 1; iDiv < DIV_COUNT; ++iDiv) {
        float xcNew = cosInc * xc - sinInc * yc;
        yc = sinInc * xc + cosInc * yc;
        xc = xcNew;

        coordA[coordIdx++] = xc;
        coordA[coordIdx++] = yc;
    }

    // Repeat first point as last point to close circle.
    coordA[coordIdx++] = 1.0f;
    coordA[coordIdx++] = 0.0f;

    GLuint vboId = 0;
    glGenBuffers(1, &circVboId);
    glBindBuffer(GL_ARRAY_BUFFER, circVboId);
    glBufferData(GL_ARRAY_BUFFER, (DIV_COUNT + 2) * 2 * sizeof(GLfloat), coordA, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    delete[] coordA;

    glBindBuffer(GL_ARRAY_BUFFER, circVboId);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posLoc);

    glDrawArrays(GL_TRIANGLE_FAN, 0, DIV_COUNT + 2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
*/

/*
void drawCircle(GLfloat x, GLfloat y, GLfloat z, GLfloat radius, GLint numSides) {
    GLint numVertices = numSides + 2;
    //GLfloat doublePi = 2.0f & M_PI;
    GLfloat doublePi = 3.14f;

    GLfloat circleVerticesX[numVertices];
    GLfloat circleVerticesY[numVertices];
    GLfloat circleVerticesZ[numVertices];

    circleVerticesX[0] = x;
    circleVerticesY[0] = y;
    circleVerticesZ[0] = z;

    for (int i = 1; i < numVertices; i++) {
        circleVerticesX[i] = x + ( radius * cos(i * doublePi / numSides));
        circleVerticesY[i] = y + ( radius * sin(i * doublePi / numSides));
        circleVerticesZ[i] = z;
    }

    GLfloat allVertices[numVertices * 3];

    for (int i = 1; i < numVertices; i++) {
        allVertices[i * 3] = circleVerticesX[i];
        allVertices[(i * 3) + 1] = circleVerticesY[i];
        allVertices[(i * 3) + 2] = circleVerticesZ[i];
    }


    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, allVertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, numVertices);
    glDisableClientState(GL_VERTEX_ARRAY);
}
*/


void drawScene(float timeCount)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float t = timeCount * SpeedFactor;
	Transform TRS = Transform::Identity();
	quadShader->bind();
	quadShader->set_uniform("M", TRS.matrix());
	// Make texture unit 0 active
    glActiveTexture(GL_TEXTURE0);
	// Bind the texture to the active unit for drawing
	night->bind();
	// Set the shader's texture uniform to the index of the texture unit we have
	// bound the texture to
	quadShader->set_uniform("tex", 0);
	quad->set_attributes(*quadShader);
    quad->draw();
	night->unbind();

	float xcord = 0.7 * std::cos(t);
	float ycord = 0.7 * std::sin(t);
	TRS *= Eigen::Translation3f(xcord, ycord, 0);
	TRS *= Eigen::AngleAxisf(t + M_PI / 2, Eigen::Vector3f::UnitZ());
	TRS *= Eigen::AlignedScaling3f(0.2f, 0.2f, 1);

	quadShader->bind();
	quadShader->set_uniform("M", TRS.matrix());
	// Make texture unit 0 active
    glActiveTexture(GL_TEXTURE0);
	// Bind the texture to the active unit for drawing
    cat->bind();
	// Set the shader's texture uniform to the index of the texture unit we have
	// bound the texture to
	quadShader->set_uniform("tex", 0);
	quad->set_attributes(*quadShader);

    quad->draw();
    cat->unbind();
    quadShader->unbind();








    glDisable(GL_BLEND);


}



