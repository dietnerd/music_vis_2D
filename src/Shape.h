


#pragma once

#ifndef LAB471_SHAPE_H_INCLUDED
#define LAB471_SHAPE_H_INCLUDED

#include <string>
#include <vector>
#include <memory>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define TEXSIZE 300

class Program;
using namespace glm;

vec3 CalculateNormal(int i, int ai, int bi, int ci, std::vector<float> *posBuf);

class Shape
{
public:
	//stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
	void loadMesh(const std::string &meshName, std::string *mtlName = NULL, unsigned char *(loadimage)(char const *, int *, int *, int *, int) = NULL);
	void init();
	void resize();
	void draw(const std::shared_ptr<Program> prog, bool use_extern_texures);

private:
	int obj_count = 0;
	unsigned int *materialIDs = NULL;
    unsigned int *textureIDs = NULL;
    std::vector<unsigned int> *eleBuf = NULL;
    std::vector<float> *posBuf = NULL;
    std::vector<float> *norBuf = NULL;
    std::vector<float> *texBuf = NULL;


	unsigned int *eleBufID = 0;
	unsigned int *posBufID = 0;
	unsigned int *norBufID = 0;
	unsigned int *texBufID = 0;
	unsigned int *vaoID = 0;
    bool isBound = 0;
};

#endif // LAB471_SHAPE_H_INCLUDED
