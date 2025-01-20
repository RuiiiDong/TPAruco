
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ArUco-OpenGL.h"
#include <windows.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2\calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <GLUT.h>
#include <GL/GLU.h>
#include <cmath>
#include <vector>
#include <string>
#include <map>


#define PI  3.14159265358979323846
using namespace std;

float angle = 0.0f; // L'angle de rotation
Marker sunMarker;
Point2f sunPos = { 0.0, 0.0 };
struct planet {
    float speed;
    float radius;
    string name;
    string textureFile;
    GLuint textureID;
};

bool isPosOk = false;

map<int, planet> planets = { {141, {2.0f, 0.2f, "Earth","textures/earth.jpg",1}},
    {217, {0.0f, 0.0f, "Sun","textures/sun.jpg",2}},
    {144,{1.0f, 0.4f, "Jupiter","textures/jupiter.jpg",3}} };

// Constructor
ArUco::ArUco(string intrinFileName, float markerSize) {
    // Initializing attributes
    m_IntrinsicFile = intrinFileName;
    m_MarkerSize = markerSize;
    // read camera parameters if passed
    m_CameraParams.readFromXMLFile(intrinFileName);

}

// Destructor
ArUco::~ArUco() {}

// Texture Loading Function
void loadTexture(const char* filename, GLuint& textureID) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb); // Load image

    if (!image) {
        cerr << "Failed to load texture: " << filename << std::endl;
        return;
    }

    glGenTextures(1, &textureID);  // Generate a texture ID
    glBindTexture(GL_TEXTURE_2D, textureID);  // Bind the texture for use

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Upload the texture to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    stbi_image_free(image);  // Free the image data after uploading to OpenGL
}

void drawTexturedSphere(float radius, int slices, int stacks) {
    glEnable(GL_TEXTURE_2D);
    GLUquadric* quad = gluNewQuadric();  // Create a new quadric object
    gluQuadricTexture(quad, GL_TRUE);  // Enable texture mapping
    gluSphere(quad, radius, slices, stacks);  // Draw the sphere
    gluDeleteQuadric(quad);  // Delete the quadric object when done
    glDisable(GL_TEXTURE_2D);
}

void ArUco::resizeCameraParams(cv::Size newSize) {
    m_CameraParams.resize(newSize);
}

// Detect marker and draw things
void ArUco::doWork(Mat inputImg) {
    m_InputImage = inputImg;
    m_GlWindowSize = m_InputImage.size();
    m_CameraParams.resize(m_InputImage.size());
    resize(m_GlWindowSize.width, m_GlWindowSize.height);
}

// Draw axis function
void ArUco::drawAxis(float axisSize) {
    // X
    glColor3f(1, 0, 0);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(axisSize, 0.0f, 0.0f); // ending point of the line
    glEnd();

    // Y
    glColor3f(0, 1, 0);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(0.0f, axisSize, 0.0f); // ending point of the line
    glEnd();

    // Z
    glColor3f(0, 0, 1);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(0.0f, 0.0f, axisSize); // ending point of the line
    glEnd();
}

void ArUco::drawWireCone(GLdouble base, GLdouble height, GLint slices, GLint stacks) {
    GLint i, j;

    // Draw the cone's side
    for (i = 0; i < slices; i++) {
        // Draw a segment of the cone using lines
        glBegin(GL_LINE_LOOP);

        for (j = 0; j < stacks; j++) {
            // Calculate the angle for each point
            float angle = (2.0f * PI * i) / slices;
            float x = base * (1.0f - (float(j) / stacks)) * cos(angle);
            float y = base * (1.0f - (float(j) / stacks)) * sin(angle);
            float z = (height / stacks) * float(j);
            glVertex3f(x, y, z);  // Draw the points for the side
        }

        glEnd();
    }

    // Draw the base of the cone (a circle)
    glBegin(GL_LINE_LOOP);
    for (i = 0; i < slices; i++) {
        float angle = (2.0f * 3.141592 * i) / slices;
        float x = base * cos(angle);
        float y = base * sin(angle);
        glVertex3f(x, y, 0.0f);  // Points on the base circle
    }
    glEnd();
}

void drawSolidSphere(GLdouble radius, GLint slices, GLint stacks) {
    for (GLint i = 0; i < slices; i++) {
        GLfloat lat0 = 3.141592 * (-0.5 + (GLfloat)(i) / slices);
        GLfloat lat1 = 3.141592 * (-0.5 + (GLfloat)(i + 1) / slices);

        glBegin(GL_QUAD_STRIP);
        for (GLint j = 0; j <= stacks; j++) {
            GLfloat lon = 2 * PI * (GLfloat)(j) / stacks;
            GLfloat x0 = radius * cos(lat0) * cos(lon);
            GLfloat y0 = radius * cos(lat0) * sin(lon);
            GLfloat z0 = radius * sin(lat0);

            GLfloat x1 = radius * cos(lat1) * cos(lon);
            GLfloat y1 = radius * cos(lat1) * sin(lon);
            GLfloat z1 = radius * sin(lat1);

            glVertex3f(x0, y0, z0); // Vertex at latitude lat0
            glVertex3f(x1, y1, z1); // Vertex at latitude lat1
        }
        glEnd();
    }
}

// Fonction qui dessine un cube de différentes manières (type)
void ArUco::drawBox(GLfloat size, GLenum type)
{
    static const GLfloat n[6][3] =
    {
      {-1.0, 0.0, 0.0},
      {0.0, 1.0, 0.0},
      {1.0, 0.0, 0.0},
      {0.0, -1.0, 0.0},
      {0.0, 0.0, 1.0},
      {0.0, 0.0, -1.0}
    };
    static const GLint faces[6][4] =
    {
      {0, 1, 2, 3},
      {3, 2, 6, 7},
      {7, 6, 5, 4},
      {4, 5, 1, 0},
      {5, 6, 2, 1},
      {7, 4, 0, 3}
    };
    GLfloat v[8][3];
    GLint i;

    v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
    v[4][0] = v[5][0] = v[6][0] = v[7][0] = size / 2;
    v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
    v[2][1] = v[3][1] = v[6][1] = v[7][1] = size / 2;
    v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
    v[1][2] = v[2][2] = v[5][2] = v[6][2] = size / 2;

    for (i = 5; i >= 0; i--) {
        glBegin(type);
        glNormal3fv(&n[i][0]);
        glVertex3fv(&v[faces[i][0]][0]);
        glVertex3fv(&v[faces[i][1]][0]);
        glVertex3fv(&v[faces[i][2]][0]);
        glVertex3fv(&v[faces[i][3]][0]);
        glEnd();
    }
}

void drawPlanet(double modelview_matrix[16], Marker m_Marker, float m_MarkerSize, bool& hasSun, bool isPosOk) {
    planet p = planets[m_Marker.id];

    if (hasSun && p.name != "Sun" && isPosOk) {
        m_Marker = sunMarker;
    }

    m_Marker.glGetModelViewMatrix(modelview_matrix);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // on charge cette matrice pour se placer dans le repere de ce marqueur [m] 
    glLoadMatrixd(modelview_matrix);

    loadTexture(p.textureFile.c_str(), p.textureID);//string ->const char*
    glBindTexture(GL_TEXTURE_2D, p.textureID);

    // On se deplace sur Z de la moitie du marqueur pour dessiner "sur" le plan du marqueur
    glPushMatrix();

    glTranslatef(0, 0, m_MarkerSize / 2);
    if (isPosOk && hasSun) {
        if (p.speed != 0) {
            angle += p.speed;
            if (angle >= 360.0f) {
                angle -= 360.0f;
            }
        }
        glRotatef(angle, 0.0f, 0.0f, 1.0f);  // Rotate around Y-axis
        glTranslatef(p.radius, 0.0f, 0.0f);  // Move the sphere along the X-axis by the radius
    }

    //drawSolidSphere(m_MarkerSize / 2, 20, 20);
    drawTexturedSphere(m_MarkerSize / 2, 20, 20);

    glPopMatrix();
}

// Drawing function
void ArUco::drawScene() {
    if (m_ResizedImage.rows == 0)
        return;

    // On "reset" les matrices OpenGL de ModelView et de Projection
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // On deinit une vue orthographique de la taille de l'image OpenGL
    glOrtho(0, m_GlWindowSize.width, 0, m_GlWindowSize.height, -1.0, 1.0);
    // on definit le viewport correspond a un rendu "plein ecran"
    glViewport(0, 0, m_GlWindowSize.width, m_GlWindowSize.height);

    // on desactive les textures
    glDisable(GL_TEXTURE_2D);

    // On "flippe" l'axe des Y car OpenCV et OpenGL on un axe Y inverse pour les images/textures
    glPixelZoom(1, -1);

    // On definit la position ou l'on va ecrire dans l'image
    glRasterPos3f(0, m_GlWindowSize.height, -1.0f);

    // On "dessine" les pixels contenus dans l'image OpenCV m_ResizedImage (donc l'image de la Webcam qui nous sert de fond)
    glDrawPixels(m_GlWindowSize.width, m_GlWindowSize.height, GL_RGB, GL_UNSIGNED_BYTE, m_ResizedImage.ptr(0));

    // On active ensuite le depth test pour les objets 3D
    glEnable(GL_DEPTH_TEST);

    // On passe en mode projection pour definir la bonne projection calculee par ArUco
    glMatrixMode(GL_PROJECTION);
    double proj_matrix[16];
    m_CameraParams.glGetProjectionMatrix(m_ResizedImage.size(), m_GlWindowSize, proj_matrix, 0.01, 100);
    glLoadIdentity();
    // on charge la matrice d'ArUco 
    glLoadMatrixd(proj_matrix);

    // On affiche le nombre de marqueurs (ne sert a rien)
    double modelview_matrix[16];
    std::cout << "Number of markers: " << m_Markers.size() << std::endl;

    // On desactive le depth test
    glDisable(GL_DEPTH_TEST);

    bool hasSun = false;

    // Check if we have a marker with the name "Sun"
    for (unsigned int m = 0; m < m_Markers.size(); m++)
    {
        if (planets[m_Markers[m].id].name == "Sun") {
            hasSun = true;
            sunMarker = m_Markers[m];
            sunPos = m_Markers[m].getCenter();
            break;
        }
    }

    for (unsigned int m = 0; m < m_Markers.size(); m++)
    {
        cout << "Checking marker ID: " << m_Markers[m].id << endl;
        for (Marker marker : m_Markers) {
            if (marker != m_Markers[m] && planets[marker.id].name != "Sun") {
                if ((planets[m_Markers[m].id].radius > planets[marker.id].radius && norm(marker.getCenter() - sunPos) > norm(m_Markers[m].getCenter() - sunPos))
                    || (planets[m_Markers[m].id].radius < planets[marker.id].radius && norm(marker.getCenter() - sunPos) < norm(m_Markers[m].getCenter() - sunPos)))
                {
                    isPosOk = false;

                }
                else {
                    isPosOk = true;
                }
            }
        }
        drawPlanet(modelview_matrix, m_Markers[m], m_MarkerSize, hasSun, isPosOk);
    }

    // Desactivation du depth test
    glDisable(GL_DEPTH_TEST);
}



// Idle function
void ArUco::idle(Mat newImage) {
    // Getting new image
    m_InputImage = newImage.clone();

    // Undistort image based on distorsion parameters
    m_UndInputImage.create(m_InputImage.size(), CV_8UC3);

    //transform color that by default is BGR to RGB because windows systems do not allow reading BGR images with opengl properly
    cv::cvtColor(m_InputImage, m_InputImage, cv::COLOR_BGR2RGB);

    //remove distorion in image ==> does not work very well (the YML file is not that of my camera)
    //cv::undistort(m_InputImage,m_UndInputImage, m_CameraParams.CameraMatrix, m_CameraParams.Distorsion);
    m_UndInputImage = m_InputImage.clone();

    //resize the image to the size of the GL window
    cv::resize(m_UndInputImage, m_ResizedImage, m_GlWindowSize);

    //detect markers
    m_PPDetector.detect(m_ResizedImage, m_Markers, m_CameraParams, m_MarkerSize, false);

}

// Resize function
void ArUco::resize(GLsizei iWidth, GLsizei iHeight) {
    m_GlWindowSize = Size(iWidth, iHeight);

    //not all sizes are allowed. OpenCv images have padding at the end of each line in these that are not aligned to 4 bytes
    if (iWidth * 3 % 4 != 0) {
        iWidth += iWidth * 3 % 4;//resize to avoid padding
        resize(iWidth, m_GlWindowSize.height);
    }
    else {
        //resize the image to the size of the GL window
        if (m_UndInputImage.rows != 0)
            cv::resize(m_UndInputImage, m_ResizedImage, m_GlWindowSize);
    }

}

// Test using ArUco to display a 3D cube in OpenCV
void ArUco::draw3DCube(cv::Mat img, int markerInd) {
    if (m_Markers.size() > markerInd) {
        aruco::CvDrawingUtils::draw3dCube(img, m_Markers[markerInd], m_CameraParams);
    }
}

void ArUco::draw3DAxis(cv::Mat img, int markerInd) {
    if (m_Markers.size() > markerInd) {
        aruco::CvDrawingUtils::draw3dAxis(img, m_Markers[markerInd], m_CameraParams);
    }

}