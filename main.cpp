
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "Math.h"
#include "teapot.h" // teapot mesh
//#include "monkey.h" // monkey mesh

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;

unsigned int teapot_vao = 0;
int width = 800;
int height = 600;
char keyFunction;

GLuint loc1;
GLuint loc2;
GLfloat rotatex = 0.0f;
GLfloat rotatey = 0.0f;
GLfloat rotatez = 0.0f;
GLfloat translatex = 0.0f;
GLfloat translatey = 0.0f;
GLfloat scalex = 1.0f;
GLfloat scaley = 1.0f;
GLfloat scalez = 1.0f;
vec3 camerapos = vec3(0.0f, 3.0f, 3.0f);
vec3 camerafront = vec3(0.0f, 0.0f, -0.1f);
vec3 cameratarget = camerafront + camerapos;
vec3 cameraup = vec3(0.0f, 1.0f, 0.0f);


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {
	FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "../Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "../Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateObjectBufferTeapot() {
	GLuint vp_vbo = 0;

	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normals");

	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof(float), teapot_vertex_points, GL_STATIC_DRAW);
	GLuint vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof(float), teapot_normals, GL_STATIC_DRAW);

	glGenVertexArrays(1, &teapot_vao);
	glBindVertexArray(teapot_vao);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");
	int isFront_location = glGetUniformLocation(shaderProgramID, "isFront");

	// Hierarchy of Teapots

	glCullFace(GL_FRONT);
	{
		// Root of the Hierarchy
			//mat4 view = identity_mat4();
		mat4 view = look_at(camerapos, cameratarget, cameraup);
		mat4 persp_proj = perspective(75.0, (float)width / (float)height, 0.1, 400.0);
		mat4 local1 = identity_mat4();

		glViewport(0, 0, width, height);
		local1 = rotate_x_deg(local1, rotatex);
		local1 = rotate_y_deg(local1, rotatey);
		local1 = rotate_z_deg(local1, 0);
		local1 = scale(local1, vec3(scalex, scaley, scalez));
		local1 = translate(local1, vec3(translatex, translatey, -60.0f));

		// for the root, we orient it in global space
		mat4 global1 = local1;
		// update uniforms & draw
		glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
		glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global1.m);
		glUniform1i(isFront_location, false);
		glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);


		// child of hierarchy
		mat4 local2 = identity_mat4();
		local2 = rotate_x_deg(local2, rotatez);
		// translation is 15 units in the y direction from the parents coordinate system
		local2 = translate(local2, vec3(-25.0, 5.0, 0.0));
		// global of the child is got by pre-multiplying the local of the child by the global of the parent
		mat4 global2 = global1 * local2;
		// update uniform & draw
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2.m);
		glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);
	}

	glCullFace(GL_BACK);
	{
		// Root of the Hierarchy
		//mat4 view = identity_mat4();
		mat4 view = look_at(camerapos, cameratarget, cameraup);
		mat4 persp_proj = perspective(75.0, (float)width / (float)height, 0.1, 400.0);
		mat4 local1 = identity_mat4();

		glViewport(0, 0, width, height);
		local1 = rotate_x_deg(local1, rotatex);
		local1 = rotate_y_deg(local1, rotatey);
		local1 = rotate_z_deg(local1, 0);
		local1 = scale(local1, vec3(scalex, scaley, scalez));
		local1 = translate(local1, vec3(translatex, translatey, -60.0f));

		// for the root, we orient it in global space
		mat4 global1 = local1;
		// update uniforms & draw
		glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
		glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global1.m);
		glUniform1i(isFront_location, true);
		glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);


		// child of hierarchy
		mat4 local2 = identity_mat4();
		local2 = rotate_x_deg(local2, rotatez);
		// translation is 15 units in the y direction from the parents coordinate system
		local2 = translate(local2, vec3(-25.0, 5.0, 0.0));
		// global of the child is got by pre-multiplying the local of the child by the global of the parent
		mat4 global2 = global1 * local2;
		// update uniform & draw
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2.m);
		glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);
	}

	glutSwapBuffers();
}

void reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}


void updateScene() {

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	rotatez += 0.2f;
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load teapot mesh into a vertex buffer array
	generateObjectBufferTeapot();

}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {

	switch (key) {
		//Translate the base, etc.
	case 'x': translatex += 1.0; break;
	case 'X': translatex -= 1.0; break;
	case 'y': translatey += 1.0; break;
	case 'Y': translatey -= 1.0; break;
	case 'a': rotatey += 1.0; break;
	case 'd': rotatex += 1.0; break;
	case 's': scalex += 0.1; scaley += 0.1; scalez += 0.1; break;
	case 'i': camerapos += vec3(0, 0, -1); break;
	case 'k': camerapos -= vec3(0, 0, -1); break;
	case 'l': camerapos -= vec3(-1, 0, 0); cameratarget += vec3(1, 0, 0); break;
	case 'j':  camerapos += vec3(-1, 0, 0); cameratarget -= vec3(1, 0, 0); break;
	case 'm': cameratarget += vec3(sin(10), 0, cos(10)); break;

	case ' ': translatex = 0.0; translatey = 0.0; rotatex = 0.0; rotatey = 0.0;  
		scalex = 1.0; scaley = 1.0; scalez = 1.0; 
		camerapos = vec3(0.0, 3.0, 3.0); cameratarget = camerapos + camerafront;
		break;

		init();
	}
}



	int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutReshapeFunc(reshape);


	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	glutKeyboardFunc(keypress);
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}

