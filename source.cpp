#include <GL/glut.h>
#include <cmath>
#include <cstdlib>  
#include <ctime>    
#include <iostream>
#include <string>
#include <array>
#include <sstream>
#include <algorithm>
#include <iomanip>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Face 
{
    int indices[4];
    int normal;
    int offsetIndex;
    int plane;
    bool isTouching;
    int count;
    float distance; 
};

GLfloat vertices[8][3] =
{
    {-1.0f, -1.0f, -1.0f}, //0
    { 1.0f, -1.0f, -1.0f}, //1
    { 1.0f,  1.0f, -1.0f}, //2
    {-1.0f,  1.0f, -1.0f}, //3
    {-1.0f, -1.0f,  1.0f}, //4
    { 1.0f, -1.0f,  1.0f}, //5
    { 1.0f,  1.0f,  1.0f}, //6
    {-1.0f,  1.0f,  1.0f}  //7
};

struct Level
{
    GLfloat ballColor[3];
    GLfloat bgColor[3];
    std::string texturePath;
};

Level levels[6] =
{
    {{0.728f, 0.84313f, 0.568f}, {0.844f, 0.904f, 0.86f}, "emerald.png"},
    {{0.572f, 0.788f, 0.95f}, {0.88f, 0.88f, 0.88f}, "diamonds.png"},
    {{0.8f, 0.556f, 0.152f}, {1.0f, 1.0f, 0.788f}, "lucky.png"},
    {{0.96f, 0.96f, 0.96f}, {0.96f, 0.956f, 0.98f}, "purple.png"},
    {{0.82f, 0.82f, 0.82f}, {0.8f, 0.717f, 0.686f}, "bricks.png"},
    {{0.45f, 0.36f, 0.2235f}, {0.78f, 0.78f, 0.78f}, "planks.png"},
};

GLuint textures[6];

GLfloat normals[6][3] =
{
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {-1.0f, 0.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {0.0f, 0.0f, -1.0f}
};

Face faces[6] = {
    {{0, 1, 2, 3}, 5, 2, -1, false, 0, 0}, // Back face
    {{5, 1, 2, 6}, 0, 0, 1, false, 0, 0}, // Right face
    {{0, 4, 7, 3}, 3, 0, -1, false, 0, 0}, // Left face
    {{2, 3, 7, 6}, 1, 1, 1, false, 0, 0}, // Top face
    {{0, 4, 5, 1}, 4, 1, -1, false, 0, 0},  // Bottom face
    {{4, 5, 6, 7}, 2, 2, 1, false, 0, 0}, // Front face
};

GLfloat texCoords[4][2] = {
    {0.f, 0.f},
    {1.f, 0.f},
    {1.f, 1.f},
    {0.f, 1.f}
};

bool compare_faces(const Face& a, const Face& b) 
{
    return a.distance > b.distance; 
}

const float M_PI = acos(-1.0f);
const float GRAVITY = 0.05f;
const float ENERGY_DRAIN = 0.8f;
const float JUMP_VELOCITY = 1;
const float BALL_RADIUS = 0.1f;

float ballVelocity[3] = {0.0f, 0.0f, 0.0f};
float ballPos[3] = {0.0f, 0.0f, 0.0f};
float gravityVec[3] = { 0.0,-GRAVITY, 0.0 };

float aX = 0.0;
float aY = 0.0;
int level = 0;

bool isDragging = false;
int lastX = 0, lastY = 0;
float zoom = 0;

GLfloat cubeLightPos[] = { 3.0f, 3.0f, 1.0f, 1.0f }; 
GLfloat cubeLightDiff[] = { 1.0f, 1.0f, 1.0f, 1.0f }; 
GLfloat cubeLightSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f }; 
GLfloat cubeLightAmb[] = { 0.4f, 0.2f, 0.2f, 1.0f }; 

float calc_dist(Face& face)
{
    float x = (vertices[face.indices[0]][0] + vertices[face.indices[1]][0] +
        vertices[face.indices[2]][0] + vertices[face.indices[3]][0]) / 4;
    float y = (vertices[face.indices[0]][1] + vertices[face.indices[1]][1] +
        vertices[face.indices[2]][1] + vertices[face.indices[3]][1]) / 4;
    float z = (vertices[face.indices[0]][2] + vertices[face.indices[1]][2] +
        vertices[face.indices[2]][2] + vertices[face.indices[3]][2]) / 4;

    float nX = cos(aY) * x + sin(aY) * z;
    float nY = sin(aX) * sin(aY) * x + cos(aX) * y - sin(aX) * cos(aY) * z;
    float nZ = -cos(aX) * sin(aY) * x + sin(aX) * y + cos(aX) * cos(aY) * z + zoom;

    return -(pow(nX,2) + pow(nY,2) + pow(nZ, 2));
}

void render_string(float x, float y, const char* string)
{
    
    glRasterPos3f(x, y, 0.15f);
    for (const char *c = string; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
}


void sort_faces()
{
    for (int i = 0; i < 6; ++i)
        faces[i].distance = calc_dist(faces[i]);
    std::sort(faces, faces + 6, compare_faces);
}

void start_round(int i)
{
    for (int i = 0; i < 6; i++)
    {
        faces[i].count = 0;
        faces[i].isTouching = false;
    }

    sort_faces();
    glBindTexture(GL_TEXTURE_2D, textures[i]);
}

void load_textures()
{
    glGenTextures(6, textures);

    for (int i = 0; i < 6; i++)
    {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        int w, h, ncs;
        stbi_set_flip_vertically_on_load(1);
        unsigned char* data = stbi_load(levels[i].texturePath.c_str(), &w, &h, &ncs, STBI_rgb_alpha);
        if (data != NULL)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
    }
}

void handle_mouse_move(int x, int y) 
{
    if (isDragging) 
    {   
        aX += ((y - lastY) * M_PI) / 180.f;
        aY += ((x - lastX) * M_PI) / 180.f;

        lastX = x;
        lastY = y;

        sort_faces();

        gravityVec[0] = sin(aY) * sin(aX) * -GRAVITY;
        gravityVec[1] = cos(aX) * -GRAVITY;
        gravityVec[2] = cos(aY) * sin(aX) * GRAVITY;

        glutPostRedisplay(); 
    }
}

void handle_keyboard(unsigned char key, int x, int y)
{
    if (key == ' ')
    {
        float len = sqrt(gravityVec[0] * gravityVec[0] + gravityVec[1] * gravityVec[1] + gravityVec[2] * gravityVec[2]);
        ballVelocity[0] = JUMP_VELOCITY * -gravityVec[0]/len;
        ballVelocity[1] = JUMP_VELOCITY * -gravityVec[1]/len;
        ballVelocity[2] = JUMP_VELOCITY * -gravityVec[2]/len;
    }
}

void handle_mouse_button(int button, int state, int x, int y) 
{
    if (button == GLUT_LEFT_BUTTON) 
    {
        if (state == GLUT_DOWN) 
        {
            isDragging = true;
            lastX = x;
            lastY = y;
        }
        else 
        {
            isDragging = false;
        }
    }
    if (button == 3)
    {
        zoom += 0.5;
        if (zoom > 10) zoom = 10; 
        glutPostRedisplay(); 
    }
    if (button == 4)
    {
        zoom -= 0.5;
        if (zoom < 5) zoom = 5; 
        glutPostRedisplay();
    }
}


void draw()
{
    glEnable(GL_LIGHTING);
    for (int i = 0; i < 6; ++i) 
    {
        if (faces[i].isTouching)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
        else
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        glBegin(GL_QUADS);
        for (int j = 0; j < 4; ++j)
        {
            glNormal3fv(normals[faces[i].normal]);
            glColor4f(0.15f, 0.15f, 0.15f, faces[i].count / 100.f);
            glTexCoord2fv(texCoords[j]);
            glVertex3fv(vertices[faces[i].indices[j]]);
        }
        glEnd();
    }

    glPushMatrix();
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTranslatef(ballPos[0], ballPos[1], ballPos[2]);
        glColor3fv(levels[level].ballColor);
        glutSolidSphere(BALL_RADIUS, 20, 20);
    glPopMatrix();

    glDisable(GL_LIGHTING);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(levels[level].bgColor[0], levels[level].bgColor[1], levels[level].bgColor[2], 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(0, 0, zoom,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0);

    glRotatef(aX*(180.f/M_PI), 1.0, 0.0, 0.0);
    glRotatef(aY*(180.f/M_PI), 0.0, 1.0, 0.0);
    draw();

    glLoadIdentity();
    gluLookAt(0, 0, 1.75f,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0);

    float totalSum = 0;
    for (int i = 0; i < 6; i++)
        totalSum += faces[i].count * (1.f/6.f);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << totalSum;

    glColor3fv(levels[level].ballColor);
    render_string(-0.85f, 0.85f, ("Level " + std::to_string(level+1) + ": " + ss.str() + " %").c_str());
    render_string(-0.85, -0.75, "Spacebar: jump");
    render_string(-0.85, -0.8, "Mouse wheel: zoom");
    render_string(-0.85, -0.85, "Mouse: rotate");
    glutSwapBuffers();
}


void update(int value) 
{
    ballVelocity[0] += gravityVec[0];
    ballVelocity[1] += gravityVec[1];
    ballVelocity[2] += gravityVec[2];

    ballPos[0] += ballVelocity[0];
    ballPos[1] += ballVelocity[1];
    ballPos[2] += ballVelocity[2];

    for (int i = 0; i < 6; ++i)
    {
        if (faces[i].plane*ballPos[faces[i].offsetIndex] + BALL_RADIUS > 1.0f)
        {
            ballPos[faces[i].offsetIndex] = faces[i].plane * (1.f - BALL_RADIUS);
            ballVelocity[faces[i].offsetIndex] = -ballVelocity[faces[i].offsetIndex] * pow(ENERGY_DRAIN,level+1);

            if (!faces[i].isTouching)
            {
                if (++faces[i].count >= 100)
                    faces[i].count = 100;

                faces[i].isTouching = true;
            }
        }
        else faces[i].isTouching = false;
    }

    bool levelFinished = true;
    for (int i = 0; i < 6; i++)
    {
        if (faces[i].count != 100)
        {
            levelFinished = false;
            break;
        }
    }

    if (levelFinished && level < 5) 
    {
        start_round(++level);
    }
        

    glutPostRedisplay(); 
    glutTimerFunc(16, update, 0); 
}

void init()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, cubeLightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, cubeLightDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, cubeLightSpec);
    glLightfv(GL_LIGHT0, GL_AMBIENT, cubeLightAmb);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    load_textures();
    start_round(level);
    glDisable(GL_LIGHTING);
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glShadeModel(GL_SMOOTH);
    glLoadIdentity();
    gluPerspective(60, (GLfloat)w / (GLfloat)h, 1, 100.0f);
}

int main(int argc, char** argv) 
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Прыгающий мяч");

    init();

    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(handle_keyboard);
    glutMotionFunc(handle_mouse_move);
    glutMouseFunc(handle_mouse_button);

    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}