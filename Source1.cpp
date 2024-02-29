#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "camera.h"
#include <iostream>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Eggs"; // Macro for window title

    // offset to use to encompass vertex position and color; change this value if using 
    // textures, etc.
    const int STRIDE = 7;

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices; 
        GLfloat nVertices;  // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    //triangle mesh data for lamp
    GLMesh gMesh;
    // egg mesh data
    GLMesh gYolkMesh;
    GLMesh gWhiteMesh;
    // plate data
    GLMesh gPlateMesh;
    // Texture
    GLuint gTextureYolk;
    GLuint gTextureWhite;
    glm::vec2 gUVScale(5.0f, 5.0f);
    GLint gTexWrapMode = GL_REPEAT;
    
    // Shader program
    GLuint gProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    bool ortho = false;
    GLfloat fov = 45.0f;

    // Light color, position and scale
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f); // White
    glm::vec3 gLightPosition(1.0f, 1.0f, 3.0f);
    glm::vec3 gLightScale(0.3f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreatePrismMesh(GLfloat verts[], GLushort indices[], int numSides, float radius, float halfLen);
void UCreateCylinderMesh(GLMesh& mesh);
void UCreatePlateMesh(GLMesh& mesh);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code */
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
layout(location = 1) in vec3 normal; // Normal data from Vertex Attrib Pointer 1
layout(location = 2) in vec2 textureCoordinate; // Texture data from Vertex Attrib Pointer 2

out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec2 vertexTextureCoordinate; // For outgoing texture coordinate

// Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices to clip coordinates
    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
    vertexNormal = mat3(transpose(inverse(model))) * normal; // Gets normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate; // Gets texture coordinate
}
);

/* Fragment Shader Source Code */
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexFragmentPos; // For incoming fragment position
in vec3 vertexNormal; // For incoming normals
in vec2 vertexTextureCoordinate; // For incoming texture coordinate

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /* Phong lighting model calculations to generate ambient, diffuse, and specular components */

    // LAMP 1: Calculate ambient lighting
    float ambientStrength = 0.1f; // Set ambient or global lighting strength 10%
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

 
    // LAMP 1: Calculate diffuse lighting
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color


    // LAMP 1: Calculate specular lighting
    float specularIntensity = 0.1f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector

    
    // LAMP 1: Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;


    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    // Send lighting results to GPU
    fragmentColor = vec4(phong, 1.0);
}
);

/* Lamp Shader Source Code */
const GLchar* lampVertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0

// Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Lamp Fragment Shader Source Code */
const GLchar* lampFragmentShaderSource = GLSL(440,
    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateCylinderMesh(gYolkMesh);
    UCreateCylinderMesh(gWhiteMesh);
    UCreatePlateMesh(gPlateMesh);
    UCreateMesh(gMesh);

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load textures
    const char* texFilename = "../OpenGLSample/resources/textures/yolk.png";
    if (!UCreateTexture(texFilename, gTextureYolk))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    const char* texFilename2 = "../OpenGLSample/resources/textures/white.jpg";
    if (!UCreateTexture(texFilename2, gTextureWhite))
    {
        cout << "Failed to load texture " << texFilename2 << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gYolkMesh);
    UDestroyMesh(gWhiteMesh);
    UDestroyMesh(gPlateMesh);
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(gTextureYolk);
    UDestroyTexture(gTextureWhite);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // glad: load all OpenGL function pointers

    // ---------------------------------------

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && gTexWrapMode != GL_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureYolk);
        glBindTexture(GL_TEXTURE_2D, gTextureWhite);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_REPEAT;

        cout << "Current Texture Wrapping Mode: REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && gTexWrapMode != GL_MIRRORED_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureYolk);
        glBindTexture(GL_TEXTURE_2D, gTextureWhite);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_MIRRORED_REPEAT;

        cout << "Current Texture Wrapping Mode: MIRRORED REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_EDGE)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureYolk);
        glBindTexture(GL_TEXTURE_2D, gTextureWhite);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_EDGE;

        cout << "Current Texture Wrapping Mode: CLAMP TO EDGE" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_BORDER)
    {
        float color[] = { 1.0f, 0.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

        glBindTexture(GL_TEXTURE_2D, gTextureYolk);
        glBindTexture(GL_TEXTURE_2D, gTextureWhite);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_BORDER;

        cout << "Current Texture Wrapping Mode: CLAMP TO BORDER" << endl;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
    {
        gUVScale += 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
    {
        gUVScale -= 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }
}



// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS) {
            cout << "Left mouse button pressed" << endl;
            gCamera.ResetCamera();
        }
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Functioned called to render a frame
void URender()
{
    const int nrows = 10;
    const int ncols = 10;
    const int nlevels = 10;

    const float xsize = 10.0f;
    const float ysize = 10.0f;
    const float zsize = 10.0f;

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(120.0f, glm::vec3(1.0, 1.0f, 1.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // Transforms the camera: move the camera back (z axis)
    glm::mat4 view = glm::translate(glm::vec3(0.0f, 0.0f, -5.0f));

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    

    // Pass color, light, and camera data to the Shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    
    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));


    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gYolkMesh.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureWhite);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gYolkMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle
   
    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gWhiteMesh.vao);

    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.2f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureYolk);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gWhiteMesh.nIndices, GL_UNSIGNED_SHORT, NULL);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gPlateMesh.vao);

    scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES,0, gPlateMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

    // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);



    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    GLfloat verts[] = {
        // Vertex Positions		// Normals			// Texture coords	// Index
        0.0f, 0.5f, 0.0f,		0.0f, 0.0f, 0.0f,	0.5f, 1.0f,			//0 Apex
        -0.5f, -0.5f, -0.5f,	0.0f, 0.0f, 0.0f,	0.0f, 0.0f,			//1 Back Left
        0.5f, -0.5f, -0.5f,		0.0f, 0.0f, 0.0f,	1.0f, 0.0f,			//2 Back Right
        0.5f,  -0.5f, 0.5f,		0.0f, 0.0f, 0.0f,	1.0f, 1.0f,			//3 Front Right
        -0.5f,  -0.5f, 0.5f,	0.0f, 0.0f, 0.0f,	0.0f, 1.0f			//4 Front Left
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 2,  // Front Face
        0, 1, 3,  // Right Face
        0, 3, 4,  // Back Face
        0, 4, 2,  // Left Face
        1, 3, 2,  // Bottom Front Right 
        4, 2, 3   // Bottom Back Left 
    };

    // total float values per each type
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    // store vertex and index count
    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nIndices = sizeof(indices) / (sizeof(indices[0]));

    // Create VAO
    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);  //activates the VAO

    // Create VBOs
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the vertex buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]); // Activates the index buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(floatsPerVertex + floatsPerNormal));
    glEnableVertexAttribArray(2);
}

// creates a prism based on the number of side turned in; assumes a stride of seven
void UCreatePrismMesh(GLfloat verts[], GLushort indices[], int numSides, float radius, float halfLen)
{
    // create constant for 2PI used in calculations
    const float TWO_PI = 2.0f * 3.1415926f;
    const float radiansPerSide = TWO_PI / numSides;

    // value to increment after each vertex is created
    int currentVertex = 0;

    // in this  algorithm, vertex zero is the top center vertex, and vertex one is the bottom center
    // each is offset by the step size
    verts[0] = 0.0f;    // 0 x
    verts[1] = halfLen; // 0 y
    verts[2] = 0.0f;    // 0 z
    verts[3] = 1.0f;    // 0 r
    verts[4] = 0.0f;    // 0 g
    verts[5] = 0.0f;    // 0 b
    verts[6] = 1.0f;    // 0 a
    currentVertex++;

    verts[7] = 0.0f;    // 1 x
    verts[8] = -halfLen;// 1 y
    verts[9] = 0.0f;    // 1 z
    verts[10] = 1.0f;   // 1 r
    verts[11] = 0.0f;   // 1 g
    verts[12] = 0.0f;   // 1 b
    verts[13] = 1.0f;   // 1 a
    currentVertex++;

    // variable to keep track of every triangle added to indices
    int currentTriangle = 0;

    // note: the number of flat sides is equal to the number of edge on the sides
    for (int edge = 0; edge < numSides; edge++)
    {
        // calculate theta, which is the angle from the center point to the next vertex
        float theta = ((float)edge) * radiansPerSide;

        // top triangle first perimeter vertex
        verts[currentVertex * STRIDE + 0] = radius * cos(theta);    // x
        verts[currentVertex * STRIDE + 1] = halfLen;                // y
        verts[currentVertex * STRIDE + 2] = radius * sin(theta);    // z
        verts[currentVertex * STRIDE + 3] = 0.0f;                   // r
        verts[currentVertex * STRIDE + 4] = 1.0f;                   // g
        verts[currentVertex * STRIDE + 5] = 0.0f;                   // b
        verts[currentVertex * STRIDE + 6] = 1.0f;                   // a
        currentVertex++;

        // bottom triangle first perimeter vertex
        verts[currentVertex * STRIDE + 0] = radius * cos(theta);    // x
        verts[currentVertex * STRIDE + 1] = -halfLen;               // y
        verts[currentVertex * STRIDE + 2] = radius * sin(theta);    // z
        verts[currentVertex * STRIDE + 3] = 0.0f;                   // r
        verts[currentVertex * STRIDE + 4] = 0.0f;                   // g
        verts[currentVertex * STRIDE + 5] = 1.0f;                   // b
        verts[currentVertex * STRIDE + 6] = 1.0f;                   // a
        currentVertex++;

        if (edge > 0)
        {
            // now to create the indices for the triangles
            // top triangle
            indices[(3 * currentTriangle) + 0] = 0;                 // center of top of prism
            indices[(3 * currentTriangle) + 1] = currentVertex - 4; // upper left vertex of side
            indices[(3 * currentTriangle) + 2] = currentVertex - 2; // upper right vertex of side
            currentTriangle++;

            // bottom triangle
            indices[(3 * currentTriangle) + 0] = 1;                 // center of bottom of prism
            indices[(3 * currentTriangle) + 1] = currentVertex - 3; // bottom left vertex of side
            indices[(3 * currentTriangle) + 2] = currentVertex - 1; // bottom right vertex of side
            currentTriangle++;

            // triangle for 1/2 retangular side
            indices[(3 * currentTriangle) + 0] = currentVertex - 4; // upper left vertex of side
            indices[(3 * currentTriangle) + 1] = currentVertex - 3; // bottom left vertex of side
            indices[(3 * currentTriangle) + 2] = currentVertex - 1; // bottom right vertex of side
            currentTriangle++;

            // triangle for second 1/2 retangular side
            indices[(3 * currentTriangle) + 0] = currentVertex - 1; // bottom right vertex of side
            indices[(3 * currentTriangle) + 1] = currentVertex - 2; // upper right vertex of side
            indices[(3 * currentTriangle) + 2] = currentVertex - 4; // upper left vertex of side
            currentTriangle++;

        }

    }

    // now, just need to wire up the last side
    // now to create the indices for the triangles
    // top triangle
    indices[(3 * currentTriangle) + 0] = 0;                 // center of top of prism
    indices[(3 * currentTriangle) + 1] = currentVertex - 2; // upper left vertex of side
    indices[(3 * currentTriangle) + 2] = 2;                 // first upper left vertex created, now right
    currentTriangle++;

    // bottom triangle
    indices[(3 * currentTriangle) + 0] = 1;                 // center of bottom of prism
    indices[(3 * currentTriangle) + 1] = currentVertex - 1; // bottom left vertex of side
    indices[(3 * currentTriangle) + 2] = 3;                 // first bottom left vertex created, now right
    currentTriangle++;

    // triangle for 1/2 retangular side
    indices[(3 * currentTriangle) + 0] = currentVertex - 2; // upper left vertex of side
    indices[(3 * currentTriangle) + 1] = currentVertex - 1; // bottom left vertex of side
    indices[(3 * currentTriangle) + 2] = 3;                 // bottom right vertex of side
    currentTriangle++;

    // triangle for second 1/2 retangular side
    indices[(3 * currentTriangle) + 0] = 3;                 // bottom right vertex of side
    indices[(3 * currentTriangle) + 1] = 2;                 // upper right vertex of side
    indices[(3 * currentTriangle) + 2] = currentVertex - 2; // upper left vertex of side
    currentTriangle++;

}

// Implements the UCreateMesh function
void UCreateCylinderMesh(GLMesh& mesh)
{
    // number of sides for the prism we will create
    const int NUM_SIDES = 100;

    // the number of vertices is the number of sides * 2 (think, two vertices per edge line), plus 
    // 2 for the center points at the top and bottom; since each vertex has a stride of 7 (x,y,z,r,g,b,a) would
    // also need to multiply by seven
    const int NUM_VERTICES = STRIDE * (2 + (2 * NUM_SIDES));

    // the number of indices of a prism is 3 * the number of triangle that will be drawn; therefore, for a cube prism
    // with a center point on each end (i.e. the smaller ends will be made up of four triangles instead of two) there 
    // would be (4 triangles * 2 ends) + (2 triangles * 4 sides) = 16 triangles. Therefore, a total of 3 * 16, or 48 vertices
    // for a cube prism. Or, more generally, 4 triangles needed for every side (top slice of pie, bottom, and two for the rectangle 
    // on the side). Or, 12 * num sides is the amount of indices needed.
    const int NUM_INDICES = 12 * NUM_SIDES;

    // Position and Color data
    GLfloat verts[NUM_VERTICES];

    // Index data to share position data
    GLushort indices[NUM_INDICES];

    // fill the verts and indices arrays with data
    UCreatePrismMesh(verts, indices, NUM_SIDES, 0.25f, 0.02f);

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;
    if (STRIDE != floatsPerVertex + floatsPerColor)
    {
        exit(EXIT_FAILURE);
    }

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
}


void UCreatePlateMesh(GLMesh& mesh)
{
    // Vertex data
    GLfloat verts[] = {
    -0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f,  // top left
     0.5f,  0.5f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f,  0.0f, 1.0f, // top right
     0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f,  0.0f, 1.0f,// bottom right
    -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 1.0f, 1.0f,  0.0f, 1.0f// bottom left
    };

    // Index data
    GLuint indices[] = {
    0, 1, 2,  // first triangle
    2, 3, 0   // second triangle
    };

    // total float values per each type
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 4;
    const GLuint floatsPerUV = 2;

    // store vertex and index count
    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

    // Generate the VAO for the mesh
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);	// activate the VAO

    // Create VBOs for the mesh
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends data to the GPU

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]); // Activates the buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}



// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

