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
Vec3 getPointBezier(Vec3, Vec3, Vec3, Vec3, float);
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
GLuint programIDCircle;
//GLuint vao;
GLuint vao[3];

// ----- TRIANGLE FAN CIRCLE
const unsigned numStepsFan = 360;

// ----- TRIANGLE FAN WINGS
const unsigned numSteps = 30;

GLuint programIDWingRight;
GLuint programIDWingLeft;



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
        //sun.draw(sun_M.matrix());


		c_buf->unbind();
		fbShader->unbind();


        // ----- DRAW TRIANGLE
        //glUseProgram(programID);
        //glBindVertexArray(vao);
        //glDrawArrays(GL_TRIANGLES, 0, 3);


        // ----- DRAW TRIANGLE FAN - CIRCLE
        glUseProgram(programIDCircle);
        glBindVertexArray(vao[0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, numStepsFan + 2);

        // ----- DRAW TRIANGLE FAN - WINGS
        // Right wing
        glUseProgram(programIDWingRight);
        glBindVertexArray(vao[1]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, (numSteps - 1) * 2);

        // Left wing
        glUseProgram(programIDWingLeft);
        glBindVertexArray(vao[2]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, (numSteps - 1) * 2);


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
    //loadTexture(night, "night.png");
    loadTexture(night, "background.png");


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




    // ==== GOLDEN SNITCH RENDERING -----------------
    // ---- CIRCLE VIA TRIANGLE FAN -----------------

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    programIDCircle = OpenGP::load_shaders("vshader.glsl", "fshader.glsl");
    glUseProgram(programIDCircle);

    vao[0] = 0;
    glGenVertexArrays(1, &vao[0]);
    glBindVertexArray(vao[0]);
    glEnableVertexAttribArray(0);

    GLfloat* coordA = new GLfloat[(numStepsFan + 2) * 2];

    // Declare offset and radius
    float offsetX = 0.6f;
    float offsetY = 0.8f;
    float radius = 0.02f;

    // Origin
    unsigned coordIdx = 0;
    coordA[coordIdx++] = 0.0f;
    coordA[coordIdx++] = 0.0f;

    // Angle increments between each point
    float angInc = 2.0f * M_PI / static_cast<float>(numStepsFan);
    float cosInc = cos(angInc);
    float sinInc = sin(angInc);

    // Starting point
    coordA[coordIdx++] = radius;
    coordA[coordIdx++] = 0.0f;

    // Remaining points (determined via angle)
    float xc = radius;
    float yc = 0.0f;
    for (unsigned iDiv = 1; iDiv < numStepsFan; ++iDiv) {
        float xcNew = cosInc * xc - sinInc * yc;

        yc = sinInc * xc + cosInc * yc;
        xc = xcNew;

        coordA[coordIdx++] = xcNew;
        coordA[coordIdx++] = yc;
    }

    // End point
    coordA[coordIdx++] = radius;
    coordA[coordIdx++] = 0.0f;

    // Apply offset
    coordIdx = 0;
    for (unsigned iDiv = 1; iDiv <= numStepsFan + 2; ++iDiv) {
        coordA[coordIdx++] += offsetX;
        coordA[coordIdx++] += offsetY;
    }

    GLuint vboCircle = 0;
    glGenBuffers(1, &vboCircle);
    glBindBuffer(GL_ARRAY_BUFFER, vboCircle);
    glBufferData(GL_ARRAY_BUFFER, (numStepsFan + 2) * 2 * sizeof(GLfloat), coordA, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    delete[] coordA;



    // ----- RIGHT WING VIA TRIANGLE STRIP -----------------
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    programIDWingRight = OpenGP::load_shaders("vshader.glsl", "fshader.glsl");
    glUseProgram(programIDWingRight);

    vao[1] = 0;
    glGenVertexArrays(1, &vao[1]);
    glBindVertexArray(vao[1]);
    glEnableVertexAttribArray(0);

    // End and control points for Bezier curves
    Vec3 startPointR = Vec3(0.62f, 0.8f, 0.0f);
    Vec3 controlPointRAA = Vec3(0.635f, 0.87f, 0.0f);
    Vec3 controlPointRAB = Vec3(0.685f, 0.84f, 0.0f);
    Vec3 controlPointRBA = Vec3(0.635f, 0.82f, 0.0f);
    Vec3 controlPointRBB = Vec3(0.685f, 0.82f, 0.0f);
    Vec3 endPointR = Vec3(0.69f, 0.84f, 0.0f);

    Vec3 pointsRightWingTop[numSteps];
    Vec3 pointsRightWingBottom[numSteps];

    GLfloat* pointTotalRight = new GLfloat[(numSteps + 0) * 2 * 2];

    //Vec3 pointsTrianglesL[(numSteps - 1) * 2];

    float incrementR = 1.0f / numSteps;
    float tR = 0.0f;

    // Generate vertices
    for (int i = 0; i < numSteps; i++) {
        // Right wing
        pointsRightWingTop[i] = getPointBezier(startPointR, controlPointRAA, controlPointRAB, endPointR, tR);
        pointsRightWingBottom[i] = getPointBezier(startPointR, controlPointRBA, controlPointRBB, endPointR, tR);


        //std::cout << pointsRightWingTop[i](0) << " | " << pointsRightWingTop[i](1) << " \n ";
        //std::cout << pointsRightWingBottom[i](0) << " | " << pointsRightWingBottom[i](1) << " \n ";
        tR += incrementR;
    }

    //int indexVarVec = 0;
    //pointsTriangles[indexVarVec++] = startPoint;
    //pointsTriangles[13] = endPoint;
    int indexVarR = 0;

    // Add starting points for wings
    pointTotalRight[indexVarR++] = startPointR(0);
    pointTotalRight[indexVarR++] = startPointR(1);

    // Add vertices into array
    for (int i = 1; i < (numSteps - 1); i++) {
        // Right wing
        pointTotalRight[indexVarR++] = pointsRightWingTop[i](0);
        pointTotalRight[indexVarR++] = pointsRightWingTop[i](1);
        pointTotalRight[indexVarR++] = pointsRightWingBottom[i](0);
        pointTotalRight[indexVarR++] = pointsRightWingBottom[i](1);

        //pointsTriangles[indexVarVec++] = pointsRightWingTop[i];
        //pointsTriangles[indexVarVec++] = pointsRightWingBottom[i];
    }

    //pointsTriangles[indexVarVec++] = endPoint;

    // Add ending points for wings
    pointTotalRight[indexVarR++] = endPointR(0);
    pointTotalRight[indexVarR++] = endPointR(1);

    for (int i = 0; i < (numSteps - 1) * 2 * 2; i += 2) {
        std::cout << "Right Wing Coordinates: \n" << pointTotalRight[i] << " | " << pointTotalRight[i + 1] << " \n ";
    }

    GLuint vboWingRight = 0;
    glGenBuffers(1, &vboWingRight);
    glBindBuffer(GL_ARRAY_BUFFER, vboWingRight);
    glBufferData(GL_ARRAY_BUFFER, (numSteps - 1) * 2 * 2 * sizeof(GLfloat), pointTotalRight, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);


    // ----- LEFT WING VIA TRIANGLE STRIP -----------------
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    programIDWingLeft = OpenGP::load_shaders("vshader.glsl", "fshader.glsl");
    glUseProgram(programIDWingLeft);

    vao[2] = 0;
    glGenVertexArrays(1, &vao[2]);
    glBindVertexArray(vao[2]);
    glEnableVertexAttribArray(0);

    // End and control points for Bezier curves
    Vec3 startPointL = Vec3(0.58f, 0.8f, 0.0f);
    Vec3 controlPointLAA = Vec3(0.565f, 0.87f, 0.0f);
    Vec3 controlPointLAB = Vec3(0.515f, 0.84f, 0.0f);
    Vec3 controlPointLBA = Vec3(0.565f, 0.82f, 0.0f);
    Vec3 controlPointLBB = Vec3(0.515f, 0.82f, 0.0f);
    Vec3 endPointL = Vec3(0.51f, 0.84f, 0.0f);

    Vec3 pointsLeftWingTop[numSteps];
    Vec3 pointsLeftWingBottom[numSteps];

    GLfloat* pointTotalLeft = new GLfloat[(numSteps + 0) * 2 * 2];

    //Vec3 pointsTriangles[(numSteps - 1) * 2];

    float incrementL = 1.0f / numSteps;
    float tL = 0.0f;

    // Generate vertices for left wing
    for (int i = 0; i < numSteps; i++) {
        // Left wing
        pointsLeftWingTop[i] = getPointBezier(startPointL, controlPointLAA, controlPointLAB, endPointL, tL);
        pointsLeftWingBottom[i] = getPointBezier(startPointL, controlPointLBA, controlPointLBB, endPointL, tL);

        //std::cout << pointsRightWingTop[i](0) << " | " << pointsRightWingTop[i](1) << " \n ";
        //std::cout << pointsRightWingBottom[i](0) << " | " << pointsRightWingBottom[i](1) << " \n ";
        tL += incrementL;
    }

    //int indexVarVec = 0;
    //pointsTriangles[indexVarVec++] = startPoint;
    //pointsTriangles[13] = endPoint;
    int indexVarL = 0;

    // Add starting points for wings
    pointTotalLeft[indexVarL++] = startPointL(0);
    pointTotalLeft[indexVarL++] = startPointL(1);

    // Add points for left wing into array
    for (int i = 1; i < (numSteps - 1); i++) {
        pointTotalLeft[indexVarL++] = pointsLeftWingTop[i](0);
        pointTotalLeft[indexVarL++] = pointsLeftWingTop[i](1);
        pointTotalLeft[indexVarL++] = pointsLeftWingBottom[i](0);
        pointTotalLeft[indexVarL++] = pointsLeftWingBottom[i](1);

        //pointsTriangles[indexVarVec++] = pointsRightWingTop[i];
        //pointsTriangles[indexVarVec++] = pointsRightWingBottom[i];
    }

    //pointsTriangles[indexVarVec++] = endPoint;

    // Add ending points for wings
    pointTotalLeft[indexVarL++] = endPointL(0);
    pointTotalLeft[indexVarL++] = endPointL(1);

    for (int i = 0; i < (numSteps - 1) * 2 * 2; i += 2) {
        std::cout << "Left Wing Coordinates: \n" << pointTotalLeft[i] << " | " << pointTotalLeft[i + 1] << " \n ";
    }

    GLuint vboWingLeft = 0;
    glGenBuffers(1, &vboWingLeft);
    glBindBuffer(GL_ARRAY_BUFFER, vboWingLeft);
    glBufferData(GL_ARRAY_BUFFER, (numSteps - 1) * 2 * 2 * sizeof(GLfloat), pointTotalLeft, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
}

// Initialize quad (background)
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

// Load texture from file (background)
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

// Get Bezier point for a given curve
Vec3 getPointBezier(Vec3 startPoint, Vec3 controlPointA, Vec3 controlPointB, Vec3 endPoint, float t)
//Vec2 getPointBezier(Vec2 startPoint, Vec2controlPointA, Vec2 controlPointB, Vec2 endPoint, float t)
{
    float tempT = (1.0f - t) * (1.0f - t);

    Vec3 point = (tempT * (1.0f - t) * startPoint) + (3.0f * t * tempT * controlPointA) +
            (3.0f * t * t * (1.0f - t) * controlPointB) + (t * t * t * endPoint);

    //std::cout << point(0) << " | " << point(1) << " | " << point(2) << " \n ";

    return point;
}

// Draw the scene
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

    //quad->draw();
    cat->unbind();
    quadShader->unbind();








    glDisable(GL_BLEND);


}



