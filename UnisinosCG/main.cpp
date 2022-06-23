
#define _CRT_SECURE_NO_WARNINGS
//#define STB_IMAGE_IMPLEMENTATION
#include "gl_utils.cpp"
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <time.h>
#define GL_LOG_FILE "gl.log"
#include <iostream>
#include <vector>
#include "TileMap.h"
#include "DiamondView.h"
#include "SlideView.h"
#include "ltMath.h"
#include <fstream>

using namespace std;


/* Command line build:
  g++ -framework Cocoa -framework OpenGL -framework IOKit -o demoIsom gl_utils.cpp maths_funcs.cpp stb_image.cpp _isometrico.cpp  -I include -I/sw/include -I/usr/local/include -I ../common/include ../common/osx_64/libGLEW.a ../common/osx_64/libglfw3.a
 */

using namespace std;

int g_gl_width = 800;
int g_gl_height = 800;
float xi = -1.0f;
float xf = 1.0f;
float yi = -1.0f;
float yf = 1.0f;
float w = xf - xi;
float h = yf - yi;
float tw, th, tw2, th2;
int tileSetCols = 9, tileSetRows = 9;
float tileW, tileW2;
float tileH, tileH2;
int cx = -1, cy = -1;

TilemapView* tview = new DiamondView();
TileMap* tmap = NULL;

GLFWwindow* g_window = NULL;

TileMap* readMap(char* filename) {
	ifstream arq(filename);
	int w, h;
	arq >> w >> h;
	TileMap* tmap = new TileMap(w, h, 0);
	for (int r = 0; r < h; r++) {
		for (int c = 0; c < w; c++) {
			int tid;
			arq >> tid;
			cout << tid << " ";
			tmap->setTile(c, h - r - 1, tid);
		}
		cout << endl;
	}
	arq.close();
	return tmap;
}

int loadTexture(unsigned int& texture, char* filename)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;

	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		if (nrChannels == 4)
		{
			cout << "Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			cout << "Without Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	return 1;
}

void SRD2SRU(double& mx, double& my, float& x, float& y) {
	x = xi + (mx / g_gl_width) * w;
	y = yi + (1 - (my / g_gl_height)) * h;
}

void mouse(double& mx, double& my) {

	// cout << "DEBUG => mouse click" << endl;

	// 1) Definição do tile candidato ao clique
	float y = 0;
	float x = 0;
	SRD2SRU(mx, my, x, y);

	int c, r;
	tview->computeMouseMap(c, r, tw, th, x, y);
	c += (tmap->getWidth() - 1) / 2;
	r += (tmap->getHeight() - 1) / 2;
	// cout << "\tDEBUG => r: " << r << " c: " << c << endl;

	// 2) Verificar se o ponto pertence ao tile indicado:

	// 2.1) Normalização do clique:
	float x0, y0;
	tview->computeDrawPosition(c, r, tw, th, x0, y0);
	x0 += xi;

	// cout << "\tDEBUG => mx: " << x  << " my: " << y  << endl;
	// cout << "\tDEBUG => x0: " << x0 << " y0: " << y0 << endl;
	// cout << "\tDEBUG => tw: " << tw << endl;

	float point[] = { x, y };

	// 2.2) Verifica se o ponto está dentro do triângulo da esquerda ou da direita do losangulo (metades)
	//      Implementação via cálculo de área dos triangulos: area(ABC) == area(ABp)+area(ACp)+area(BCp)
	// triangulo ABC:
	float* abc = new float[6];

	// 2.2.1) Define metade da esquerda ou da direita
	bool left = x < (x0 + tw / 2.0f);
	// cout << "\tDEBUG => mx: " << x << " midx: " << (x0 + tw/2.0f) << endl; 

	if (left) { // left
		abc[0] = x0;           abc[1] = y0 + th / 2.0f;
		abc[2] = x0 + tw / 2.0f; abc[3] = y0 + th;
		abc[4] = x0 + tw / 2.0f; abc[5] = y0;
		// cout << "DEBUG => TRG LFT [(x,y),...] = ([" << abc[0] << "," << abc[1] << "], "
		// << "[" << abc[2] << "," << abc[3] << "], "
		// << "[" << abc[4] << "," << abc[5] << "])" << endl;
	}
	else { // right
		abc[0] = x0 + tw / 2.0f; abc[1] = y0;
		abc[2] = x0 + tw / 2.0f; abc[3] = y0 + th;
		abc[4] = x0 + tw;      abc[5] = y0 + th / 2.0f;
		// cout << "DEBUG => TRG RGHT [(x,y),...] = ([" << abc[0] << "," << abc[1] << "], "
		// << "[" << abc[2] << "," << abc[3] << "], "
		// << "[" << abc[4] << "," << abc[5] << "])" << endl;
	}

	// 2.3) Calcular colisão do ponto com o triangulo
	bool collide = triangleCollidePoint2D(abc, point);

	if (!collide) {
		// 2.4) Em caso "erro" de cálculo, deve ser feito o tileWalking para tile certo!
		cout << "tileWalking " << endl;
		if (left) {
			tview->computeTileWalking(c, r, DIRECTION_WEST);
		}
		else {
			tview->computeTileWalking(c, r, DIRECTION_EAST);
		}
	}

	if ((c < 0) || (c >= tmap->getWidth()) || (r < 0) || (r >= tmap->getHeight())) {
		cout << "wrong click position: " << c << ", " << r << endl;
		return; // posição inválida!
	}

	cout << "SELECIONADO c=" << c << "," << r << endl;
	cx = c; cy = r;
}

int main()
{
#pragma region inicialização do OpenGL
	restart_gl_log();
	// all the GLFW and GLEW start-up code is moved to here in gl_utils.cpp
	start_gl();
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS);
#pragma endregion

#pragma region carregamento do tmap
	cout << "Tentando criar tmap" << endl;
	tmap = readMap("terrain1.tmap");
	tw = w / (float)tmap->getWidth();
	th = tw / 2.0f;
	tw2 = th;
	th2 = th / 2.0f;
	tileW = 1.0f / (float)tileSetCols;
	tileW2 = tileW / 2.0f;
	tileH = 1.0f / (float)tileSetRows;
	tileH2 = tileH / 2.0f;

	cout << "tw=" << tw << " th=" << th << " tw2=" << tw2 << " th2=" << th2
		<< " tileW=" << tileW << " tileH=" << tileH
		<< " tileW2=" << tileW2 << " tileH2=" << tileH2
		<< endl;
#pragma endregion

#pragma region carregamento de texturas e associação com tmap
	// cenario
	GLuint tid;
	loadTexture(tid, "images/terrain.png");


	tmap->setTid(tid);
	cout << "Tmap inicializado" << endl;

	unsigned int texturaObjeto;
	glGenTextures(1, &texturaObjeto);
	glBindTexture(GL_TEXTURE_2D, texturaObjeto);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;

	// unsigned char *data = stbi_load("spritesheet-muybridge.jpg", &width, &height, &nrChannels, 0);
	//unsigned char* data = stbi_load("spritesheet-muybridge.png", &width, &height, &nrChannels, 0);
	// MAPEAMENTO PARA SULLY
	unsigned char* data = stbi_load("images/sully.png", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		// MAPEAMENTO PARA SULLY
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
#pragma endregion

#pragma region vértices
	float verticesCenario[] = {
		// positions   // texture coords
		xi    , yi + th2, 0.0f, tileH2,   // left
		xi + tw2, yi    , tileW2, 0.0f,   // bottom
		xi + tw , yi + th2, tileW, tileH2,  // right
		xi + tw2, yi + th , tileW2, tileH,  // top
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		3, 1, 2  // second triangle
	};

	float verticesObjeto[] = {
		 0.5f, 0.5f, 0.25f, 0.25f, // top right
		 0.5f, -0.5f, 0.25f, 0.0f, // bottom right
		 -0.5f, -0.5f, 0.0f, 0.0f, // bottom left
		 -0.5f, 0.5f, 0.0f, 0.25f, // top left
	};
#pragma endregion

#pragma region passagem dados para GPU
	unsigned int VBOCenario, VBOObjeto, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBOCenario);
	glGenBuffers(1, &VBOObjeto);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	// cenário
	glBindBuffer(GL_ARRAY_BUFFER, VBOCenario);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCenario), verticesCenario, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// objeto
	glBindBuffer(GL_ARRAY_BUFFER, VBOObjeto);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesObjeto), verticesObjeto, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);
	// texture coord attribute
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
#pragma endregion

#pragma region shaders
	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];
	parse_file_into_str("_geral_vs.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("_geral_fs.glsl", fragment_shader, 1024 * 256);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* p = (const GLchar*)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);
#pragma endregion

#pragma region  check for compile errors
	// check for compile errors
	int params = -1;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		print_shader_info_log(vs);
		return 1; // or exit or something
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar*)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);

	// check for compile errors
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		print_shader_info_log(fs);
		return 1; // or exit or something
	}
#pragma endregion

#pragma region cria programa e linka os shaders com o shader program
	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: could not link shader programme GL index %i\n",
			shader_programme);
		// 		print_programme_info_log( shader_programme );
		return false;
	}
#pragma endregion
	for (int r = 0; r < tmap->getHeight(); r++) {
		for (int c = 0; c < tmap->getWidth(); c++) {
			unsigned char t_id = tmap->getTile(c, r);
			cout << ((int)t_id) << " ";
		}
		cout << endl;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glEnable(GL_DEPTH_TEST);

#pragma region inicialização das variáveis
	float fw = 0.25f;
	float fh = 0.25f;
	float offsetx = 0, offsety = 0;
	int frameAtual = 0;
	int acao = 3;
	int sign = 1;
	float previous = glfwGetTime();
#pragma endregion

	while (!glfwWindowShouldClose(g_window))
	{
		_update_fps_counter(g_window);
		double current_seconds = glfwGetTime();

#pragma region limpa a tela
		// wipe the drawing surface clear
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// glClear(GL_COLOR_BUFFER_BIT);
#pragma endregion

		glViewport(0, 0, g_gl_width, g_gl_height);

#pragma region define programa
		glUseProgram(shader_programme);
#pragma endregion

#pragma region bind da geometria
		glBindVertexArray(VAO);
#pragma endregion

#pragma region passagem de informações aos shaders e desenho do cenario
		glBindBuffer(GL_ARRAY_BUFFER, VBOCenario);
		glUniform1f(glGetUniformLocation(shader_programme, "isObject"), false);

		//para cada linha e columa da matriz é calculada a posição de desenho chamando computeDrawPosition
		float x, y;
		int r = 0, c = 0;
		for (int r = 0; r < tmap->getHeight(); r++) {
			for (int c = 0; c < tmap->getWidth(); c++) {
				//t_id: nº do tile na matriz usado para calcular a porção da textura que deve ser apresentada nesse tile
				int t_id = (int)tmap->getTile(c, r);
				int u = t_id % tileSetCols;
				int v = t_id / tileSetCols;

				tview->computeDrawPosition(c, r, tw, th, x, y);

				glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), u * tileW);
				glUniform1f(glGetUniformLocation(shader_programme, "offsety"), v * tileH);
				glUniform1f(glGetUniformLocation(shader_programme, "tx"), x);
				glUniform1f(glGetUniformLocation(shader_programme, "ty"), y + 1.0);
				glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), 0.50);
		
				//adiciona cor ao tile selecionado
				glUniform1f(glGetUniformLocation(shader_programme, "weight"), (c == cx) && (r == cy) ? 0.5 : 0.0);

				// bind Texture
				// glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, tmap->getTileSet());
				glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}
#pragma endregion

#pragma region Objeto
		glBindBuffer(GL_ARRAY_BUFFER, VBOObjeto);
		glUniform1f(glGetUniformLocation(shader_programme, "isObject"), true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texturaObjeto);
		glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);

		glUseProgram(shader_programme);
		glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), offsetx);
		glUniform1f(glGetUniformLocation(shader_programme, "offsety"), offsety);
		glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), 0.40);

		if ((current_seconds - previous) > (0.16))
		{
			previous = current_seconds;

			// CALCULA TROCA DE LINHA
			if (frameAtual == 3)
			{
				acao = (4 + (acao - 1)) % 4;
				frameAtual = 0;
			}
			else
			{
				frameAtual = (frameAtual + 1) % 4;
			}

			offsetx = fw * (float)frameAtual;
			offsety = fh * (float)acao;
		}
		glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
#pragma endregion

#pragma region Eventos
		glfwPollEvents();
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(g_window, 1);
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_UP))
		{
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_DOWN))
		{
		}
		double mx, my;
		glfwGetCursorPos(g_window, &mx, &my);

		const int state = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT);

		if (state == GLFW_PRESS) {
			mouse(mx, my);
		}
#pragma endregion

		// put the stuff we've been drawing onto the display
		glfwSwapBuffers(g_window);
	}

	// close GL context and any other GLFW resources
	glfwTerminate();
	delete tmap;
	return 0;
}

