// TODO - Win state

// tag::C++11check[]
#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if __cplusplus < 201103L
#pragma message("WARNING: the compiler may not be C++11 compliant. __cplusplus version is : " STRING(__cplusplus))
#endif
// end::C++11check[]

// tag::includes[]
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <math.h>

#include <GL/glew.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
// end::includes[]

// tag::using[]
// see https://isocpp.org/wiki/faq/Coding-standards#using-namespace-std
// don't use the whole namespace, either use the specific ones you want, or just type std::
using std::cout;
using std::endl;
using std::max;
// end::using[]


// tag::globalVariables[]
std::string exeName;
SDL_Window *win; //pointer to the SDL_Window
SDL_GLContext context; //the SDL_GLContext
int frameCount = 0;
std::string frameLine = "";
// end::globalVariables[]

// tag::vertexShader[]
//string holding the **source** of our vertex shader, to save loading from a file

// mat4 multiplaction in the shader seems to be the issue? Why? 

const std::string strVertexShader = R"(
	#version 330

		in vec2 position;
	in vec4 vertexColor;

		out vec4 fragmentColor;

		uniform vec2 offset;
	uniform mat4 rotation;

		uniform mat4 modelMatrix      = mat4(1.0);
	uniform mat4 viewMatrix       = mat4(1.0);
	uniform mat4 projectionMatrix = mat4(1.0);

		void main()
	{
		vec2 tempPos = position + offset;
		gl_Position = projectionMatrix * viewMatrix * modelMatrix * rotation * vec4(tempPos, 0.0, 1.0);
		fragmentColor = vertexColor;
	}
)";
// end::vertexShader[]

// tag::fragmentShader[]
//string holding the **source** of our fragment shader, to save loading from a file
const std::string strFragmentShader = R"(
	#version 330
	in vec4 fragmentColor;
	out vec4 outputColor;
	void main()
	{
		 outputColor = fragmentColor;
	}
)";
// end::fragmentShader[]

//my variables
bool done = false;
bool ballMovingLeft = true;
bool wDown = false;
bool sDown = false;
bool upArrowDown = false;
bool downArrowDown = false;
bool rotateRight = true;

float ballIncrement = 0.005f;
float ballYDirection = 0.000f;
float rotateAngle = 0.000f;

int playerOneScore = 0;
int playerTwoScore = 0;

// tag::vertexData[]
//the data about our geometry
const GLfloat vertexDataLeftPaddle[] = {
	// X        Y        R     G     B      A
	-0.855f,  0.150f,   0.9f, 0.5f, 0.25f,  1.0f,
	-0.855f, -0.150f,   0.9f, 0.5f, 0.25f,  1.0f,
	-0.875f,  0.150f,   0.9f, 0.25f, 0.25f,  1.0f,
	-0.875f, -0.150f,   0.9f, 0.25f, 0.25f,  1.0f
};
const GLfloat vertexDataRightPaddle[] = {
	// X        Y        R     G     B      A
	0.855f,  0.150f,   0.5f, 0.25f, 0.9f,  1.0f,
	0.855f, -0.150f,   0.5f, 0.25f, 0.9f,  1.0f,
	0.875f,  0.150f,   0.25f, 0.25f, 0.9f,  1.0f,
	0.875f, -0.150f,   0.25f, 0.25f, 0.9f,  1.0f
};
const GLfloat vertexDataBall[] = {
	// X        Y        Z     R     G     B      A
	-0.015f,  0.015f, 0.000f, 1.0f, 1.0f, 1.0f,  1.0f,
	-0.015f, -0.015f, 0.000f, 1.0f, 1.0f, 1.0f,  1.0f,
	 0.015f,  0.015f, 0.000f, 1.0f, 1.0f, 1.0f,  1.0f,
	 0.015f, -0.015f, 0.000f, 1.0f, 1.0f, 1.0f,  1.0f
};
const GLfloat vertexDataBoundry[] = {
	// X        Y        R     G     B      A
	-0.990f, -0.980f,   0.0f, 0.0f, 0.0f,  1.0f,
	-0.990f,  0.980f,   0.0f, 0.0f, 0.0f,  1.0f,
	0.990f,  -0.980f,   0.0f, 0.0f, 0.0f,  1.0f,
	0.990f,   0.980f,   0.0f, 0.0f, 0.0f,  1.0f
};
GLfloat vertexDataScoreMarker[6000];

//the color we'll pass to the GLSL
GLfloat color[] = { 1.0f, 1.0f, 1.0f }; //using different values from CPU and static GLSL examples, to make it clear this is working
GLfloat leftPaddleOffset[] = { 0.0f, 0.0f };
GLfloat rightPaddleOffset[] = { 0.0f, 0.0f };
GLfloat ballOffset[] = { 0.0f, 0.0f };

// tag::GLVariables[]
//my GL and GLSL variables
GLuint theProgram; //GLuint that we'll fill in to refer to the GLSL program (only have 1 at this point)
GLint positionLocation; //GLuint that we'll fill in with the location of the `position` attribute in the GLSL
GLint vertexColorLocation; //GLuint that we'll fill in with the location of the `vertexColor` attribute in the GLSL
GLint offsetLocation;
GLint uniRotation;

GLuint vertexDataBufferObjectLeftPaddle;
GLuint vertexDataBufferObjectRightPaddle;
GLuint vertexDataBufferObjectBall;
GLuint vertexDataBufferObjectBoundry;
GLuint vertexArrayObjectLeftPaddle;
GLuint vertexArrayObjectRightPaddle;
GLuint vertexArrayObjectBall;
GLuint vertexArrayObjectBoundry;
// end::GLVariables[]

// tag::glmVariables[]
glm::mat4 rotation;
// end::glmVariables[]

// end Global Variables
/////////////////////////

// tag::initialise[]
void initialise()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		exit(1);
	}
	cout << "SDL initialised OK!\n";
}
// end::initialise[]

// tag::createWindow[]
void createWindow()
{
	//get executable name, and use as window title
	int beginIdxWindows = exeName.rfind("\\"); //find last occurrence of a backslash
	int beginIdxLinux = exeName.rfind("/"); //find last occurrence of a forward slash
	int beginIdx = max(beginIdxWindows, beginIdxLinux);
	std::string exeNameEnd = exeName.substr(beginIdx + 1);
	const char *exeNameCStr = exeNameEnd.c_str();

	//create window
	win = SDL_CreateWindow(exeNameCStr, 100, 100, 1000, 600, SDL_WINDOW_OPENGL); //same height and width makes the window square ...

																				 //error handling
	if (win == nullptr)
	{
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "SDL CreatedWindow OK!\n";
}
// end::createWindow[]

// tag::setGLAttributes[]
void setGLAttributes()
{
	int major = 3;
	int minor = 3;
	cout << "Built for OpenGL Version " << major << "." << minor << endl; //ahttps://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions
																		  // set the opengl context version
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //core profile
	cout << "Set OpenGL context to versicreate remote branchon " << major << "." << minor << " OK!\n";
}
// tag::setGLAttributes[]

// tag::createContext[]
void createContext()
{
	setGLAttributes();

	context = SDL_GL_CreateContext(win);
	if (context == nullptr) {
		SDL_DestroyWindow(win);
		std::cout << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "Created OpenGL context OK!\n";
}
// end::createContext[]

// tag::initGlew[]
void initGlew()
{
	GLenum rev;
	glewExperimental = GL_TRUE; //GLEW isn't perfect - see https://www.opengl.org/wiki/OpenGL_Loading_Library#GLEW
	rev = glewInit();
	if (GLEW_OK != rev) {
		std::cout << "GLEW Error: " << glewGetErrorString(rev) << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLEW Init OK!\n";
	}
}
// end::initGlew[]

// tag::createShader[]
GLuint createShader(GLenum eShaderType, const std::string &strShaderFile)
{
	GLuint shader = glCreateShader(eShaderType);
	//error check
	const char *strFileData = strShaderFile.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch (eShaderType)
		{
		case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
		case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
		case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	return shader;
}
// end::createShader[]

// tag::createProgram[]
GLuint createProgram(const std::vector<GLuint> &shaderList)
{
	GLuint program = glCreateProgram();

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glAttachShader(program, shaderList[iLoop]);

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glDetachShader(program, shaderList[iLoop]);

	return program;
}
// end::createProgram[]

// tag::initializeProgram[]
void initializeProgram()
{
	std::vector<GLuint> shaderList;

	shaderList.push_back(createShader(GL_VERTEX_SHADER, strVertexShader));
	shaderList.push_back(createShader(GL_FRAGMENT_SHADER, strFragmentShader));

	theProgram = createProgram(shaderList);
	if (theProgram == 0)
	{
		cout << "GLSL program creation error." << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLSL program creation OK! GLUint is: " << theProgram << std::endl;
	}

	positionLocation = glGetAttribLocation(theProgram, "position");
	offsetLocation = glGetUniformLocation(theProgram, "offset");
	uniRotation = glGetUniformLocation(theProgram, "rotation");
	glUniformMatrix4fv(uniRotation, 1, GL_FALSE, glm::value_ptr(rotation));
	vertexColorLocation = glGetAttribLocation(theProgram, "vertexColor");
	//clean up shaders (we don't need them anymore as they are no in theProgram
	for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
}
// end::initializeProgram[]

// tag::initializeVertexArrayObject[]
//setup a GL object (a VertexArrayObject) that stores how to access data and from where
void initializeVertexArrayObject()
{
	// Left Paddle

	glGenVertexArrays(1, &vertexArrayObjectLeftPaddle); //create a Vertex Array Object
	cout << "Vertex Array Object created OK! GLUint is: " << vertexArrayObjectLeftPaddle << std::endl;

	glBindVertexArray(vertexArrayObjectLeftPaddle); //make the just created vertexArrayObject the active one
	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObjectLeftPaddle); //bind vertexDataBufferObject
	glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation
	glEnableVertexAttribArray(vertexColorLocation); //enable attribute at index vertexColorLocation
	glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(0 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation
	glVertexAttribPointer(vertexColorLocation, 4, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(2 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation
	glBindVertexArray(0); //unbind the vertexArrayObject so we can't change it

						  // Right Paddle

	glGenVertexArrays(1, &vertexArrayObjectRightPaddle);
	cout << "Vertex Array Object created OK! GLUint is: " << vertexArrayObjectRightPaddle << std::endl;

	glBindVertexArray(vertexArrayObjectRightPaddle);
	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObjectRightPaddle);
	glEnableVertexAttribArray(positionLocation);
	glEnableVertexAttribArray(vertexColorLocation);
	glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(0 * sizeof(GLfloat)));
	glVertexAttribPointer(vertexColorLocation, 4, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(2 * sizeof(GLfloat)));
	glBindVertexArray(0);

	// Ball

	glGenVertexArrays(1, &vertexArrayObjectBall);
	cout << "Vertex Array Object created OK! GLUint is: " << vertexArrayObjectBall << std::endl;

	glBindVertexArray(vertexArrayObjectBall);
	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObjectBall);
	glEnableVertexAttribArray(positionLocation);
	glEnableVertexAttribArray(vertexColorLocation);
	glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, (7 * sizeof(GL_FLOAT)), (GLvoid *)(0 * sizeof(GLfloat)));
	glVertexAttribPointer(vertexColorLocation, 4, GL_FLOAT, GL_FALSE, (7 * sizeof(GL_FLOAT)), (GLvoid *)(3 * sizeof(GLfloat)));
	glBindVertexArray(0);

	// Boundry

	glGenVertexArrays(1, &vertexArrayObjectBoundry);
	cout << "Vertex Array Object created OK! GLUint is: " << vertexArrayObjectBoundry << std::endl;

	glBindVertexArray(vertexArrayObjectBoundry);
	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObjectBoundry);
	glEnableVertexAttribArray(positionLocation);
	glEnableVertexAttribArray(vertexColorLocation);
	glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(0 * sizeof(GLfloat)));
	glVertexAttribPointer(vertexColorLocation, 4, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(2 * sizeof(GLfloat)));
	glBindVertexArray(0);

	// Cleanup

	glDisableVertexAttribArray(positionLocation); //disable vertex attribute at index positionLocation
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer

}
// end::initializeVertexArrayObject[]

// tag::initializeVertexBuffer[]
void initializeVertexBuffer()
{

	// Left Paddle
	glGenBuffers(1, &vertexDataBufferObjectLeftPaddle);

	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObjectLeftPaddle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexDataLeftPaddle), vertexDataLeftPaddle, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "vertexDataBufferObject created OK! GLUint is: " << vertexDataBufferObjectLeftPaddle << std::endl;

	// Right Paddle
	glGenBuffers(1, &vertexDataBufferObjectRightPaddle);

	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObjectRightPaddle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexDataRightPaddle), vertexDataRightPaddle, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "vertexDataBufferObject2 created OK! GLUint is: " << vertexDataBufferObjectRightPaddle << std::endl;

	// Ball
	glGenBuffers(1, &vertexDataBufferObjectBall);

	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObjectBall);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexDataBall), vertexDataBall, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "vertexDataBufferObjectBall created OK! GLUint is: " << vertexDataBufferObjectBall << std::endl;

	// Ball
	glGenBuffers(1, &vertexDataBufferObjectBoundry);

	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObjectBoundry);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexDataBoundry), vertexDataBoundry, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "vertexDataBufferObjectBoundry created OK! GLUint is: " << vertexDataBufferObjectBoundry << std::endl;

	initializeVertexArrayObject();
}
// end::initializeVertexBuffer[]

// tag::loadAssets[]
void loadAssets()
{
	initializeProgram(); //create GLSL Shaders, link into a GLSL program, and get IDs of attributes and variables

	initializeVertexBuffer(); //load data into a vertex buffer

	cout << "Loaded Assets OK!\n";
}
// end::loadAssets[]

// tag::handleInput[]
void handleInput()
{
	//Event-based input handling
	//The underlying OS is event-based, so **each** key-up or key-down (for example)
	//generates an event.
	//  - https://wiki.libsdl.org/SDL_PollEvent
	//In some scenarios we want to catch **ALL** the events, not just to present state
	//  - for instance, if taking keyboard input the user might key-down two keys during a frame
	//    - we want to catch based, and know the order
	//  - or the user might key-down and key-up the same within a frame, and we still want something to happen (e.g. jump)
	//  - the alternative is to Poll the current state with SDL_GetKeyboardState

	SDL_Event event; //somewhere to store an event

					 //NOTE: there may be multiple events per frame
	while (SDL_PollEvent(&event)) //loop until SDL_PollEvent returns 0 (meaning no more events)
	{
		switch (event.type)
		{
		case SDL_QUIT:
			done = true; //set donecreate remote branch flag if SDL wants to quit (i.e. if the OS has triggered a close event,
						 //  - such as window close, or SIGINT
			break;

			//keydown handling - we should to the opposite on key-up for direction controls (generally)
		case SDL_KEYDOWN:
			//Keydown can fire repeatable if key-repeat is on.
			//  - the repeat flag is set on the keyboard event, if this is a repeat event
			//  - in our case, we're going to ignore repeat events
			//  - https://wiki.libsdl.org/SDL_KeyboardEvent
			//if (!event.key.repeat)
			switch (event.key.keysym.sym)
			{
				//hit escape to exit
			case SDLK_ESCAPE: done = true;
				break;
			case SDLK_w:
				wDown = true;
				break;
			case SDLK_s:
				sDown = true;
				break;
			case SDLK_UP:
				upArrowDown = true;
				break;
			case SDLK_DOWN:
				downArrowDown = true;
				break;
			}
			break;
		case SDL_KEYUP:
			//Keydown can fire repeatable if key-repeat is on.
			//  - the repeat flag is set on the keyboard event, if this is a repeat event
			//  - in our case, we're going to ignore repeat events
			//  - https://wiki.libsdl.org/SDL_KeyboardEvent
			//if (!event.key.repeat)
			switch (event.key.keysym.sym)
			{
			case SDLK_w:
				wDown = false;
				break;
			case SDLK_s:
				sDown = false;
				break;
			case SDLK_UP:
				upArrowDown = false;
				break;
			case SDLK_DOWN:
				downArrowDown = false;
				break;
			}
			break;
		}
	}
}
// end::handleInput[]

void BounceBallOffPaddle(float ballMidY, float paddleMidY, float paddleTopY, float paddleBottomY)
{
	float ballYImpactPoint;

	ballIncrement *= 1.1f;
	ballIncrement *= -1;
	rotateRight = false;

	if (ballMidY > paddleMidY)
	{
		ballYImpactPoint = (ballMidY - paddleMidY) / (paddleTopY - paddleMidY);
		cout << "Impact Percentage: " << ballYImpactPoint << endl;
		ballYDirection = ballYImpactPoint / 100;
	}
	else if (ballMidY < paddleMidY)
	{
		ballYImpactPoint = ((ballMidY - paddleMidY) / (paddleBottomY - paddleMidY) * -1);
		cout << "Impact Percentage: " << ballYImpactPoint << endl;
		ballYDirection = ballYImpactPoint / 100;
	}
}

void ResetBall()
{
	ballOffset[0] = 0.0f;
	ballOffset[1] = 0.0f;
	ballYDirection = 0.0f;

	if (playerOneScore < 6 & playerTwoScore < 6)
	{
		if (ballIncrement < 0)
		{
			ballIncrement = -0.005f;
		}
		else
		{
			ballIncrement = 0.005f;
		}
	}
	else
	{
		ballIncrement = 0.0f;
	}
}

void BallWindowCollisions(float ballTop, float ballBottom, float ballRight, float ballLeft)
{
	// Wall Collisions
	// Top/Bottom (Reverse ball Y direction)
	if (ballTop >= 0.98f)
		ballYDirection *= -1;
	else if (ballBottom <= -0.98f)
		ballYDirection *= -1;
	// Back Walls (Opposing player scores a point)
	if (ballRight >= 1.0f)
	{
		if (playerOneScore < 6)
			playerOneScore++;
		ResetBall();
	}
	else if (ballLeft <= -1.0f)
	{
		if (playerTwoScore < 6)
			playerTwoScore++;
		ResetBall();
	}
}

void BallController()
{
	// Current ball vertice positions
	float ball_leftXVertice = -0.015f + ballOffset[0];
	float ball_rightXVertice = 0.015f + ballOffset[0];
	float ball_topYVertice = 0.015f + ballOffset[1];
	float ball_bottomYVertice = -0.015f + ballOffset[1];
	float ball_midY = (ball_topYVertice + ball_bottomYVertice) / 2;
	float ballYImpactPoint;

	// Current left paddle vertice positions
	float leftPaddle_leftXVertice = -0.875f + leftPaddleOffset[0];
	float leftPaddle_rightXVertice = -0.855f + leftPaddleOffset[0];
	float leftPaddle_topYVertice = 0.150f + leftPaddleOffset[1];
	float leftPaddle_bottomYVertice = -0.150f + leftPaddleOffset[1];
	float leftPaddle_midY = (leftPaddle_topYVertice + leftPaddle_bottomYVertice) / 2;

	// Current right paddle vertice positions
	float rightPaddle_leftXVertice = 0.855f + rightPaddleOffset[0];
	float rightPaddle_rightXVertice = 0.875f + rightPaddleOffset[0];
	float rightPaddle_topYVertice = 0.150f + rightPaddleOffset[1];
	float rightPaddle_bottomYVertice = -0.150f + rightPaddleOffset[1];
	float rightPaddle_midY = (rightPaddle_topYVertice + rightPaddle_bottomYVertice) / 2;

	// Left/Right ball movement
	// Bouncing off right paddle
	if (ball_rightXVertice > (rightPaddle_leftXVertice - 0.023f)
		&& ball_leftXVertice < rightPaddle_rightXVertice
		&& ball_topYVertice > rightPaddle_bottomYVertice
		&& ball_bottomYVertice < rightPaddle_topYVertice)
	{
		BounceBallOffPaddle(ball_midY, rightPaddle_midY, rightPaddle_topYVertice, rightPaddle_bottomYVertice);
	}
	// Bouncing off left paddle
	else if (ball_leftXVertice < (leftPaddle_rightXVertice - 0.01f)
		&& ball_rightXVertice > leftPaddle_leftXVertice
		&& ball_topYVertice > leftPaddle_bottomYVertice
		&& ball_bottomYVertice < leftPaddle_topYVertice)
	{
		BounceBallOffPaddle(ball_midY, leftPaddle_midY, leftPaddle_topYVertice, leftPaddle_bottomYVertice);
	}

	BallWindowCollisions(ball_topYVertice, ball_bottomYVertice, ball_rightXVertice, ball_leftXVertice);

}

// tag::updateSimulation[]
void updateSimulation(double simLength = 0.02) //update simulation with an amount of time to simulate for (in seconds)
{
	//WARNING - we should calculate an appropriate amount of time to simulate - not always use a constant amount of time
	// see, for example, http://headerphile.blogspot.co.uk/2014/07/part-9-no-more-delays.html
	//CHANGE ME

	if (wDown && leftPaddleOffset[1] < 0.8f)
	{
		leftPaddleOffset[1] += 0.025f;
	}
	else if (sDown && leftPaddleOffset[1] > -0.8f)
	{
		leftPaddleOffset[1] -= 0.025f;
	}

	if (upArrowDown && rightPaddleOffset[1] < 0.8f)
	{
		rightPaddleOffset[1] += 0.025f;
	}
	else if (downArrowDown && rightPaddleOffset[1] > -0.8f)
	{
		rightPaddleOffset[1] -= 0.025f;
	}

	ballOffset[0] += ballIncrement;
	ballOffset[1] += ballYDirection;

	BallController();
}
// end::updateSimulation[]

// tag::preRender[]
void preRender()
{
	glViewport(0, 0, 1000, 600); //set viewpoint
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); //set clear colour
	glClear(GL_COLOR_BUFFER_BIT); //clear the window (technical the scissor box bounds)
}
// end::preRender[]

void renderScoreMarker(float rightX, float modifier)
{

	GLuint tempVertexDataBufferObject;
	GLuint tempVertexArrayObject;
	float R;
	float G;
	float B;
	float A = 1.0f;
	float gradient = 0.25f;

	if (rightX > 1)
	{
		R = 0.5f;
		G = 0.25f;
		B = 0.9f;
	}
	else
	{
		R = 0.9f;
		G = 0.5f;
		B = 0.25f;
	}

	// Generate vertex data for a circle with ~1000 triangles
	for (int i = 0; i < 6000; i += 12)
	{
		float theta = 2.0f * M_PI * float(i) / float(1000);

		float xAngle = 0.03f * cosf(theta);
		float yAngle = 0.04f * sinf(theta);

		vertexDataScoreMarker[i] = xAngle + rightX + modifier; // X axis - Take the generated x axis and apply the rightX offset and modifier
		vertexDataScoreMarker[i + 1] = yAngle + 0.905f; // Y axis - Move the circle up
														// gradient logic
		if (rightX > 1)
			vertexDataScoreMarker[i + 2] = gradient; // R - Applys gradient to player two's markers
		else
			vertexDataScoreMarker[i + 2] = R;  // R
		if (rightX < 1)
			vertexDataScoreMarker[i + 3] = gradient; // G - Apply's gradient to player one's markers
		else
			vertexDataScoreMarker[i + 3] = G;  // G
											   // end graident logic
		vertexDataScoreMarker[i + 4] = B;  // B
		vertexDataScoreMarker[i + 5] = A;  // A

		vertexDataScoreMarker[i + 6] = xAngle + rightX + modifier; // X
		vertexDataScoreMarker[i + 7] = (yAngle * -1) + 0.905f; // Y
		vertexDataScoreMarker[i + 8] = R;  // R
		vertexDataScoreMarker[i + 9] = G;  // G
		vertexDataScoreMarker[i + 10] = B; // B
		vertexDataScoreMarker[i + 11] = A; // A
	}

	glGenBuffers(1, &tempVertexDataBufferObject);

	// Stops marker inheriting offset values
	glUniform2f(offsetLocation, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, tempVertexDataBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexDataScoreMarker), vertexDataScoreMarker, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &tempVertexArrayObject); //create a Vertex Array Object

	glBindVertexArray(tempVertexArrayObject); //make the just created vertexArrayObject2 the active one
	glBindBuffer(GL_ARRAY_BUFFER, tempVertexDataBufferObject); //bind vertexDataBufferObject
	glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation
	glEnableVertexAttribArray(vertexColorLocation); //enable attribute at index vertexColorLocation
	glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(0 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation
	glVertexAttribPointer(vertexColorLocation, 4, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(2 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation
	glBindVertexArray(0); //unbind the vertexArrayObject2 so we can't change it

	glDisableVertexAttribArray(positionLocation); //disable vertex attribute at index positionLocation
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer

	glBindVertexArray(tempVertexArrayObject);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 1000);
	glBindVertexArray(0);
}

// Can I change this to also pass in a vertexDataArray and playerNumber to use this for both playerOne and playerTwo?
void renderScoreMarkers(float index, bool playerOneScored)
{

	float rightX;
	float modifier;

	if (playerOneScored)
	{
		rightX = -1.320f;
		modifier = ((index / 2.00f) + 2.30f) / 6.00f;
	}
	else
	{
		rightX = 1.320f;
		modifier = ((-index / 2.00f) - 2.30f) / 6.00f;
	}

	renderScoreMarker(rightX, modifier);

}

// tag::render[]
void render()
{
	glUseProgram(theProgram); //installs the program object specified by program as part of current rendering state

							  // Boundry
	glUniform2f(offsetLocation, 0, 0);
	glBindVertexArray(vertexArrayObjectBoundry);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	// Left Paddle
	glUniform2f(offsetLocation, leftPaddleOffset[0], leftPaddleOffset[1]);
	glBindVertexArray(vertexArrayObjectLeftPaddle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	// Right Paddle
	glUniform2f(offsetLocation, rightPaddleOffset[0], rightPaddleOffset[1]);
	glBindVertexArray(vertexArrayObjectRightPaddle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	// Ball
	if (rotateRight)
	{
		rotateAngle -= 5.0f;
	}
	else
	{
		rotateAngle += 5.0f;
	}
	rotation = glm::translate(rotation, glm::vec3(ballOffset[0] + 0.015f, ballOffset[1], 0.0f));
	rotation = glm::rotate(rotation, glm::radians(rotateAngle), glm::vec3(0.0f, 0.0f, 1.0f));
	rotation = glm::translate(rotation, glm::vec3(-ballOffset[0], -ballOffset[1], 0.0f));
	glUniformMatrix4fv(uniRotation, 1, GL_FALSE, glm::value_ptr(rotation));

	glUniform2f(offsetLocation, ballOffset[0], ballOffset[1]);
	glBindVertexArray(vertexArrayObjectBall);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	// Reset rotation
	rotation = glm::mat4();
	glUniformMatrix4fv(uniRotation, 1, GL_FALSE, glm::value_ptr(rotation));

	// Render Player One Score Markers
	for (int i = 0; i < playerOneScore; i++)
	{
		renderScoreMarkers(i, true);
	}
	// Render Player Two Score Markers
	for (int i = 0; i < playerTwoScore; i++)
	{
		renderScoreMarkers(i, false);
	}

	glUseProgram(0); //clean up

}
// end::render[]

// tag::postRender[]
void postRender()
{
	SDL_GL_SwapWindow(win);; //present the frame buffer to the display (swapBuffers)
	frameLine += "Frame: " + std::to_string(frameCount++);
	cout << "\r" << frameLine << std::flush;
	frameLine = "";
}
// end::postRender[]

// tag::cleanUp[]
void cleanUp()
{
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	cout << "Cleaning up OK!\n";
}
// end::cleanUp[]

// tag::main[]
int main(int argc, char* args[])
{
	exeName = args[0];
	//setup
	//- do just once
	initialise();
	createWindow();

	createContext();

	initGlew();

	SDL_GL_SwapWindow(win); //force a swap, to make the trace clearer

	loadAssets();

	while (!done) //loop until done flag is set)
	{

		handleInput(); // this should ONLY SET VARIABLES

		updateSimulation(); // this should ONLY SET VARIABLES according to simulation

		preRender();

		render(); // this should render the world state according to VARIABLES -

		postRender();

	}

	//cleanup and exit
	cleanUp();
	SDL_Quit();

	return 0;
}
// end::main[]
