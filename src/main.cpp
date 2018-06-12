/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "recordAudio.h"
#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>

#define SCALING 0.5

using namespace std;
using namespace glm;
shared_ptr<Shape> shape;
extern captureAudio actualAudioData;
extern int running;
int currIndex;
float avgHeight;

#include "fftw/fftw3.h"
#include <math.h>
#include <algorithm>    

BYTE texels[TEXSIZE * TEXSIZE * 4];
fftw_complex out[10000];
// note: R is low, G is high
//BYTE yOff[TEXSIZE*SAMPLES*4];

#define FFTW_ESTIMATEE (1U << 6)
#define AVG_HEIGHT 40.0

fftw_complex * fft(int &length)
{
    fftw_complex *out = NULL;
    int N = pow(2, 10);
    BYTE data[MAXS];
    int size = 0;
    actualAudioData.readAudio(data, size);
    length = size / 8;
    if (size == 0)
        return NULL;

    double *samples = new double[length];
    for (int ii = 0; ii < length; ii++)
    {
        float *f = (float*)&data[ii * 8];
        samples[ii] = (double)(*f);
    }
    out = new fftw_complex[length];
    fftw_plan p;
    p = fftw_plan_dft_r2c_1d(length, samples, out, FFTW_ESTIMATEE);

    fftw_execute(p);
    fftw_destroy_plan(p);

    delete[] samples;

    return out;
}
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

BYTE delayfilter(BYTE old, BYTE actual, float mul)
{
    float fold = (float)old;
    float factual = (float)actual;
    float fres = fold - (fold - factual) / mul;
    if (fres > 255) fres = 255;
    else if (fres < 0)fres = 0;
    return (BYTE)fres;
}

void write_to_tex(GLuint texturebuf, int resx, int resy)
{
    int length;
    static float arg = 0;
    arg += 0.1;
    float erg = sin(arg) + 1;
    erg *= 100;
    fftw_complex *outfft = fft(length);
    for (int x = 0; x < resx; ++x)
    {
        if (x >= length / 2)break;
        float fftd = sqrt(outfft[x][0] * outfft[x][0] + outfft[x][1] * outfft[x][1]);
        int y = currIndex - 1 < 0 ? TEXSIZE - 1 : currIndex - 1;
        int index = x * 4 + y * resx * 4;
        BYTE oldval = texels[index];
        texels[x * 4 + currIndex * resx * 4 + 0] = delayfilter(oldval, (BYTE)(fftd*60.0), 15);
        avgHeight += texels[x * 4 + currIndex * resx * 4];
    }
    avgHeight /= resx;

    /*for (int i = 0; i < TEXSIZE / 2; i++)
    {
        yOff[i * 4 + currIndex * TEXSIZE * 4 + 0] = texels[2 * i * 4 + 0];
        yOff[(TEXSIZE - i) * 4 + currIndex * TEXSIZE * 4 + 0] = texels[2 * i * TEXSIZE * 4];
    }*/
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturebuf);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXSIZE, TEXSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels);
    glGenerateMipmap(GL_TEXTURE_2D);

    delete[] outfft;
}

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}
class camera
{
public:
    glm::vec3 pos, rot;
    int w, a, s, d, q, e, z, c;
    camera()
    {
        w = a = s = d = q = e = z = c = 0;
        pos = glm::vec3(-TEXSIZE / 2.5, -40, TEXSIZE / 4);
        rot = glm::vec3(3.141596 / 15.0, -3.141596 / 2.0, 0);
    }
    glm::mat4 process(double ftime)
    {
        float speed = 0;
        if (w == 1)
        {
            speed = 90 * ftime;
        }
        else if (s == 1)
        {
            speed = -90 * ftime;
        }
        float yangle = 0;
        if (a == 1)
            yangle = -3 * ftime;
        else if (d == 1)
            yangle = 3 * ftime;
        rot.y += yangle;
        float zangle = 0;
        if (q == 1)
            zangle = -3 * ftime;
        else if (e == 1)
            zangle = 3 * ftime;
        rot.z += zangle;
        float xangle = 0;
        if (z == 1)
            xangle = -0.3 * ftime;
        else if (c == 1)
            xangle = 0.3 * ftime;
        rot.x += xangle;

        glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
        glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
        glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
        glm::vec4 dir = glm::vec4(0, 0, speed, 1);
        R = Rx * Rz * R;
        dir = dir * R;
        pos += glm::vec3(dir.x, dir.y, dir.z);
        glm::mat4 T = glm::translate(glm::mat4(1), pos);
        return R * T;
    }
};
camera mycam;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog, crysProg;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our box to OpenGL
	GLuint VertexBufferID, VertexColorIDBox, IndexBufferIDBox;

    GLuint AudioTex;

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        if (key == GLFW_KEY_W && action == GLFW_PRESS)
        {
            mycam.w = 1;
        }
        if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        {
            mycam.w = 0;
        }
        if (key == GLFW_KEY_S && action == GLFW_PRESS)
        {
            mycam.s = 1;
        }
        if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        {
            mycam.s = 0;
        }
        if (key == GLFW_KEY_A && action == GLFW_PRESS)
        {
            mycam.a = 1;
        }
        if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        {
            mycam.a = 0;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
        {
            mycam.d = 1;
        }
        if (key == GLFW_KEY_D && action == GLFW_RELEASE)
        {
            mycam.d = 0;
        }
        if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        {
            mycam.q = 1;
        }
        if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
        {
            mycam.q = 0;
        }
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            mycam.e = 1;
        }
        if (key == GLFW_KEY_E && action == GLFW_RELEASE)
        {
            mycam.e = 0;
        }
        if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        {
            mycam.z = 1;
        }
        if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
        {
            mycam.z = 0;
        }
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            mycam.c = 1;
        }
        if (key == GLFW_KEY_C && action == GLFW_RELEASE)
        {
            mycam.c = 0;
        }

    }

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;
		float newPt[2];
		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			std::cout << "Pos X " << posX <<  " Pos Y " << posY << std::endl;

			//change this to be the points converted to WORLD
			//THIS IS BROKEN< YOU GET TO FIX IT - yay!
			newPt[0] = 0;
			newPt[1] = 0;

			std::cout << "converted:" << newPt[0] << " " << newPt[1] << std::endl;
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
			//update the vertex array with the updated points
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*6, sizeof(float)*2, newPt);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}

	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{
		string resourceDirectory = "../resources" ;
		// Initialize mesh.
		shape = make_shared<Shape>();
		//shape->loadMesh(resourceDirectory + "/t800.obj");
		shape->loadMesh(resourceDirectory + "/sphere.obj");
		shape->resize();
		shape->init();

        // memset(&yOff, 0, TEXSIZE * SAMPLES * 4);
	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = std::make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("campos");
        prog->addUniform("currIndex");
        prog->addUniform("xtex");
        prog->addUniform("scaling");
        prog->addUniform("avgheight");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");



        //[TWOTEXTURES]
        //set the 2 textures to the correct samplers in the fragment shader:
        GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
        GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
        // Then bind the uniform samplers to texture units:
        glUseProgram(prog->pid);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);


        // dynamic audio texture

        /*
        int datasize = TEXSIZE *TEXSIZE * 4 * sizeof(GLfloat);
        glGenBuffers(1, &AudioTexBuf);
        glBindBuffer(GL_TEXTURE_BUFFER, AudioTexBuf);
        glBufferData(GL_TEXTURE_BUFFER, datasize, NULL, GL_DYNAMIC_COPY);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);

        glGenTextures(1, &AudioTex);
        glBindTexture(GL_TEXTURE_BUFFER, AudioTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, AudioTexBuf);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
        //glBindImageTexture(2, AudioTex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
        */
        for (int ii = 0; ii < TEXSIZE * TEXSIZE * 4; ii++)
            texels[ii] = 0;
        glGenTextures(1, &AudioTex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, AudioTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXSIZE, TEXSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels);
        //glGenerateMipmap(GL_TEXTURE_2D);

        currIndex = 0;
	}

	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{
		double frametime = get_last_elapsed_time();

        static float curr = 0;
        curr += 0.1;

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width/(float)height;
		glViewport(0, 0, width, height);


        float r = max(0, min(0.5, avgHeight / AVG_HEIGHT - 1));
        float g = max(0,  min(0.5, avgHeight / AVG_HEIGHT - 1));
        float b = max(0, min(0.5, avgHeight / AVG_HEIGHT - 1));

        glClearColor(r, g, b, 1.0);
		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// Create the matrix stacks - please leave these alone for now
		
		glm::mat4 V, M, P, M2; //View, Model and Perspective matrix
		V = mycam.process(frametime);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones

		//animation with the model matrix:

        mat4 Flatten = scale(mat4(1), vec3(SCALING, SCALING / 5.0, SCALING));
        mat4 Rotate = rotate(mat4(1.0f), (float)(3.1415963 / 4.0), vec3(1.0f, 0.0f, 0.0f));

        M = Flatten * Rotate;

		// Draw the box using GLSL.
		prog->bind();

        glEnable(GL_DEPTH_TEST);

        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);

        glUniform1i(prog->getUniform("currIndex"), currIndex);
        glUniform1i(prog->getUniform("xtex"), (int)(TEXSIZE));
        glUniform1f(prog->getUniform("scaling"), (float)SCALING);
        glUniform1f(prog->getUniform("avgheight"), avgHeight);

        // Generate a sample for the history
        write_to_tex(AudioTex, TEXSIZE, TEXSIZE);

        //generateHeights(AudioTex, curr);
        shape->draw(prog, true);

		prog->unbind();

        currIndex = ++currIndex % TEXSIZE;
	}

};
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1400, 800);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

    thread t1(start_recording);

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}
    running = FALSE;
    t1.join();

	// Quit program.
	windowManager->shutdown();
	return 0;
}
