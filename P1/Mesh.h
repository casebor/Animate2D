#pragma once
#include <cassert>

#include "OpenGP/Image/Image.h"
#include "OpenGP/GL/Eigen.h"
#include "OpenGP/GL/check_error_gl.h"
#include "OpenGP/GL/shader_helpers.h"
#include "OpenGP/SurfaceMesh/SurfaceMesh.h"

class Mesh{
protected:
    GLuint _vao[3];
    GLuint _pid[3];
    GLuint _texture;
    GLuint _vpoint;
    GLuint _vnormal;
    GLuint _tcoord;
    GLuint vboCircle;
    GLuint vboWingRight;
    GLuint vboWingLeft;

    unsigned int numSteps = 360;
    unsigned numStepsFan = 360;

    bool hasNormals;
    bool hasTextures;
    bool hasTexCoords;

public:

    GLuint getProgramID(){ return _pid[1]; }

    void init() {
        check_error_gl();
        hasNormals = false;
        hasTextures = false;
        hasTexCoords = false;
    }

    void loadVertices() {
        // ==== GOLDEN SNITCH RENDERING -----------------
        // ---- CIRCLE VIA TRIANGLE FAN -----------------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        _pid[2] = OpenGP::load_shaders("Snitch_vshader.glsl", "Snitch_body_fshader.glsl");
        glUseProgram(_pid[2]);

        _vao[2] = 0;
        glGenVertexArrays(1, &_vao[2]);
        glBindVertexArray(_vao[2]);
        glEnableVertexAttribArray(0);

        GLfloat* circlePoints = new GLfloat[(numStepsFan + 2) * 2];

        // Declare offset and radius
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float radius = 0.04f;

        // Origin
        unsigned circlePointIndex = 0;
        circlePoints[circlePointIndex++] = 0.0f;
        circlePoints[circlePointIndex++] = 0.0f;

        // Angle increments between each point
        float angleIncrement = 2.0f * static_cast<float>(M_PI) / static_cast<float>(numStepsFan);
        float cosIncrement = cos(angleIncrement);
        float sinIncrement = sin(angleIncrement);

        // Starting point
        circlePoints[circlePointIndex++] = radius;
        circlePoints[circlePointIndex++] = 0.0f;

        // Remaining points (determined via angle)
        float xc = radius;
        float yc = 0.0f;
        for (unsigned iDiv = 1; iDiv < numStepsFan; ++iDiv) {
            float xcNew = cosIncrement * xc - sinIncrement * yc;

            yc = sinIncrement * xc + cosIncrement * yc;
            xc = xcNew;

            circlePoints[circlePointIndex++] = xcNew;
            circlePoints[circlePointIndex++] = yc;
        }

        // End point
        circlePoints[circlePointIndex++] = radius;
        circlePoints[circlePointIndex++] = 0.0f;

        // Apply offset
        circlePointIndex = 0;
        for (unsigned iDiv = 1; iDiv <= numStepsFan + 2; ++iDiv) {
            circlePoints[circlePointIndex++] += offsetX;
            circlePoints[circlePointIndex++] += offsetY;
        }

        vboCircle = 0;
        glGenBuffers(1, &vboCircle);
        glBindBuffer(GL_ARRAY_BUFFER, vboCircle);
        glBufferData(GL_ARRAY_BUFFER, (numStepsFan + 2) * 2 * sizeof(GLfloat), circlePoints, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        delete[] circlePoints;

        // ----- RIGHT WING VIA TRIANGLE STRIP -----------------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        _pid[0] = OpenGP::load_shaders("Snitch_vshader.glsl", "Snitch_wing_fshader.glsl");
        glUseProgram(_pid[0]);

        _vao[0] = 0;
        glGenVertexArrays(1, &_vao[0]);
        glBindVertexArray(_vao[0]);
        glEnableVertexAttribArray(0);

        OpenGP::Vec3 startPointR = OpenGP::Vec3(0.04f, 0.0f, 0.0f);
        OpenGP::Vec3 controlPointRAA = OpenGP::Vec3(0.07f, 0.14f, 0.0f);
        OpenGP::Vec3 controlPointRAB = OpenGP::Vec3(0.17f, 0.08f, 0.0f);
        OpenGP::Vec3 controlPointRBA = OpenGP::Vec3(0.07f, 0.04f, 0.0f);
        OpenGP::Vec3 controlPointRBB = OpenGP::Vec3(0.17f, 0.04f, 0.0f);
        OpenGP::Vec3 endPointR = OpenGP::Vec3(0.18f, 0.08f, 0.0f);

        OpenGP::Vec3 *pointsRightWingTop = new OpenGP::Vec3[numSteps];
        OpenGP::Vec3 *pointsRightWingBottom = new OpenGP::Vec3[numSteps];

        GLfloat* pointTotalRight = new GLfloat[(numSteps + 0) * 2 * 2];

        OpenGP::Vec3* pointsTrianglesR = new OpenGP::Vec3[(numSteps - 1) * 2];

        float incrementR = 1.0f / numSteps;
        float tR = 0.0f;

        // Generate vertices
        for (int i = 0; i < numSteps; i++) {
            // Right wing
            pointsRightWingTop[i] = getPointBezier(startPointR, controlPointRAA, controlPointRAB, endPointR, tR);
            pointsRightWingBottom[i] = getPointBezier(startPointR, controlPointRBA, controlPointRBB, endPointR, tR);
            tR += incrementR;
        }

        int indexVarVec = 0;
        pointsTrianglesR[indexVarVec++] = startPointR;
        pointsTrianglesR[numSteps - 1] = endPointR;
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

            pointsTrianglesR[indexVarVec++] = pointsRightWingTop[i];
            pointsTrianglesR[indexVarVec++] = pointsRightWingBottom[i];
        }

        pointsTrianglesR[indexVarVec++] = endPointR;

        // Add ending points for wings
        pointTotalRight[indexVarR++] = endPointR(0);
        pointTotalRight[indexVarR++] = endPointR(1);

        for (int i = 0; i < (numSteps - 1) * 2 * 2; i += 2) {
            //std::cout << "Right Wing Coordinates: \n" << pointTotalRight[i] << " | " << pointTotalRight[i + 1] << " \n ";
        }

        vboWingRight = 0;
        glGenBuffers(1, &vboWingRight);
        glBindBuffer(GL_ARRAY_BUFFER, vboWingRight);
        glBufferData(GL_ARRAY_BUFFER, (numSteps - 1) * 2 * 2 * sizeof(GLfloat), pointTotalRight, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);


        // ----- LEFT WING VIA TRIANGLE STRIP -----------------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        _pid[1] = OpenGP::load_shaders("Snitch_vshader.glsl", "Snitch_wing_fshader.glsl");
        glUseProgram(_pid[1]);

        _vao[1] = 0;
        glGenVertexArrays(1, &_vao[1]);
        glBindVertexArray(_vao[1]);
        glEnableVertexAttribArray(0);

        OpenGP::Vec3 startPointL = OpenGP::Vec3(-0.04f, 0.0f, 0.0f);
        OpenGP::Vec3 controlPointLAA = OpenGP::Vec3(-0.07f, 0.14f, 0.0f);
        OpenGP::Vec3 controlPointLAB = OpenGP::Vec3(-0.17f, 0.08f, 0.0f);
        OpenGP::Vec3 controlPointLBA = OpenGP::Vec3(-0.07f, 0.04f, 0.0f);
        OpenGP::Vec3 controlPointLBB = OpenGP::Vec3(-0.17f, 0.04f, 0.0f);
        OpenGP::Vec3 endPointL = OpenGP::Vec3(-0.18f, 0.08f, 0.0f);



        OpenGP::Vec3 *pointsLeftWingTop = new OpenGP::Vec3[numSteps];
        OpenGP::Vec3 *pointsLeftWingBottom = new OpenGP::Vec3[numSteps];

        GLfloat* pointTotalLeft = new GLfloat[(numSteps + 0) * 2 * 2];

        //Vec3 pointsTriangles[(numSteps - 1) * 2];

        float incrementL = 1.0f / numSteps;
        float tL = 0.0f;

        // Generate vertices for left wing
        for (int i = 0; i < numSteps; i++) {
            // Left wing
            pointsLeftWingTop[i] = getPointBezier(startPointL, controlPointLAA, controlPointLAB, endPointL, tL);
            pointsLeftWingBottom[i] = getPointBezier(startPointL, controlPointLBA, controlPointLBB, endPointL, tL);
            tL += incrementL;
        }
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
        }

        // Add ending points for wings
        pointTotalLeft[indexVarL++] = endPointL(0);
        pointTotalLeft[indexVarL++] = endPointL(1);

        vboWingRight = 0;
        glGenBuffers(1, &vboWingRight);
        glBindBuffer(GL_ARRAY_BUFFER, vboWingRight);
        glBufferData(GL_ARRAY_BUFFER, (numSteps - 1) * 2 * 2 * sizeof(GLfloat), pointTotalRight, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        vboWingLeft = 0;
        glGenBuffers(1, &vboWingLeft);
        glBindBuffer(GL_ARRAY_BUFFER, vboWingLeft);
        glBufferData(GL_ARRAY_BUFFER, (numSteps - 1) * 2 * 2 * sizeof(GLfloat), pointTotalLeft, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
    }

    void loadNormals(const std::vector<OpenGP::Vec3> &normalArray) {
        glBindVertexArray(_vao[0]);
        check_error_gl();

        glGenBuffers(1, &_vnormal);
        glBindBuffer(GL_ARRAY_BUFFER, _vnormal);
        glBufferData(GL_ARRAY_BUFFER, normalArray.size() * sizeof(OpenGP::Vec3), &normalArray[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3 /*vec3*/, GL_FLOAT, GL_TRUE /*NORMALIZE*/, 3*sizeof(float) /*STRIDE*/, (void*)0 /*ZERO_BUFFER_OFFSET*/);
        check_error_gl();

        hasNormals = true;
        glBindVertexArray(0);
    }

    void loadTexCoords(const std::vector<OpenGP::Vec2> &tCoordArray) {
        glBindVertexArray(_vao[0]);
        check_error_gl();

        glGenBuffers(1, &_tcoord);
        glBindBuffer(GL_ARRAY_BUFFER, _tcoord);
        glBufferData(GL_ARRAY_BUFFER, tCoordArray.size() * sizeof(OpenGP::Vec3), &tCoordArray[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2 /*vec2*/, GL_FLOAT, GL_FALSE /*DONT_NORMALIZE*/, 2*sizeof(float) /*STRIDE*/, (void*)0 /*ZERO_BUFFER_OFFSET*/);
        check_error_gl();

        hasTexCoords = true;
        glBindVertexArray(0);
    }

    void loadTextures(const std::string filename) {

        // Used snippet from https://raw.githubusercontent.com/lvandeve/lodepng/master/examples/example_decode.cpp
        std::vector<unsigned char> image; //the raw pixels
        unsigned width, height;

        //decode
        unsigned error = lodepng::decode(image, width, height, filename);
        //if there's an error, display it
        if(error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
        //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...

        // unfortunately they are upside down...lets fix that
        unsigned char* row = new unsigned char[4*width];
        for(int i = 0; i < int(height)/2; ++i) {
            memcpy(row, &image[4*i*width], 4*width*sizeof(unsigned char));
            memcpy(&image[4*i*width], &image[image.size() - 4*(i+1)*width], 4*width*sizeof(unsigned char));
            memcpy(&image[image.size() - 4*(i+1)*width], row, 4*width*sizeof(unsigned char));
        }
        delete[] row;

        glBindVertexArray(_vao[0]);
        check_error_gl();

        glUseProgram(_pid[0]);

        GLuint tex_id = glGetUniformLocation(_pid[0], "texImage");
        check_error_gl();
        glUniform1i(tex_id, 0);
        check_error_gl();

        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);

        check_error_gl();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        check_error_gl();

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
        check_error_gl();

        hasTextures = true;
        glUseProgram(0);
        glBindVertexArray(0);
    }

    // Get Bezier point for a given curve
    OpenGP::Vec3 getPointBezier(OpenGP::Vec3 startPoint, OpenGP::Vec3 controlPointA, OpenGP::Vec3 controlPointB, OpenGP::Vec3 endPoint, float t)
    //Vec2 getPointBezier(Vec2 startPoint, Vec2controlPointA, Vec2 controlPointB, Vec2 endPoint, float t)
    {
        float tempT = (1.0f - t) * (1.0f - t);

        OpenGP::Vec3 point = (tempT * (1.0f - t) * startPoint) + (3.0f * t * tempT * controlPointA) +
                (3.0f * t * t * (1.0f - t) * controlPointB) + (t * t * t * endPoint);

        //std::cout << point(0) << " | " << point(1) << " | " << point(2) << " \n ";

        return point;
    }

    void draw(OpenGP::Mat4x4 Model, OpenGP::Mat4x4 View, OpenGP::Mat4x4 Projection, int vaoIndex){

        // Use specified shaders and VAO
        glUseProgram(_pid[vaoIndex]);
        glBindVertexArray(_vao[vaoIndex]);
        check_error_gl();

        // Check whether to use normals when shading
        if(hasNormals) {
            glUniform1i(glGetUniformLocation(_pid[vaoIndex], "hasNormals"), 1);
        } else {
            glUniform1i(glGetUniformLocation(_pid[vaoIndex], "hasNormals"), 0);
        }

        // Check whether to use textures when shading
        if(hasTextures && hasTexCoords) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _texture);
            glUniform1i(glGetUniformLocation(_pid[vaoIndex], "hasTextures"), 1);
        } else {
            glUniform1i(glGetUniformLocation(_pid[vaoIndex], "hasTextures"), 0);
        }

        // Configure Model, View and Projection
        glUniformMatrix4fv(glGetUniformLocation(_pid[vaoIndex], "MODEL"), 1, GL_FALSE, Model.data());
        glUniformMatrix4fv(glGetUniformLocation(_pid[vaoIndex], "VIEW"), 1, GL_FALSE, View.data());
        glUniformMatrix4fv(glGetUniformLocation(_pid[vaoIndex], "PROJ"), 1, GL_FALSE, Projection.data());

        check_error_gl();

        // Draw item
        if (vaoIndex == 0 || vaoIndex == 1) {
            glDrawArrays(GL_TRIANGLE_STRIP, 0, (numSteps - 1) * 2);
        } else {
            glDrawArrays(GL_TRIANGLE_FAN, 0, numStepsFan + 2);
        }

        check_error_gl();
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void draw(OpenGP::Mat4x4 Model, int vaoIndex) {
        OpenGP::Mat4x4 id = OpenGP::Mat4x4::Identity();
        draw(Model, id, id, vaoIndex);
    }
};
