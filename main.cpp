#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS


struct object_struct{
	//the saving form of our object like earth 
	unsigned int program;
	unsigned int vao;
	unsigned int vbo[4];
	unsigned int texture;
	glm::mat4 model;
	//struct's constructor
	object_struct(): model(glm::mat4(1.0f)){}
} ;

//the vector that save our created objects
std::vector<object_struct> objects;//vertex array object,vertex buffer object and texture(color) for objs
unsigned int program, program_flat, program_Gouraud, program_phong, program_Binn_Phong;
//the index of a obj to our vertex list
std::vector<int> indicesCount;//Number of indice of objs

static void error_callback(int error, const char* description)
{
	//to print the error message
	fputs(description, stderr);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//if input "ESC" , then this function will close the window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static unsigned int setup_shader(const char *vertex_shader, const char *fragment_shader)
{
	//to create the vertex shader
	GLuint vs=glCreateShader(GL_VERTEX_SHADER);
	//to find our shader like vs.txt and fs.txt
	glShaderSource(vs, 1, (const GLchar**)&vertex_shader, nullptr);

	glCompileShader(vs);

	int status, maxLength;
	char *infoLog=nullptr;
	//to check the result of compiling the shader
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE)
	{
		//if the compilation is failed, then get and print the error message
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(vs, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Vertex Shader Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}

	// similar to mentioned above
	GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, (const GLchar**)&fragment_shader, nullptr);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE)
	{
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(fs, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Fragment Shader Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}

	//Both vertex shader and fragment shader are success
	//to create a program for shader
	unsigned int program=glCreateProgram();
	// Attach our shaders to our program
	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	//to check the result of linking the shader
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if(status==GL_FALSE)
	{
		//if the linking is failed, then get and print the error message
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);


		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];
		
		//why this line????
		glGetProgramInfoLog(program, maxLength, NULL, infoLog);

		glGetProgramInfoLog(program, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Link Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}
	return program;
}

static std::string readfile(const char *filename)
{
	std::ifstream ifs(filename);
	if(!ifs)
		exit(EXIT_FAILURE);
	
	/*
	string  (InputIterator first, InputIterator last);
	Copies the sequence of characters in the range [first,last), in the same order.
	The following code will return a string which has all chars in the ifs
	
	this function is used to read shaders
	*/
	
	return std::string( (std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>()));
}

// mini bmp loader written by HSU YOU-LUN
static unsigned char *load_bmp(const char *bmp, unsigned int *width, unsigned int *height, unsigned short int *bits)
{
	unsigned char *result=nullptr;
	FILE *fp = fopen(bmp, "rb");
	if(!fp)
		return nullptr;
	char type[2];
	unsigned int size, offset;
	// check for magic signature	
	//size_t fread ( void * ptr, size_t size, size_t count, FILE * stream );
	//to read infomation from "fp" to "type"
	fread(type, sizeof(type), 1, fp);
	if(type[0]==0x42 || type[1]==0x4d){
		fread(&size, sizeof(size), 1, fp);
		// ignore 2 two-byte reversed fields
		fseek(fp, 4, SEEK_CUR);
		fread(&offset, sizeof(offset), 1, fp);
		// ignore size of bmpinfoheader field
		fseek(fp, 4, SEEK_CUR);
		fread(width, sizeof(*width), 1, fp);
		fread(height, sizeof(*height), 1, fp);
		// ignore planes field
		fseek(fp, 2, SEEK_CUR);
		fread(bits, sizeof(*bits), 1, fp);
		unsigned char *pos = result = new unsigned char[size-offset];
		fseek(fp, offset, SEEK_SET);
		while(size-ftell(fp)>0)
			pos+=fread(pos, 1, size-ftell(fp), fp);
	}
	fclose(fp);
	return result;
}

static int add_obj(unsigned int program, const char *filename,const char *texbmp)
{
	//to add new obj like earth
	object_struct new_node;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	// to load our obj
	std::string err = tinyobj::LoadObj(shapes, materials, filename);

	if (!err.empty()||shapes.size()==0)
	{
		std::cerr<<err<<std::endl;
		exit(1);
	}

	//to generate the VAO
	glGenVertexArrays(1, &new_node.vao);
	//to generate the VBOs
	glGenBuffers(4, new_node.vbo);
	
	glGenTextures(1, &new_node.texture);

	//to start VAO to store(point to) the infomation of VBO
	glBindVertexArray(new_node.vao);

	// Upload postion array
	
	//to bind the vbo[0] to GL_ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[0]);
	//1st argument is the destination of our copying data, 2nd argument is the size of our copying data, 3rd argument is our copying data, and 4th argument is our management of this data
	//to send our obj infomation to vbo[0](GL_ARRAY_BUFFER)
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.positions.size(),shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	//to enable the vertex attribut buffer (location = 0)
	glEnableVertexAttribArray(0);
	//1st argument is the vertex attribute we want to set (by location), 2nd argument depends on how much data we want to give to the vertex attribute, 3rd argument is the type of our data,
	//4th argument is that we want to normalize our data or not, 5th argument is the stride , and 6th argument is offset
	//if stride is set to 0 , it means that the stride will be determined by OpenGL 
	//to give the data to position in vertex shader (location = 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	if(shapes[0].mesh.texcoords.size()>0)
	{
		// similar to mentioned above
		// Upload texCoord array
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.texcoords.size(),
				shapes[0].mesh.texcoords.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		//to bind the texture
		glBindTexture( GL_TEXTURE_2D, new_node.texture);
		unsigned int width, height;
		unsigned short int bits;
		unsigned char *bgr=load_bmp(texbmp, &width, &height, &bits);
		GLenum format = (bits == 24? GL_BGR: GL_BGRA);
		//to generate texture from "bgr" to new_node.texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, bgr);
		//some setting to texture
		//the texture filter is linear filtering when magnifying
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//the texture mipmaps take the nearest mipmap level and samples using linear interpolation when minifying
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		//to generate all the required mipmaps for the texture
		glGenerateMipmap(GL_TEXTURE_2D);
		delete [] bgr;
	}

	if(shapes[0].mesh.normals.size()>0)
	{
		// similar to mentioned above
		// Upload normal array
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.normals.size(),
				shapes[0].mesh.normals.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// Setup index buffer for glDrawElements
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_node.vbo[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*shapes[0].mesh.indices.size(),
			shapes[0].mesh.indices.data(), GL_STATIC_DRAW);

	indicesCount.push_back(shapes[0].mesh.indices.size());

	//to close the vao
	glBindVertexArray(0);

	//to set the shader to new_node
	new_node.program = program;

	objects.push_back(new_node);
	return objects.size()-1;
}

static void releaseObjects()
{
	//to release all objs
	for(int i=0;i<objects.size();i++){
		glDeleteVertexArrays(1, &objects[i].vao);
		glDeleteTextures(1, &objects[i].texture);
		glDeleteBuffers(4, objects[i].vbo);
	}
	//to release the shader
	glDeleteProgram(program);
}

static void setUniformMat4(unsigned int program, const std::string &name, const glm::mat4 &mat)
{
	// This line can be ignore. But, if you have multiple shader program
	// You must check if currect binding is the one you want
	glUseProgram(program);
	//name.c_str() means name + '\0'(null-character)
	GLint loc=glGetUniformLocation(program, name.c_str());
	if(loc==-1) return;

	// mat4 of glm is column major, same as opengl
	// we don't need to transpose it. so..GL_FALSE
	//to set the uniform in shader
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

static void render()
{
	//to clear the buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//to render each obj in objects
	
	/*
	for(int i=0;i<objects.size();i++){
	
		glUseProgram(objects[i].program);
		glBindVertexArray(objects[i].vao);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture);
		//you should send some data to shader here
			
		if(i == 0)
		{
			//sun
			//to stay at origin
			setUniformMat4(objects[i].program, "model", glm::mat4());
		}
	
		if(i == 1)
		{
			//earth
			//oval revolution, rotation
			setUniformMat4(objects[i].program, "model", glm::translate(glm::mat4(1.0f),glm::vec3(10.0f * cos((GLfloat)glfwGetTime()),0.0f, 15.0f * sin((GLfloat)glfwGetTime()))  ) * glm::rotate(glm::mat4(1.0f), (GLfloat)glfwGetTime() * 10.0f, glm::vec3(0.0f,1.0f,0.0f)) * glm::scale(glm::mat4(1.0f),glm::vec3(2.0f,2.0f,2.0f)));
		}
		
		//1st argument is the draw mode, 2nd argument is the number of indices, 3rd argument is the type of 2nd, 4th argument is offset
		//to draw
		glDrawElements(GL_TRIANGLES, indicesCount[i], GL_UNSIGNED_INT, nullptr);
	}
	*/
	
	//Gouraud shading
	//left bottom
	
	glUseProgram(objects[1].program);
	glBindVertexArray(objects[1].vao);
	glBindTexture(GL_TEXTURE_2D, objects[1].texture);
	
	setUniformMat4(objects[1].program, "model", glm::translate(glm::mat4(),glm::vec3(-5.0f, -2.0f, 10.0f)));
	
	glDrawElements(GL_TRIANGLES, indicesCount[1], GL_UNSIGNED_INT, nullptr);
	
	
	//phong shading
	//right top
	glUseProgram(objects[2].program);
	glBindVertexArray(objects[2].vao);
	glBindTexture(GL_TEXTURE_2D, objects[2].texture);
	
	setUniformMat4(objects[2].program, "model", glm::translate(glm::mat4(1.0f),glm::vec3(10.0f, 8.0f, -1.0f)));
	
	glDrawElements(GL_TRIANGLES, indicesCount[2], GL_UNSIGNED_INT, nullptr);
	
	//Blinn–Phong shading
	//right bottom
	glUseProgram(objects[3].program);
	glBindVertexArray(objects[3].vao);
	glBindTexture(GL_TEXTURE_2D, objects[3].texture);
	
	setUniformMat4(objects[3].program, "model", glm::translate(glm::mat4(1.0f),glm::vec3(8.0f, -2.0f, 5.0f)));
	
	glDrawElements(GL_TRIANGLES, indicesCount[3], GL_UNSIGNED_INT, nullptr);
	
	//flat shading
	//left top
	
	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
	
	glUseProgram(objects[0].program);
	glBindVertexArray(objects[0].vao);
	glBindTexture(GL_TEXTURE_2D, objects[0].texture);
	
	setUniformMat4(objects[0].program, "model", glm::translate(glm::mat4(),glm::vec3(-5.0f, 8.0f, 5.0f)));
	
	glDrawElements(GL_TRIANGLES, indicesCount[0], GL_UNSIGNED_INT, nullptr);
	
	//to close the vao
	glBindVertexArray(0);
}

int main(int argc, char *argv[])
{
	GLFWwindow* window;
	//to set error function
	glfwSetErrorCallback(error_callback);
	//initialization
	if (!glfwInit())
		exit(EXIT_FAILURE);
	// OpenGL 3.3, Mac OS X is reported to have some problem. However I don't have Mac to test
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// For Mac OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//to create the window
	window = glfwCreateWindow(800, 600, "Simple Example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);

	// This line MUST put below glfwMakeContextCurrent
	glewExperimental = GL_TRUE;
	//initialization
	glewInit();

	// Enable vsync
	glfwSwapInterval(1);

	// Setup input callback
	glfwSetKeyCallback(window, key_callback);

	// load shader program
	program = setup_shader(readfile("vs.txt").c_str(), readfile("fs.txt").c_str());
	program_flat = setup_shader(readfile("flat_vs.txt").c_str(), readfile("flat_fs.txt").c_str());
	program_Gouraud = setup_shader(readfile("Gouraud_vs.txt").c_str(), readfile("Gouraud_fs.txt").c_str());
	program_phong = setup_shader(readfile("phong_vs.txt").c_str(), readfile("phong_fs.txt").c_str());
	program_Binn_Phong = setup_shader(readfile("Binn_Phong_vs.txt").c_str(), readfile("Binn_Phong_fs.txt").c_str());
	
	//to create the sun in diffrent shaders
	int sun_flat = add_obj(program_flat, "sun.obj","sun.bmp");
	int sun_Gouraud = add_obj(program_Gouraud, "sun.obj","sun.bmp");
	int sun_phong = add_obj(program_phong, "sun.obj","sun.bmp");
	int sun_Binn_Phong = add_obj(program_Binn_Phong, "sun.obj","sun.bmp");
	
	//int sun = add_obj(program, "sun.obj","sun.bmp");
	//int earth = add_obj(program2, "earth.obj","earth.bmp");

	//to enable z buffer
	glEnable(GL_DEPTH_TEST);
	 glCullFace(GL_BACK);
	// Enable blend mode for billboard
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//to set uniform vp
	setUniformMat4(program, "vp", glm::perspective(glm::radians(45.0f), 640.0f/480, 1.0f, 100.f)*
			glm::lookAt(glm::vec3(20.0f), glm::vec3(), glm::vec3(0, 1, 0))*glm::mat4(1.0f));
			
	setUniformMat4(program_flat, "vp", glm::perspective(glm::radians(45.0f), 640.0f/480, 1.0f, 100.f)*
			glm::lookAt(glm::vec3(20.0f), glm::vec3(), glm::vec3(0, 1, 0))*glm::mat4(1.0f));
			
	setUniformMat4(program_Gouraud, "vp", glm::perspective(glm::radians(45.0f), 640.0f/480, 1.0f, 100.f)*
			glm::lookAt(glm::vec3(20.0f), glm::vec3(), glm::vec3(0, 1, 0))*glm::mat4(1.0f));
			
	setUniformMat4(program_phong, "vp", glm::perspective(glm::radians(45.0f), 640.0f/480, 1.0f, 100.f)*
			glm::lookAt(glm::vec3(20.0f), glm::vec3(), glm::vec3(0, 1, 0))*glm::mat4(1.0f));
			
	setUniformMat4(program_Binn_Phong, "vp", glm::perspective(glm::radians(45.0f), 640.0f/480, 1.0f, 100.f)*
			glm::lookAt(glm::vec3(20.0f), glm::vec3(), glm::vec3(0, 1, 0))*glm::mat4(1.0f));
			
	//setUniformMat4(program2, "vp", glm::mat4(1.0));
	
	//glm::mat4 tl=glm::translate(glm::mat4(),glm::vec3(15.0f,0.0f,0.0));
	//glm::mat4 rot;
	//glm::mat4 rev;
	
	//translate + revolution
	glm::mat4 tl_rev = glm::translate(glm::mat4(1.0f),glm::vec3(10.0f * cos((GLfloat)glfwGetTime()),0.0f, 15.0f * sin((GLfloat)glfwGetTime()))  );
	//rotate
	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (GLfloat)glfwGetTime() * 10.0f, glm::vec3(0.0f,1.0f,0.0f));

	float last, start;
	//initialization
	last = start = glfwGetTime();
	//frames per second
	int fps=0;
	objects[sun_flat].model = glm::scale(glm::mat4(1.0f), glm::vec3(0.85f));
	objects[sun_Gouraud].model = glm::scale(glm::mat4(1.0f), glm::vec3(0.85f));
	objects[sun_phong].model = glm::scale(glm::mat4(1.0f), glm::vec3(0.85f));
	objects[sun_Binn_Phong].model = glm::scale(glm::mat4(1.0f), glm::vec3(0.85f));
	
	while (!glfwWindowShouldClose(window))
	{//program will keep draw here until you close the window
		//to calculate the fps
		float delta = glfwGetTime() - start;
		render();
		glfwSwapBuffers(window);
		glfwPollEvents();
		fps++;
		if(glfwGetTime() - last > 1.0)
		{
			//to print fps
			std::cout<<(double)fps/(glfwGetTime()-last)<<std::endl;
			fps = 0;
			last = glfwGetTime();
		}
	}

	//to release and close all the thing
	releaseObjects();
	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}
