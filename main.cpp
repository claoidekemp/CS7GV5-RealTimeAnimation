// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

//GLM library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "maths_funcs.h"


/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME "cylinderblend4.obj"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint shaderProgramID1;


ModelData cylinder;
unsigned int body_vao = 0;

mat4 initialbodymatrix;
mat4 initialheadmatrix;

mat4 initialleg11matrix;
mat4 initialleg12matrix;
mat4 initialleg13matrix;

mat4 initialleg21matrix;
mat4 initialleg22matrix;
mat4 initialleg23matrix;

mat4 initialleg31matrix;
mat4 initialleg32matrix;
mat4 initialleg33matrix;

mat4 initialleg41matrix;
mat4 initialleg42matrix;
mat4 initialleg43matrix;

mat4 initialtail1matrix;
mat4 initialtail2matrix;
mat4 initialtail3matrix;

int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_x = 0.0f;
GLfloat rotate_y = 0.0f;

vec3 cameraPos = vec3(10.0f, 0.0f, 10.0f);
vec3 cameraTarget = vec3(0.0f, 0.0f, 0.0f);
vec3 up = vec3(0.0f, 1.0f, 0.0f);

//class Bone {
//public:
//	Bone *parent, *leftmostChild, *rightSibling;
//	string BoneName;
//	Bone() {}
//	Bone(string BoneName);
//};

//Bone::Bone(string BoneName) {
//	this->BoneName = BoneName;
//	this->parent = NULL;
//	this->leftmostChild = NULL;
//	this->rightSibling = NULL;
//}

//class Skeleton {
//public:
//	Bone root;
//	int totalbones;
//	Skeleton(Bone root);
//	static int NumberBones(Bone root);
//};

//Skeleton::Skeleton(Bone root) {
//	this->root = root;
//}

//int total = 0;
//int Skeleton::NumberBones(Bone root) {
//	if (&root == NULL) {
//		return 0;
//	}
	
//	if (&root != NULL) {
//		total += 1;
//		if (&(root.leftmostChild) != NULL) {
//			total += NumberBones(*root.leftmostChild);
//		}
//		else{
//			if (&(root.rightSibling) == NULL) {
//				return total;
//			}
//			else {
//				total += NumberBones(*root.rightSibling);
//			}
//		}
//	}
//	return total;
//}


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

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
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
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
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(const char* pVertexShaderText, const char* pFragmentShaderText)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, pVertexShaderText, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, pFragmentShaderText, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
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
GLuint generateObjectBufferMesh( ModelData mesh_data, GLuint ShaderID) {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	
	unsigned int vp_vbo = 0;
	
	loc1 = glGetAttribLocation(ShaderID, "vertex_position");
	loc2 = glGetAttribLocation(ShaderID, "vertex_normal");
	loc3 = glGetAttribLocation(ShaderID, "vertex_texture");
	
	
	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	//	unsigned int vt_vbo = 0;
	//	glGenBuffers (1, &vt_vbo);
	//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	//	glBufferData (GL_ARRAY_BUFFER, monkey_head_data.mTextureCoords * sizeof (vec2), &monkey_head_data.mTextureCoords[0], GL_STATIC_DRAW);

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	//	glEnableVertexAttribArray (loc3);
	//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	//	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL
	return vao;
}
#pragma endregion VBO_FUNCTIONS

void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID1);


	int matrix_location1 = glGetUniformLocation(shaderProgramID1, "model");
	int view_mat_location1 = glGetUniformLocation(shaderProgramID1, "view");
	int proj_mat_location1 = glGetUniformLocation(shaderProgramID1, "proj");


	mat4 view1 = identity_mat4();
	view1 = look_at(cameraPos, cameraTarget, up);
	mat4 persp_proj1 = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.f);
	mat4 body = initialbodymatrix;

	glUniformMatrix4fv(proj_mat_location1, 1, GL_FALSE, persp_proj1.m);
	glUniformMatrix4fv(view_mat_location1, 1, GL_FALSE, view1.m);
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, body.m);

	glBindVertexArray(body_vao);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	///////////////////////
	// Set up the head child matrix
	mat4 head = initialheadmatrix;
	head = rotate_x_deg(head, rotate_x);
	// Apply the root matrix to the child matrix
	head = body * head;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, head.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);


	///////////////////////
	// Set up the leg matrix
	mat4 leg11 = initialleg11matrix;
	// Apply the root matrix to the child matrix
	leg11 = body * leg11;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg11.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	// Set up the leg matrix
	mat4 leg21 = initialleg21matrix;
	// Apply the root matrix to the child matrix
	leg21 = body * leg21;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg21.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	// Set up the leg matrix
	mat4 leg31 = initialleg31matrix;
	// Apply the root matrix to the child matrix
	leg31 = body * leg31;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg31.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	// Set up the leg matrix
	mat4 leg41 = initialleg41matrix;
	// Apply the root matrix to the child matrix
	leg41 = body * leg41;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg41.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	////////////////
	mat4 leg12 = initialleg12matrix;
	// Apply the root matrix to the child matrix
	leg12 = leg11 * leg12;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg12.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	mat4 leg22 = initialleg22matrix;
	// Apply the root matrix to the child matrix
	leg22 = leg21 * leg22;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg22.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	mat4 leg32 = initialleg32matrix;
	// Apply the root matrix to the child matrix
	leg32 = leg31 * leg32;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg32.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	mat4 leg42 = initialleg42matrix;
	// Apply the root matrix to the child matrix
	leg42 = leg41 * leg42;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg42.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);
	
	////////////////////////////
	mat4 leg13 = initialleg13matrix;
	// Apply the root matrix to the child matrix
	leg13 = leg12 * leg13;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg13.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	mat4 leg23 = initialleg23matrix;
	// Apply the root matrix to the child matrix
	leg23 = leg22 * leg23;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg23.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	mat4 leg33 = initialleg33matrix;
	// Apply the root matrix to the child matrix
	leg33 = leg32 * leg33;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg33.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	mat4 leg43 = initialleg43matrix;
	// Apply the root matrix to the child matrix
	leg43 = leg42 * leg43;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, leg43.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);


	/////////////////////////
	// Set up the tail child matrix
	mat4 tail1 = initialtail1matrix;
	tail1 = translate(tail1, vec3(0.0f, 0.0f, 0.0f));
	tail1 = rotate_x_deg(tail1, rotate_y);
	tail1 = translate(tail1, vec3(0.25f, 1.5f, 0.0f));
	// Apply the root matrix to the child matrix
	tail1 = body * tail1;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, tail1.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	mat4 tail2 = initialtail2matrix;
	// Apply the root matrix to the child matrix
	tail2 = tail1 * tail2;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, tail2.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);

	mat4 tail3 = initialtail3matrix;
	// Apply the root matrix to the child matrix
	tail3 = tail2 * tail3;
	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, tail3.m);
	glDrawArrays(GL_TRIANGLES, 0, cylinder.mPointCount);


	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	//rotate_y += 20.0f * delta;
	//rotate_y = fmodf(rotate_y, 360.0f);

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	cylinder = load_mesh(MESH_NAME);

	// Set up the shaders
	shaderProgramID1 = CompileShaders("toonVertexShader.txt", "toonFragmentShader.txt");
	body_vao = generateObjectBufferMesh(cylinder, shaderProgramID1);

	//Bone* body_bone = new Bone("body");
	//Bone* head_bone = new Bone("head");
	//Bone* leg_upper_bone = new Bone("upperleg");
	//Bone* leg_lower_bone = new Bone("lowerleg");
	//Bone* foot_bone = new Bone("foot");
	//Bone* tail_base_bone = new Bone("tailbase");
	//Bone* tail_middle_bone = new Bone("tailmiddle");
	//Bone* tail_end_bone = new Bone("tailend");

	//body_bone->leftmostChild = head_bone;
	//head_bone->parent = body_bone;
	//head_bone->rightSibling = leg_upper_bone;

	//leg_upper_bone->parent = body_bone;
	//leg_upper_bone->leftmostChild = leg_lower_bone;
	//leg_upper_bone->rightSibling = tail_base_bone;

	//leg_lower_bone->parent = leg_upper_bone;
	//leg_lower_bone->leftmostChild = foot_bone;

	//foot_bone->parent = leg_lower_bone;

	//tail_base_bone->parent = body_bone;
	//tail_base_bone->leftmostChild = tail_middle_bone;
	
	//tail_end_bone->parent = tail_middle_bone;
	
	//Skeleton* dog = new Skeleton(*body_bone);

	//issue with calling this transverse function
	//int number = Skeleton::NumberBones(dog->root);

	//cout << number << endl;
	

	initialbodymatrix = identity_mat4();
	initialbodymatrix = rotate_z_deg(initialbodymatrix, 90);
	initialbodymatrix = scale(initialbodymatrix, vec3(1.5, 1.5, 1.5));
	initialbodymatrix = translate(initialbodymatrix, vec3(1.5, 0.0f, 0.0f));

	initialheadmatrix = identity_mat4();
	initialheadmatrix = scale(initialheadmatrix, vec3(0.75, 0.5, 1.0));
	initialheadmatrix = translate(initialheadmatrix, vec3(0.5f, -0.5f, 0.0f));

	initialleg11matrix = identity_mat4();
	initialleg11matrix = scale(initialleg11matrix, vec3(0.3, 0.5, 0.3));
	initialleg11matrix = rotate_z_deg(initialleg11matrix, -120);
	initialleg11matrix = translate(initialleg11matrix, vec3(-0.75f, 0.75f, 0.3f));

	initialleg12matrix = identity_mat4();
	initialleg12matrix = scale(initialleg12matrix, vec3(0.5, 1.5, 1.0));
	initialleg12matrix = rotate_z_deg(initialleg12matrix, -100);
	initialleg12matrix = translate(initialleg12matrix, vec3(-0.3f, 0.1f, 0.0f));

	initialleg13matrix = identity_mat4();
	initialleg13matrix = scale(initialleg13matrix, vec3(0.6, 0.5, 0.75));
	initialleg13matrix = rotate_z_deg(initialleg13matrix, 50);
	initialleg13matrix = translate(initialleg13matrix, vec3(0.0f, 1.5f, 0.0f));

	initialleg21matrix = identity_mat4();
	initialleg21matrix = scale(initialleg21matrix, vec3(0.3, 0.5, 0.3));
	initialleg21matrix = rotate_z_deg(initialleg21matrix, -120);
	initialleg21matrix = translate(initialleg21matrix, vec3(-0.75f, 0.75f, -0.3f));

	initialleg22matrix = identity_mat4();
	initialleg22matrix = scale(initialleg22matrix, vec3(0.5, 1.5, 1.0));
	initialleg22matrix = rotate_z_deg(initialleg22matrix, -100);
	initialleg22matrix = translate(initialleg22matrix, vec3(-0.3f, 0.1f, 0.0f));

	initialleg23matrix = identity_mat4();
	initialleg23matrix = scale(initialleg23matrix, vec3(0.6, 0.5, 0.75));
	initialleg23matrix = rotate_z_deg(initialleg23matrix, 50);
	initialleg23matrix = translate(initialleg23matrix, vec3(0.0f, 1.5f, 0.0f));

	initialleg31matrix = identity_mat4();
	initialleg31matrix = scale(initialleg31matrix, vec3(0.3, 0.5, 0.3));
	initialleg31matrix = rotate_z_deg(initialleg31matrix, -120);
	initialleg31matrix = translate(initialleg31matrix, vec3(-0.75f, 1.75f, 0.3f));

	initialleg32matrix = identity_mat4();
	initialleg32matrix = scale(initialleg32matrix, vec3(0.5, 1.5, 1.0));
	initialleg32matrix = rotate_z_deg(initialleg32matrix, -100);
	initialleg32matrix = translate(initialleg32matrix, vec3(-0.3f, 0.1f, 0.0f));

	initialleg33matrix = identity_mat4();
	initialleg33matrix = scale(initialleg33matrix, vec3(0.6, 0.5, 0.75));
	initialleg33matrix = rotate_z_deg(initialleg33matrix, 50);
	initialleg33matrix = translate(initialleg33matrix, vec3(0.0f, 1.5f, 0.0f));

	initialleg41matrix = identity_mat4();
	initialleg41matrix = scale(initialleg41matrix, vec3(0.3, 0.5, 0.3));
	initialleg41matrix = rotate_z_deg(initialleg41matrix, -120);
	initialleg41matrix = translate(initialleg41matrix, vec3(-0.75f, 1.75f, -0.3f));

	initialleg42matrix = identity_mat4();
	initialleg42matrix = scale(initialleg42matrix, vec3(0.5, 1.5, 1.0));
	initialleg42matrix = rotate_z_deg(initialleg42matrix, -100);
	initialleg42matrix = translate(initialleg42matrix, vec3(-0.3f, 0.1f, 0.0f));

	initialleg43matrix = identity_mat4();
	initialleg43matrix = scale(initialleg43matrix, vec3(0.6, 0.5, 0.75));
	initialleg43matrix = rotate_z_deg(initialleg43matrix, 50);
	initialleg43matrix = translate(initialleg43matrix, vec3(0.0f, 1.5f, 0.0f));

	initialtail1matrix = identity_mat4();
	initialtail1matrix = scale(initialtail1matrix, vec3(0.25, 0.3, 0.25));
	initialtail1matrix = rotate_z_deg(initialtail1matrix, -30);
	//initialtail1matrix = translate(initialtail1matrix, vec3(0.25f, 1.5f, 0.0f));

	initialtail2matrix = identity_mat4();
	initialtail2matrix = scale(initialtail2matrix, vec3(0.5, 1.0, 0.5));
	initialtail2matrix = rotate_z_deg(initialtail2matrix, -80);
	initialtail2matrix = translate(initialtail2matrix, vec3(0.25f, 1.5f, 0.0f));

	initialtail3matrix = identity_mat4();
	initialtail3matrix = scale(initialtail3matrix, vec3(0.5, 0.8, 0.5));
	initialtail3matrix = rotate_z_deg(initialtail3matrix, -30);
	initialtail3matrix = translate(initialtail3matrix, vec3(0.0f, 1.65f, 0.0f));
}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	if (key == 'z') {
		//turnheadleft
		rotate_x = rotate_x-10;
	}
	if (key == 'x') {
		//turnheadright
		rotate_x = rotate_x+10;
	}

	if (key == 'v') {
		//turntailleft
		rotate_y = rotate_y - 20;
	}
	if (key == 'b') {
		//turntailright
		rotate_y = rotate_y + 20;
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
	glutKeyboardFunc(keypress);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
