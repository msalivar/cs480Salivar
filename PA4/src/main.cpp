
#define GLM_FORCE_RADIANS

#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <GL/freeglut.h>
#include <iostream>
#include <chrono>
#include <cctype>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier
#include "shader.h"
#include <iostream>
#include <fstream>

using namespace std;

//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat color[3];
};

//--Evil Global variables
//Just for this example!
string objPath = "";
int numFaces = 0;
int w = 640, h = 480;// Window size
GLuint program;// The GLSL program handle
GLuint vbo_geometry;// VBO handle for our cube

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_color;

//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection

//shader loader object
Shader sloader;

// Model Loader Functions
int countLines(string objFile, const char header);
Vertex* loadVertices(int numVertices, string objFile);
int loadTriangles(Vertex* vertices, Vertex* &geometry, int numVertices, int numFaces, string objFile);
Vertex loadMaterial(const string mat_name, string objFile);

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);
void contextMenu(int id);

//--Resource management
bool initialize();
void cleanUp();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

//--Main
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <.obj file name>" << endl;
        return 0;
    }
    else
    {
        objPath = argv[1];
        ifstream fin(objPath);
        if (!fin.is_open())
        {
            cout << "Could not find .obj file." << endl;
            return 0;
        }
    }

    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);
    // Name and create the Window
    glutCreateWindow("Matrix Example");

    // Now that the window is created the GL context is fully set up
    // Because of that we can now initialize GLEW to prepare work with shaders
    GLenum status = glewInit();
    if( status != GLEW_OK)
    {
        std::cerr << "[F] GLEW NOT INITIALIZED: ";
        std::cerr << glewGetErrorString(status) << std::endl;
        return -1;
    }

    // Set all of the callbacks to GLUT that we need
    glutDisplayFunc(render);// Called when its time to display
    glutReshapeFunc(reshape);// Called if the window is resized
    glutIdleFunc(update);// Called if there is nothing else to do
    glutKeyboardFunc(keyboard);// Called if there is keyboard input

    // Menu Setup
    glutCreateMenu(contextMenu);
    glutAddMenuEntry("Quit", 1);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();
    if(init)
    {
        t1 = std::chrono::high_resolution_clock::now();
        glutMainLoop();
    }

    // Clean up after ourselves
    cleanUp();
    return 0;
}

//--Implementations
void render()
{
    //--Render the scene

    //clear the screen
    glClearColor(0.0, 0.0, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //premultiply the matrix for this example
    mvp = projection * view * model;

    //enable the shader program
    glUseProgram(program);

    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));

    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    //set pointers into the vbo for each of the attributes(position and color)
    glVertexAttribPointer( loc_position,//location of attribute
                           3,//number of elements
                           GL_FLOAT,//type
                           GL_FALSE,//normalized?
                           sizeof(Vertex),//stride
                           0);//offset

    glVertexAttribPointer( loc_color,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,color));

    glDrawArrays(GL_TRIANGLES , 0, numFaces * 3);//mode, starting index, count

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_color);
                           
    //swap the buffers
    glutSwapBuffers();
}

void update()
{
    //total time
    static float angle = 0.0;
    float dt = getDT(); // if you have anything moving, use dt.
    angle += dt * M_PI/2; //move through 90 degrees a second
    model = glm::translate( glm::mat4(1.0f), glm::vec3(4.0 * sin(angle), 0.0, 0));

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}


void reshape(int n_w, int n_h)
{
    w = n_w;
    h = n_h;
    //Change the viewport to be correct
    glViewport( 0, 0, w, h);
    //Update the projection matrix as well
    //See the init function for an explaination
    projection = glm::perspective(45.0f, float(w)/float(h), 0.01f, 100.0f);
}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
    // Handle keyboard input
    switch(key)
    {
        case 27:
            glutLeaveMainLoop();
            break;
    }
}

bool initialize()
{
    Vertex* vertexIndices = 0;
    int numVertices = countLines(objPath, 'v');
    numFaces = countLines(objPath, 'f');
    vertexIndices = loadVertices(numVertices, objPath);
    Vertex* geometry = 0;
    loadTriangles(vertexIndices, geometry, numVertices, numFaces, objPath);

    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * (numFaces * 3), geometry, GL_STATIC_DRAW);

    //--Geometry done

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    GLchar* vs = 0;
    GLchar* fs = 0;
    std::string vs_location("vertex_shader.txt");
    std::string fs_location("fragment_shader.txt");
    sloader.loadFromFile(vs_location, vs);
    sloader.loadFromFile(fs_location, fs);

    //compile the shaders
    GLint shader_status;

    // Vertex shader first
    glShaderSource(vertex_shader, 1, (const GLchar**)&vs, NULL);
    glCompileShader(vertex_shader);
    //check the compile status
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE VERTEX SHADER!" << std::endl;
        return false;
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, (const GLchar**)&fs, NULL);
    glCompileShader(fragment_shader);
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE FRAGMENT SHADER!" << std::endl;
        return false;
    }

    //Now we link the 2 shader objects into a program
    //This program is what is run on the GPU
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    //check if everything linked ok
    glGetProgramiv(program, GL_LINK_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] THE SHADER PROGRAM FAILED TO LINK" << std::endl;
        return false;
    }

    //Now we set the locations of the attributes and uniforms
    //this allows us to access them easily while rendering
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_color = glGetAttribLocation(program,
                    const_cast<const char*>("v_color"));
    if(loc_color == -1)
    {
        std::cerr << "[F] V_COLOR NOT FOUND" << std::endl;
        return false;
    }

    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    }
    
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0.0, 6.0, -15.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 

    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //and its done
    delete [] vertexIndices;
    vertexIndices = 0;
    delete [] geometry;
    geometry = 0;
    return true;
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo_geometry);
}

//returns the time delta
float getDT()
{
    float ret;
    t2 = std::chrono::high_resolution_clock::now();
    ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-t1).count();
    t1 = std::chrono::high_resolution_clock::now();
    return ret;
}

void contextMenu(int id)
{
    switch(id)
    {
        case 1:
            glutLeaveMainLoop();
            break;
    }
    glutPostRedisplay();
}

int countLines(string objFile, const char header)
{
    ifstream fin;
    fin.open(objFile);
    int count = 0;
    if (fin.is_open())
    {
        while (fin.eof() == false)
        {
            char buffer[80];
            fin.getline(buffer, 80);
            if (buffer[0] == header && buffer[1] == ' ')
            {
                count++;
            }
        }
        fin.close();
        return count;
    }
    fin.close();
    return -1;
}

Vertex* loadVertices(int numVertices, string objFile)
{
    Vertex* vertexIndices = new Vertex[numVertices];
    ifstream fin;
    fin.open(objFile);
    int numAdded = 0;
    if (fin.is_open())
    {
        while (fin.eof() == false)
        {
            string input = "";
            fin >> input;
            if (input == "v")
            {
                float x;
                fin >> x;
                float y;
                fin >> y;
                float z;
                fin >> z;
                Vertex temp = {{x, y, z}, {1.0, 1.0, 1.0}};
                vertexIndices[numAdded] = temp;
                numAdded++;
            }
        }
    }
    fin.close();
    return vertexIndices;
}

int loadTriangles(Vertex* vertices, Vertex* &geometry, int numVertices, int numFaces, string objFile)
{
    geometry = new Vertex[numFaces * 3];
    float currentColor[3] = {1, 1, 1};
    ifstream fin;
    fin.open(objFile);
    int numAdded = 0;
    if (fin.is_open())
    {
        while (numAdded < (numFaces * 3) && fin.eof() == false)
        {
            string input = "";
            fin >> input;
            if (input == "usemtl")
            {
                fin >> input;
                Vertex temp = loadMaterial(input, objPath);
                currentColor[0] = temp.color[0];
                currentColor[1] = temp.color[1];
                currentColor[2] = temp.color[2];
            }
            else if (input == "f")
            {
                for (int count = 0; count < 3; count++)
                {
                    string vertex;
                    fin >> vertex;
                    int index = vertex[0] - '0';
                    if (isdigit(vertex[1]))
                    {
                        index *= 10;
                        index += vertex[1] - '0';
                    }
                    float x = vertices[index - 1].position[0];
                    float y = vertices[index - 1].position[1];
                    float z = vertices[index - 1].position[2];
                    Vertex temp = {{x, y, z}, {currentColor[0], currentColor[1], currentColor[2]}};
                    geometry[numAdded] = temp;
                    numAdded++;
                }
            }
        }
    }
    fin.close();
    return numAdded;
}

Vertex loadMaterial(const string mat_name, string objFile)
{
    Vertex temp = {{0, 0, 0}, {1, 1, 1}};
    string path = "";
    ifstream reader;
    reader.open(objFile);
    if (reader.is_open())
    {
        while (reader.eof() == false)
        {
            string input = "";
            reader >> input;
            if (input == "mtllib")
            {
                reader >> path;
                break;
            }
        }
        reader.close();
    }
    reader.open(path);
    if(reader.is_open())
    {
        while (reader.eof() == false)
        {
            string input = "";
            reader >> input;
            if (input == mat_name)
            {
                while (input != "Kd")
                {
                    reader >> input;
                }
                float r, g, b;
                reader >> r;
                reader >> g;
                reader >> b;
                temp = {{0, 0, 0}, {r, g, b}};
            }
        }
        reader.close();
    }
    return temp;
}