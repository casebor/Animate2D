#include <OpenGP/GL/Application.h>
#include <OpenGP/external/LodePNG/lodepng.cpp>
#include "math.h"
#include "Mesh.h"

using namespace OpenGP;

#define POINTSIZE 10.0f

// Shader constants
static const char* line_vshader =
#include "line_vshader.glsl"
;
static const char* line_fshader =
#include "line_fshader.glsl"
;
static const char* fb_vshader =
#include "fb_vshader.glsl"
;
static const char* fb_fshader =
#include "fb_fshader.glsl"
;
static const char* quad_vshader =
#include "quad_vshader.glsl"
;
static const char* quad_fshader =
#include "quad_fshader.glsl"
;

// Function stubs
void init();
void quadInit(std::unique_ptr<GPUMesh>& quad);
void loadTexture(std::unique_ptr<RGBA8Texture>& texture, const char* filename);
Vec3 getPointBezier(Vec3, Vec3, Vec3, Vec3, float);
void drawBackground();

// Initialize Shaders
static std::unique_ptr<GPUMesh> quad;
static std::unique_ptr<Shader> quadShader;
static std::unique_ptr<Shader> fbShader;

// Scene objects
static std::unique_ptr<RGBA8Texture> background;
static Mesh snitch;

// Framebuffers and Textures
static std::unique_ptr<Framebuffer> fb;
static std::unique_ptr<RGBA8Texture> c_buf;

// Control variables
static bool flag;
const int width = 720, height = 720;
typedef Eigen::Transform<float, 3, Eigen::Affine> Transform;

// Bezier path variables
static OpenGP::Vec3 startPointPath;
static OpenGP::Vec3 controlPointPathA;
static OpenGP::Vec3 controlPointPathB;
static OpenGP::Vec3 endPointPath;

static int numSteps = 20;
static Vec2 position = Vec2(0,0);
static Vec2 *selection = nullptr;

// Mouse control variables
static std::unique_ptr<Shader> lineShader;
static std::unique_ptr<Shader> curveShader;
static std::unique_ptr<GPUMesh> line;
static std::unique_ptr<GPUMesh> curve;
static std::vector<Vec2> controlPoints;
static std::vector<Vec2> bezierPoints;


int main(int, char**) {

	Application app;
	init();

    // Initialize framebuffer
	fb = std::unique_ptr<Framebuffer>(new Framebuffer());

    // Initialize color buffer texture, and allocate memory
	c_buf = std::unique_ptr<RGBA8Texture>(new RGBA8Texture());
    c_buf->allocate(width*2, height*2);

    // Attach color texture to framebuffer
	fb->attach_color_texture(*c_buf);

    // Time offset
    float timeOffset = static_cast<float>(glfwGetTime());
    int timeAdj = 0.0;

	Window& window = app.create_window([&](Window&) {
        glViewport(0, 0, width*2, height*2);

        // ----- Draw background
        // Bind framebuffer and draw background
		fb->bind();
		glClear(GL_COLOR_BUFFER_BIT);
        drawBackground();
		fb->unbind();

        // Render window and bind shader
		glClear(GL_COLOR_BUFFER_BIT);
		fbShader->bind();

        // Bind texture background
		glActiveTexture(GL_TEXTURE0);
		c_buf->bind();
		fbShader->set_uniform("tex", 0);
        fbShader->set_uniform("tex_width", float(width));
        fbShader->set_uniform("tex_height", float(height));
        quad->set_attributes(*fbShader);
        quad->draw();

		c_buf->unbind();
		fbShader->unbind();

        // ----- Draw control points
        glPointSize(POINTSIZE);
        lineShader->bind();

        // Draw line red
        lineShader->set_uniform("selection", -1);
        line->set_attributes(*lineShader);
        line->set_mode(GL_LINE_STRIP);
        line->draw();

        // Draw points red and selected point blue
        if(selection!=nullptr) lineShader->set_uniform("selection", int(selection-&controlPoints[0]));
        line->set_mode(GL_POINTS);
        line->draw();

        lineShader->unbind();

        // ----- Draw bezier curve
        curveShader->bind();

        // Draw line red
        curveShader->set_uniform("selection", -1);
        curve->set_attributes(*curveShader);
        curve->set_mode(GL_LINE_STRIP);
        curve->draw();

        curveShader->unbind();

        // ----- Setup transformation hierarchies for Circle
        glViewport(0, 0, 2*width, 2*height);

        float time_s;
        if (flag) {
            time_s = static_cast<float>(glfwGetTime()) - timeOffset;
            startPointPath = OpenGP::Vec3(controlPoints[0](0), controlPoints[0](1), 0.0f);
            controlPointPathA = OpenGP::Vec3(controlPoints[1](0), controlPoints[1](1), 0.0f);
            controlPointPathB = OpenGP::Vec3(controlPoints[2](0), controlPoints[2](1), 0.0f);
            endPointPath = OpenGP::Vec3(controlPoints[3](0), controlPoints[3](1), 0.0f);
        } else {
            timeOffset = static_cast<float>(glfwGetTime());
            time_s = 0.0f;
            flag = true;
        }

        float timeInterval = 3.0f;

        // Add looping ability
        if (time_s > timeInterval) {
            timeAdj = static_cast<int>(floor(time_s / timeInterval));
        }

        float tPathPosition = (time_s - (timeAdj * timeInterval)) / timeInterval;

        // Calculate Bezier path
        OpenGP::Vec3 tempPosition;
        GLfloat pointX;
        GLfloat pointY;

        // Current path position
        tempPosition = getPointBezier(startPointPath, controlPointPathA, controlPointPathB, endPointPath, tPathPosition);
        pointX = tempPosition(0);
        pointY = tempPosition(1);

        // Initialize transformation matrix
        Transform snitch_m = Transform::Identity();

        // Transform and scale
        snitch_m *= Eigen::Translation3f(pointX, pointY, 0.0);
        snitch_m *= Eigen::AlignedScaling3f(tPathPosition, tPathPosition, 1.0);

        // ----- Setup transformation hierarchies for Wings
        // Draw snitch body
        snitch.draw(snitch_m.matrix(), 2);

        // Add rotating factor befone rendering wings
        Transform snitch_mRightWing = snitch_m;
        Transform snitch_mLeftWing = snitch_m;

        float oscillationSpeed = 0.005f;
        int orbitalFactor = 40;
        int currRotation = static_cast<int>(floor(time_s / oscillationSpeed)) % orbitalFactor;

        // Smooth rotation of wings
        if (static_cast<int>((floor(time_s / oscillationSpeed))) % orbitalFactor > (orbitalFactor / 2)) {
            snitch_mLeftWing *= Eigen::AngleAxisf(static_cast<float>((orbitalFactor - currRotation)) /
                                                  static_cast<float>(orbitalFactor), Eigen::Vector3f::UnitZ());
            snitch_mRightWing *= Eigen::AngleAxisf(-static_cast<float>((orbitalFactor - currRotation)) /
                                                   static_cast<float>(orbitalFactor), Eigen::Vector3f::UnitZ());
        } else {
            snitch_mLeftWing *= Eigen::AngleAxisf(static_cast<float>(currRotation) / static_cast<float>(orbitalFactor), Eigen::Vector3f::UnitZ());
            snitch_mRightWing *= Eigen::AngleAxisf(-static_cast<float>(currRotation) / static_cast<float>(orbitalFactor), Eigen::Vector3f::UnitZ());
        }

        snitch.draw(snitch_mRightWing.matrix(), 0);
        snitch.draw(snitch_mLeftWing.matrix(), 1);
	});

    window.set_title("Golden Snitch Animation");
	window.set_size(width, height);

    // Mouse movement callback
    window.add_listener<MouseMoveEvent>([&](const MouseMoveEvent &m){
        // Mouse position in clip coordinates
        Vec2 p = 2.0f*(Vec2(m.position.x()/width,-m.position.y()/height) - Vec2(0.5f,-0.5f));
        if( selection && (p-position).norm() > 0.0f) {
            // Make selected control points move with cursor
            //position = 2.0f*(Vec2(m.position.x()/width,-m.position.y()/height) - p);
            selection->x() = 2.0f * (m.position.x()/width - 0.5f);
            selection->y() = 2.0f * (-m.position.y()/height + 0.5f);
            line->set_vbo<Vec2>("vposition", controlPoints);

            bezierPoints.clear();
            for (int i = 0; i < numSteps; i++) {
                Vec3 tempA = Vec3(controlPoints[0](0), controlPoints[0](1), 0.0f);
                Vec3 tempB = Vec3(controlPoints[1](0), controlPoints[1](1), 0.0f);
                Vec3 tempC = Vec3(controlPoints[2](0), controlPoints[2](1), 0.0f);
                Vec3 tempD = Vec3(controlPoints[3](0), controlPoints[3](1), 0.0f);
                Vec3 temp3V = getPointBezier(tempA, tempB, tempC, tempD, (static_cast<float>(i) / static_cast<float>(numSteps)));
                Vec2 temp2V = Vec2(temp3V(0), temp3V(1));
                bezierPoints.push_back(temp2V);
            }
            curve->set_vbo<Vec2>("vposition", bezierPoints);
        }
        position = p;
    });

    // Mouse click callback
    window.add_listener<MouseButtonEvent>([&](const MouseButtonEvent &e){
        // Mouse selection case
        if( e.button == GLFW_MOUSE_BUTTON_LEFT && !e.released) {
            selection = nullptr;
            for(auto&& v : controlPoints) {
                if ( (v-position).norm() < POINTSIZE/std::min(width,height) ) {
                    selection = &v;
                    break;
                }
            }
        }

        // Mouse release case
        if( e.button == GLFW_MOUSE_BUTTON_LEFT && e.released) {
            if(selection) {
                selection->x() = position.x();
                selection->y() = position.y();
                selection = nullptr;
                line->set_vbo<Vec2>("vposition", controlPoints);

                bezierPoints.clear();
                for (int i = 0; i < numSteps; i++) {
                    Vec3 tempA = Vec3(controlPoints[0](0), controlPoints[0](1), 0.0f);
                    Vec3 tempB = Vec3(controlPoints[1](0), controlPoints[1](1), 0.0f);
                    Vec3 tempC = Vec3(controlPoints[2](0), controlPoints[2](1), 0.0f);
                    Vec3 tempD = Vec3(controlPoints[3](0), controlPoints[3](1), 0.0f);
                    Vec3 temp3V = getPointBezier(tempA, tempB, tempC, tempD, (static_cast<float>(i) / static_cast<float>(numSteps)));
                    Vec2 temp2V = Vec2(temp3V(0), temp3V(1));
                    bezierPoints.push_back(temp2V);
                }
                curve->set_vbo<Vec2>("vposition", bezierPoints);
            }
        }
    });

	return app.run();
}

// Initialize scene
void init() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // ----- LOAD SHADERS
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

    loadTexture(background, "background.png");

    // ----- SNITCH
    snitch.init();
    snitch.loadVertices();

    // ----- BACKGROUND
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

    std::vector<OpenGP::Vec2> quadTCoord;
    quadTCoord.push_back(OpenGP::Vec2(0.0f, 0.0f));
    quadTCoord.push_back(OpenGP::Vec2(1.0f, 0.0f));
    quadTCoord.push_back(OpenGP::Vec2(1.0f, 1.0f));
    quadTCoord.push_back(OpenGP::Vec2(0.0f, 1.0f));

    // ----- BEZIER PATH CONTROL POINTS
    lineShader = std::unique_ptr<Shader>(new Shader());
    lineShader->verbose = true;
    lineShader->add_vshader_from_source(line_vshader);
    lineShader->add_fshader_from_source(line_fshader);
    lineShader->link();

    controlPoints.push_back(Vec2(0.9f, 0.5f));
    controlPoints.push_back(Vec2(0.8f, -0.1f));
    controlPoints.push_back(Vec2(0.6f, -0.2f));
    controlPoints.push_back(Vec2(-0.8f, 0.98f));

    line = std::unique_ptr<GPUMesh>(new GPUMesh());
    line->set_vbo<Vec2>("vposition", controlPoints);
    std::vector<unsigned int> indices = {0,1,2,3};
    line->set_triangles(indices);

    // ----- BEZIER PATH POINTS
    // Generate vertices for bezier path
    curveShader = std::unique_ptr<Shader>(new Shader());
    curveShader->verbose = true;
    curveShader->add_vshader_from_source(line_vshader);
    curveShader->add_fshader_from_source(line_fshader);
    curveShader->link();
    for (int i = 0; i < numSteps; i++) {
        Vec3 tempA = Vec3(controlPoints[0](0), controlPoints[0](1), 0.0f);
        Vec3 tempB = Vec3(controlPoints[1](0), controlPoints[1](1), 0.0f);
        Vec3 tempC = Vec3(controlPoints[2](0), controlPoints[2](1), 0.0f);
        Vec3 tempD = Vec3(controlPoints[3](0), controlPoints[3](1), 0.0f);
        Vec3 temp3V = getPointBezier(tempA, tempB, tempC, tempD, (static_cast<float>(i) / static_cast<float>(numSteps)));
        Vec2 temp2V = Vec2(temp3V(0), temp3V(1));
        bezierPoints.push_back(temp2V);
    }

    curve = std::unique_ptr<GPUMesh>(new GPUMesh());
    curve->set_vbo<Vec2>("vposition", bezierPoints);
    std::vector<unsigned int> indicesCurve;
    for (int i = 0; i < numSteps; i++) {
        indicesCurve.push_back(i);
    }
    curve->set_triangles(indicesCurve);
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
    delete[] row;

	texture = std::unique_ptr<RGBA8Texture>(new RGBA8Texture());
	texture->upload_raw(width, height, &image[0]);
}

// Get Bezier point for a given curve
Vec3 getPointBezier(Vec3 startPoint, Vec3 controlPointA, Vec3 controlPointB, Vec3 endPoint, float t)
{
    float tempT = (1.0f - t) * (1.0f - t);

    Vec3 point = (tempT * (1.0f - t) * startPoint) + (3.0f * t * tempT * controlPointA) +
            (3.0f * t * t * (1.0f - t) * controlPointB) + (t * t * t * endPoint);

    return point;
}

// Draw the scene
void drawBackground()
{


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Transform TRS = Transform::Identity();
	quadShader->bind();
	quadShader->set_uniform("M", TRS.matrix());

    // Activate texture and bind
    glActiveTexture(GL_TEXTURE0);
    background->bind();

    // Set attributes and draw
	quadShader->set_uniform("tex", 0);
	quad->set_attributes(*quadShader);
    quad->draw();

    background->unbind();
    quadShader->unbind();

    glDisable(GL_BLEND);
}
