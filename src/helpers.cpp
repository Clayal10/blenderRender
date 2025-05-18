#define GLM_ENABLE_EXPERIMENTAL

#include<GL/glew.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include "lib/scolor.hpp"
#include "helpers.h"
#include "lib/tiny_obj_loader.h"
#include "lib/stb_image.h"

#define GBLEN 100000
char *general_buffer;

void init_helpers(){
	general_buffer = (char*)malloc(GBLEN);
}

void free_helpers() {
	free(general_buffer);
}

float randvel(float speed){
	long min = -100;
	long max = 100;
	return speed * (min + rand() % (max + 1 - min));
}


std::vector<glm::mat4> gameobject::create_models(){
	std::vector<glm::mat4> models;
	data_mutex.lock();
	models.reserve(locations.size());
	for(glm::vec3 l : locations){
		glm::mat4 new_model = glm::mat4(1.0f);
		new_model = translate(new_model, l);
		models.push_back(new_model);
	}
	data_mutex.unlock();
	return models;
}

int loaded_object::init() {
	// Initialization part
	std::vector<vertex> vertices;
	std::vector<uint32_t> indices; // Consider unified terminology

	load_model(vertices, indices, objectfile, scale, swap_yz, texture_scale);

	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	// TODO:  Remember to explain the layout later

	glGenBuffers(1, &ebuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indices.size(), indices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &models_buffer);

	tex = load_texture(texturefile);

	program = make_program("shaders/loaded_object_vertex_shader.glsl",0, 0, 0, "shaders/loaded_object_fragment_shader.glsl");
	if (!program)
		return 1;

	v_attrib = glGetAttribLocation(program, "in_vertex");
	t_attrib = glGetAttribLocation(program, "in_texcoord");
	mvp_uniform = glGetUniformLocation(program, "vp");
	return 0;
}

void loaded_object::draw(glm::mat4 vp) {
	glUseProgram(program);
	std::vector<glm::mat4> models = create_models();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, models_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, models.size() * sizeof(glm::mat4), models.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, models_buffer);

	glEnableVertexAttribArray(v_attrib);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glVertexAttribPointer(v_attrib, 3, GL_FLOAT, GL_FALSE, 20, 0);

	glEnableVertexAttribArray(t_attrib);
	glVertexAttribPointer(t_attrib, 2, GL_FLOAT, GL_FALSE, 20, (const void*)12);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	int size;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebuf);
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glUniformMatrix4fv(mvp_uniform, 1, 0, glm::value_ptr(vp));

	glDrawElementsInstanced(GL_TRIANGLES, size / sizeof(GLuint), GL_UNSIGNED_INT, 0, models.size());
}

GLuint make_shader(const char* filename, GLenum shaderType) {
	FILE* fd = fopen(filename, "r");
	if (fd == 0) {
		printf("File not found:  %s\n", filename);
		return 0;
	}
	size_t readlen = fread(general_buffer, 1, GBLEN, fd);
	fclose(fd);
	if (readlen == GBLEN) {
		printf(RED("Buffer Length of %d bytes Inadequate for File %s\n").c_str(), GBLEN, filename);
		return 0;
	}
	if (readlen == 0) {
		puts(RED("File read problem, read 0 bytes").c_str());
		return 0;
	}
	general_buffer[readlen] = 0;
	printf(DGREEN("Read shader in file %s (%d bytes)\n").c_str(), filename, readlen);
	puts(general_buffer);
	unsigned int s_reference = glCreateShader(shaderType);
	glShaderSource(s_reference, 1, (const char**)&general_buffer, 0);
	glCompileShader(s_reference);
	glGetShaderInfoLog(s_reference, GBLEN, NULL, general_buffer);
	puts(general_buffer);
	GLint compile_ok;
	glGetShaderiv(s_reference, GL_COMPILE_STATUS, &compile_ok);
	if (compile_ok) {
		puts(GREEN("Compile Success").c_str());
		return s_reference;
	}
	puts(RED("Compile Failed\n").c_str());
	return 0;
}

GLuint make_program(const char* v_file, const char* tcs_file, const char* tes_file, const char* g_file, const char* f_file) {
	unsigned int vs_reference = make_shader(v_file, GL_VERTEX_SHADER);
	unsigned int tcs_reference = 0, tes_reference = 0;
	if (tcs_file)
		if (!(tcs_reference = make_shader(tcs_file, GL_TESS_CONTROL_SHADER)))
			return 0;
	if (tes_file)
		if (!(tes_reference = make_shader(tes_file, GL_TESS_EVALUATION_SHADER)))
			return 0;
	unsigned int gs_reference = 0;
	if (g_file)
		gs_reference = make_shader(g_file, GL_GEOMETRY_SHADER);
	unsigned int fs_reference = make_shader(f_file, GL_FRAGMENT_SHADER);
	if (!(vs_reference && fs_reference))
		return 0;
	if (g_file && !gs_reference)
		return 0;


	unsigned int program = glCreateProgram();
	glAttachShader(program, vs_reference);
	if (g_file)
		glAttachShader(program, gs_reference);
	if (tcs_file)
		glAttachShader(program, tcs_reference);
	if (tes_file)
		glAttachShader(program, tes_reference);
	glAttachShader(program, fs_reference);
	glLinkProgram(program);
	GLint link_ok;
	glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
	if (!link_ok) {
		glGetProgramInfoLog(program, GBLEN, NULL, general_buffer);
		puts(general_buffer);
		puts(RED("Link Failed").c_str());
		return 0;
	}

	return program;
}

unsigned int load_texture(const char* filename){
	int width, height, channels;
	unsigned char *image_data = stbi_load(filename, &width, &height, &channels, 4);
	if(image_data){
		printf("Loaded image %s, %d by %d\n", filename, width, height);
		unsigned int tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		// If you want to set parameters, call glTexParameteri (or similar)
 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		free(image_data);
		printf(YELLOW("Generated texture with ID %d\n").c_str(), tex);
		return tex;
	} else {
		printf(RED("Image failed to load:  %s\n").c_str(), filename);
		return 0;
	}
}

namespace std {
	template<> struct hash<vertex> {
		size_t operator()(vertex const& v) const {
			return hash<glm::vec3>()(v.pos) ^ (hash<glm::vec2>()(v.tex_coord) << 1);
		}
	};
}

int load_model(std::vector<vertex> &vertices, std::vector<uint32_t> &indices, const char *filename, float scale, bool swap_yz, glm::vec2 texture_scale){
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename)){
		printf("Loading failed!  %s\n", err.c_str());
		return 1;
	}

	std::unordered_map<vertex, uint32_t> uniqueVertices = {};

	for(const auto& shape : shapes){
		for(const auto& index : shape.mesh.indices){
			vertex new_vertex = {};
			new_vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]

			};	
			/* Transform vertices if we need to */
			new_vertex.pos *= scale;
			if(swap_yz){
				float tmp = new_vertex.pos.y;
				new_vertex.pos.y = new_vertex.pos.z;
				new_vertex.pos.z = tmp;
			}
			
			new_vertex.tex_coord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};
			new_vertex.tex_coord *= texture_scale;
			if(uniqueVertices.count(new_vertex) == 0){
				uniqueVertices[new_vertex] = (uint32_t)vertices.size();
				vertices.push_back(new_vertex);
			}
			indices.push_back(uniqueVertices[new_vertex]);
		}
	}
	return 0;
}
