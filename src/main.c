// HighLife
// 2016-2019, dibi
// see LICENSE for legal infos.

#define GLFW_INCLUDE_GLU

#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glu.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <string.h>

#define FDEC 0.3f
#define FCOEF 0.95f
#define TESTINTERVAL 500

// simple macros
#define RX(X,I) ((X)+(I)<0?(g_sizex)+((X)+(I)):(X)+(I)>g_sizex-1?((X)+(I)-(g_sizex)):(X)+(I))
#define RY(X,I) ((X)+(I)<0?(g_sizey)+((X)+(I)):(X)+(I)>g_sizey-1?((X)+(I)-(g_sizey)):(X)+(I))
#define MAX(X,Y) ((X)>(Y)?(X):(Y))
#define MIN(X,Y) ((X)<(Y)?(X):(Y))
#define ABS(X) ((X)>0?(X):(-(X)))

// Way too many typedefs
typedef struct COLOR
{
	float r;
	float g;
	float b;
} color_t;

typedef struct CELL
{
	int palindex;
	float h;
} cell_t;

typedef struct SQUARE // I know it's a rectangle not a square thanks
{
	int x;
	int y;
	int u;
	int v;
} square_t;

typedef enum ACTION // JACKSON
{
	A_DRAW,
	A_SELECT,
	A_SELECTING,
	A_SELECTED // FishOrDie rpz
} action_t;

// Way way too many globals
// I don't give a shit it works great
int g_splash;
int g_fill;
int g_restart;
int g_continue;
int g_pause;
action_t g_actmode;
square_t g_selectzone;
int g_mx;
int g_my;
int g_plot;
int g_erase;
int g_paste;
float g_zoom;
int g_sizex;
int g_sizey;
int g_width;
int g_height;
float g_splashdens;
int g_splashsize;
int g_dbg;
int g_copied;
int g_oneframe;

cell_t** grid;
cell_t** buffer = NULL;
square_t bufsq;


void displayUsage(char * name)
{
	printf("Usage:\n");
	printf("\t%s [options]\n", name);
	printf("Options:\n");
	printf("\t-p <palette>\t Use a specific palette.\n");
	printf("\t-z <zoom>\t Size of one cell.\n");
	printf("\t-d <density>\t Density for random starting seed.\n");
	printf("\t-f \t\t Start paused.\n");
	printf("\t-h \t\t Display help.\n");
	printf("\t-hc \t\t Display controls.\n");
	printf("\t-nr \t\t Disable random starting seed.\n");
	printf("\t-nf \t\t Disable cell fading.\n");
	printf("\t-ne \t\t Disable endless mode.\n");
	printf("\t-r <r> <smin> <smax> <bmin> <bmax>\n");
	printf("\t\t\t Use specific set of rules.\n");
	printf("\t\t\t r:    neighborhood search radius.\n");
	printf("\t\t\t smin: minimum neighbors to survive.\n");
	printf("\t\t\t smax: maximum neighbors to survive.\n");
	printf("\t\t\t bmin: minimum neighbors for birth.\n");
	printf("\t\t\t bmax: maximum neighbors for birth.\n");
	printf("Examples:\n");
	printf("\t%s -r 5 34 58 34 45\n", name);
	printf("\t%s -r 5 9 9 9 9 -d 7\n", name);
	printf("\t%s -r 5 34 58 34 45\n", name);
	printf("\t%s -r 4 41 81 41 81\n", name);
	printf("\t%s -z 1\n", name);

	exit(0);
}

void displayControls()
{
	printf("Global:\n");
	printf(" ESCAPE\t\t Exits\n");
	printf(" BACKSPACE\t Restart\n");
	printf(" ENTER\t\t Pause\n");
	printf(" SPACE\t\t Iterate one frame\n\n");
	printf("Drawing:\n");
	printf(" S\t\t Switch to select mode\n");
	printf(" D\t\t Switch to draw mode\n");
	printf(" C\t\t Copy selected zone in buffer\n");
	printf(" X\t\t Cut selected zone and store in buffer\n");
	printf(" V\t\t Paste buffer (displayed in transparent)\n");
	printf(" F\t\t Fill selected zone\n");
	printf(" left click\t Paint in brush zone (draw mode)\n");
	printf(" right click\t Erase in brush zone (draw mode)\n");
	printf(" keypad *\t Increase brush size\n");
	printf(" keypad /\t Decrease brush size\n");
	printf(" keypad +\t Increase brush density\n");
	printf(" keypad -\t Decrease brush density\n");

	exit(0);
}


// Keyboard callback
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	switch(action)
	{
		case GLFW_PRESS:
			switch(key)
			{
				case GLFW_KEY_ESCAPE:
					glfwSetWindowShouldClose(window, GL_TRUE);
					g_continue = 0;
					g_restart = 0;
					break;
				case GLFW_KEY_T:
					g_splash = 1;
					break;
				case GLFW_KEY_BACKSPACE:
					g_continue = 0;
					break;
				case GLFW_KEY_KP_MULTIPLY:
					g_splashsize+=2;
					break;
				case GLFW_KEY_KP_DIVIDE:
					g_splashsize = g_splashsize>1?g_splashsize-2:1;
					break;
				case GLFW_KEY_KP_ADD:
					g_splashdens *= 1.2f;
					break;
				case GLFW_KEY_KP_SUBTRACT:
					g_splashdens *= 0.8f;
					break;
				case GLFW_KEY_P:
					g_dbg = g_dbg == 1 ? 0:1;
					break;
				case GLFW_KEY_S:
					g_actmode = A_SELECT;
					break;
				case GLFW_KEY_D:
					g_actmode = A_DRAW;
				case GLFW_KEY_SPACE:
					g_oneframe = 1;
					break;
					g_selectzone.x = -1;
					g_selectzone.y = -1;
					g_selectzone.u = -1;
					g_selectzone.v = -1;
					break;
				case GLFW_KEY_C:
					if (g_selectzone.x != -1 && g_selectzone.y != -1 && g_selectzone.u != -1 && g_selectzone.v != -1 )
					{
						if (buffer != NULL) free(buffer);
						int width = g_selectzone.u - g_selectzone.x + 1;
						int height = g_selectzone.v - g_selectzone.y + 1;
						buffer = malloc(width * sizeof(cell_t*));
						int x,y;
						for (x=0;x<width;x++)
							buffer[x] = malloc(height * sizeof(cell_t));
						for (y=0;y<height;y++)
						for (x=0;x<width;x++)
						{
							buffer[x][y].h = grid[g_selectzone.x + x][g_selectzone.y + y].h;
						}
						bufsq.u = g_selectzone.u;
						bufsq.v = g_selectzone.v;
						bufsq.x = g_selectzone.x;
						bufsq.y = g_selectzone.y;
						g_copied = 1;
					}
					break;
				case GLFW_KEY_X:
					if (g_selectzone.x != -1 && g_selectzone.y != -1 && g_selectzone.u != -1 && g_selectzone.v != -1 )
					{
						if (buffer != NULL) free(buffer);
						int width = g_selectzone.u - g_selectzone.x + 1;
						int height = g_selectzone.v - g_selectzone.y + 1;
						buffer = malloc(width * sizeof(cell_t*));
						int x,y;
						for (x=0;x<width;x++)
							buffer[x] = malloc(height * sizeof(cell_t));
						for (y=0;y<height;y++)
						for (x=0;x<width;x++)
						{
							buffer[x][y].h = grid[g_selectzone.x + x][g_selectzone.y + y].h;
							grid[g_selectzone.x + x][g_selectzone.y + y].h = 0;
						}
						bufsq.u = g_selectzone.u;
						bufsq.v = g_selectzone.v;
						bufsq.x = g_selectzone.x;
						bufsq.y = g_selectzone.y;
						g_copied = 1;
					}
					break;
				case GLFW_KEY_V:
					if (buffer != NULL)
					{
						int width = bufsq.u - bufsq.x + 1;
						int height = bufsq.v - bufsq.y + 1;
						int x,y;
						for (y=0;y<height;y++)
						for (x=0;x<width;x++)
						{
							if (buffer[x][y].h == 1.f)
								grid[RX(g_mx, x)][RY(g_my, y)].h = buffer[x][y].h;
						}
						g_copied = 1;
					}
					break;
					case GLFW_KEY_F:
						if (g_selectzone.x != -1 && g_selectzone.y != -1 && g_selectzone.u != -1 && g_selectzone.v != -1 )
						{
							g_fill = 1;
							g_copied = 0;
						}
						break;
				default:
					break;
			}
		break;
		case GLFW_RELEASE:
			switch(key)
			{
				case GLFW_KEY_T:
					g_splash = 0;
					break;
				case GLFW_KEY_ENTER:
					g_pause = g_pause == 1 ? 0 : 1;
					break;
				default:
				break;
			}
		break;
		default:
		break;
	}
}

// Cursor callbacks
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		g_plot = 1;
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		g_plot = 0;

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		g_erase = 1;
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		g_erase = 0;
}
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	g_mx = (int)(xpos*g_zoom);
	g_my = (int)(ypos*g_zoom);
}

// Useful
const char *get_filename_ext(const char *filename)
{
	const char *dot = strrchr(filename, '.');
	if(!dot || dot == filename) return "";
	return dot + 1;
}


int main( int argc, char* argv[])
{
	GLFWwindow* window;
	if (!glfwInit()) return EXIT_FAILURE; // Who u tryna fool ??

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	g_width = mode->width;
	g_height = mode->height;

	window = glfwCreateWindow(g_width, g_height, "lisa", monitor, NULL);
	//window = glfwCreateWindow(g_width, g_height, "lisa", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // does not work
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN); // sometimes work

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	g_restart = 1;
	g_continue = 1;

	srand(time(NULL));

	while (g_restart)
	{

		int norand = 0;
		int endless = 1;
		int pal = 0;
		char file[1024];
		char filelist[1024][1024];
		int nfiles = 0;
		int l_range = 1;
		int l_surv_min = 3;
		int l_surv_max = 4;
		int l_birth_min = 3;
		int l_birth_max = 3;
		int density = 2;
		int fade = 1;
		g_zoom = 1.f/4.f;
		g_splashdens = 1.f;
		g_splashsize = 2 + l_range;
		g_dbg = 0;
		g_pause = 0;
		g_actmode = A_DRAW;
		g_plot = 0;
		g_erase = 0;
		g_fill = 0;
		g_continue = 1;
		g_selectzone.x = -1;
		g_selectzone.y = -1;
		g_selectzone.u = -1;
		g_selectzone.v = -1;
		g_copied = 0;

		// arguments management
		// failure to follow syntax results in segfault, fun guaranteed
		int a;
		for (a=1;a<argc;a++)
		{
			if(strcmp(argv[a],"-h")==0 || strcmp(argv[a],"--help")==0) displayUsage(argv[0]);
			if(strcmp(argv[a],"-hc")==0) displayControls();
			else if(strcmp(argv[a],"-nr")==0) norand = 1;
			else if(strcmp(argv[a],"-ne")==0) endless = 0;
			else if(strcmp(argv[a],"-nf")==0) fade = 0;
			else if(strcmp(argv[a],"-f")==0) g_pause = 1;
			else if(strcmp(argv[a],"-r")==0)
			{
				l_range = atoi(argv[a+1]);
				l_surv_min = atoi(argv[a+2]);
				l_surv_max = atoi(argv[a+3]);
				l_birth_min = atoi(argv[a+4]);
				l_birth_max = atoi(argv[a+5]);
			}
			else if (strcmp(argv[a],"-d")==0)
			{
				density = atoi(argv[a+1]);
			}
			else if (strcmp(argv[a],"-z")==0)
			{
				g_zoom = 1.f/(float)atoi(argv[a+1]);
			}
			else if (strcmp(argv[a],"-p")==0)
			{
				pal = 1;
				strcpy(file,argv[a+1]);
			}
		}

		// if no palette specified, load random ".pal" in data/palettes folder
		if (!pal)
		{
			DIR *d;
			struct dirent *dir;
			d = opendir("data/palettes");
			if (d)
			{
				while ((dir = readdir(d)) != NULL)
					if (strcmp(get_filename_ext(dir->d_name),"pal")==0)
						strcpy(filelist[nfiles++], dir->d_name);
				closedir(d);
			}
			else
			{
				printf("data folder missing. Exiting.\n");
				return EXIT_FAILURE;
			}

			if( nfiles==0 )
			{
				printf ("No palette in data folder.\n");
				return EXIT_FAILURE;
			}

			printf ("%d palettes in data folder.\n", nfiles);
			strcpy (file,"data/palettes/");
			strcat (file,filelist[rand()%nfiles]);
		}

		FILE * fp;
		fp = fopen ( file , "r" );
		if( !fp )
		{
			printf ("Unable to open %s\n", file);
			return EXIT_FAILURE;
		}

		// read the palette
		// nothing is guaranteed in case of bad syntax
		color_t palette[5];
		int i;
		int tr, tg, tb;
		for(i=0;i<5;i++)
		{
			fscanf(fp, "%d,%d,%d\n", &tr, &tg, &tb);
			palette[i].r = tr/255.f;
			palette[i].g = tg/255.f;
			palette[i].b = tb/255.f;
		}
		printf("Palette %s loaded.\n",file);

		// THE GRID
		g_sizex = g_width * (g_zoom);
		g_sizey = g_height * (g_zoom);

		cell_t** tgrid;
		cell_t** agrid;

		int x,y;

		grid  = malloc(g_sizex * sizeof(cell_t*));
		agrid = malloc(g_sizex * sizeof(cell_t*));
		tgrid = malloc(g_sizex * sizeof(cell_t*));
		for (x=0;x<g_sizex;x++)
		{
			grid[x]  = malloc(g_sizey * sizeof(cell_t));
			agrid[x] = malloc(g_sizey * sizeof(cell_t));
			tgrid[x] = malloc(g_sizey * sizeof(cell_t));
		}


		// randomness
		for (y=0;y<g_sizey;y++)
		for (x=0;x<g_sizex;x++)
		{
			int index = rand()%5;
			grid[x][y].palindex = index;
			if(!norand) grid[x][y].h = ((rand() % density)>0?0.f:1.f);
			else grid[x][y].h = 0;
		}

		int testtime = 0;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, g_sizex, g_sizey, 0);

		// dead code below
		// double ptime_rule;
		// double ptime_render;
		// double ctime_rule;
		// double ctime_render;

		// run forever
		while (g_continue)
		{
			if(glfwWindowShouldClose(window)) // not quite forever
			{
				g_continue = 0;
				g_restart = 0;
			}

			// test if grid changed over 2 frames #1
			if (!g_pause && endless)
			{
				testtime ++;
				if (testtime>TESTINTERVAL)
				{
					for (y=0;y<g_sizey;y++)
					for (x=0;x<g_sizex;x++)
						tgrid[x][y].h = agrid[x][y].h;
					testtime = -1;
				}
			}

			// save previous state
			for (y=0;y<g_sizey;y++)
			for (x=0;x<g_sizex;x++)
			{
				agrid[x][y].h = grid[x][y].h;
				agrid[x][y].palindex = grid[x][y].palindex;
			}


			// follow the rules
			if (!g_pause || g_oneframe)
			{
				g_oneframe = 0;
				for (y=0;y<g_sizey;y++)
				for (x=0;x<g_sizex;x++)
				{
					//fade if dead
					if (agrid[x][y].h < 1.f && agrid[x][y].h > 0.01f && fade)
						grid[x][y].h *= FCOEF;
					else if (agrid[x][y].h < 1.f && agrid[x][y].h > 0.01f)
						grid[x][y].h = 0.f;

					int sum = 0;
					int i,j;
					int indexw[5];
					int pindex = 0;
					for (i=0;i<5;i++)
						indexw[i] = 0;

					for (i=-l_range;i<=l_range;i++)
					for (j=-l_range;j<=l_range;j++)
					{
						if (agrid[RX(x,i)][RY(y,j)].h == 1.f)
						{
							sum++;
							indexw[agrid[RX(x,i)][RY(y,j)].palindex]++;
							pindex++;
						}
					}

					//rules
					if      (agrid[x][y].h == 1.f && (sum < l_surv_min || sum > l_surv_max)) grid[x][y].h -= fade?FDEC:1.f;
					else if (agrid[x][y].h == 1.f || (agrid[x][y].h < 1.f && sum >= l_birth_min && sum <= l_birth_max))
					{
						grid[x][y].h = 1.f;
						int pi;
						for (pi = 0; pi < 5; pi++) // magic happens
							if (indexw[grid[x][y].palindex]<indexw[pi]) grid[x][y].palindex = pi;
					}
				}
			}

			// controls !
			if (g_actmode == A_DRAW)
			{
				if (g_plot)
				{
					int i;
					for(i = 0; i< 10*g_splashdens; i++)
					{
						int dx = rand()%(g_splashsize) - ((g_splashsize)/2);
						int dy = rand()%(g_splashsize) - ((g_splashsize)/2);
							grid[RX(g_mx,dx)][RY(g_my,dy)].h = 1.f;
					}
				}
				else if (g_erase)
				{
					int i;
					for(i = 0; i< 10*g_splashdens; i++)
					{
						int dx = rand()%(g_splashsize) - ((g_splashsize)/2);
						int dy = rand()%(g_splashsize) - ((g_splashsize)/2);
							grid[RX(g_mx,dx)][RY(g_my,dy)].h = 0.f;
					}
				}
			}
			else if (g_actmode == A_SELECT)
			{
				if (g_plot)
				{
					g_copied = 0;
					g_selectzone.x = g_mx;
					g_selectzone.y = g_my;
					g_selectzone.u = g_mx;
					g_selectzone.v = g_my;
					g_actmode = A_SELECTING;
				}
			}
			else if (g_actmode == A_SELECTING)
			{
				g_selectzone.u = g_mx;
				g_selectzone.v = g_my;
				if (!g_plot)
					g_actmode = A_SELECTED;
			}
			if (g_actmode == A_SELECTED)
			{
				int tx = g_selectzone.x;
				int ty = g_selectzone.y;
				int tu = g_selectzone.u;
				int tv = g_selectzone.v;
				g_selectzone.x = MIN(tx,tu);
				g_selectzone.y = MIN(ty,tv);
				g_selectzone.u = MAX(tx,tu);
				g_selectzone.v = MAX(ty,tv);
				g_actmode = A_SELECT;
			}

			if (g_fill)
			{
				int k,l;
				for(k=MIN(g_selectzone.x,g_selectzone.u); k<=MAX(g_selectzone.x,g_selectzone.u); k++)
				for(l=MIN(g_selectzone.y,g_selectzone.v); l<=MAX(g_selectzone.y,g_selectzone.v); l++)
					grid[k][l].h = 1.f;
				g_fill = 0;
			}

			// test if grid changed over 2 frames #2
			if (testtime == -1)
			{
				int changed = 0;
				for (y=0;y<g_sizey;y++)
				for (x=0;x<g_sizex;x++)
					if (tgrid[x][y].h != grid[x][y].h) changed++;
				if (!changed)
					g_continue = 0;
				testtime = 0;
			}


			// render everything
			glClear(GL_COLOR_BUFFER_BIT);
			glBegin(GL_QUADS);

			// THE GRID
			for (y=0;y<g_sizey;y++)
			for (x=0;x<g_sizex;x++)
			{
				if (grid[x][y].h < 0.01f) continue;
				glColor4f(palette[grid[x][y].palindex].r, palette[grid[x][y].palindex].g, palette[grid[x][y].palindex].b, grid[x][y].h);
				float fx = (float)x + 0.5f;
				float fy = (float)y + 0.5f;
				glVertex3f(0.5f + fx, -0.5f + fy, 0.f);
				glVertex3f(-0.5f + fx, -0.5f + fy, 0.f);
				glVertex3f(-0.5f + fx, 0.5f + fy, 0.f);
				glVertex3f(0.5f + fx, 0.5f + fy, 0.f);
			}

			// the brush
			if (g_actmode == A_DRAW)
			{
				glColor4f(1.0f,1.0f,1.0f,0.3f);
				glVertex3f(0.5f + g_mx + g_splashsize/2 +1, -0.5f + g_my - g_splashsize/2, 0.f);
				glVertex3f(-0.5f + g_mx - g_splashsize/2, -0.5f + g_my - g_splashsize/2, 0.f);
				glVertex3f(-0.5f + g_mx - g_splashsize/2, 0.5f + g_my + g_splashsize/2 +1, 0.f);
				glVertex3f(0.5f + g_mx + g_splashsize/2 +1, 0.5f + g_my + g_splashsize/2 +1, 0.f);
			}

			// the selection rectangle
			if (g_selectzone.x != -1 && g_selectzone.u != -1 && !g_copied)
			{
				if (g_actmode == A_SELECTING) glColor3f(0.2f,1.f,0.3f);
				else glColor3f(0.3f,0.6f,0.4f);
				float fx1 = MIN(g_selectzone.x,g_selectzone.u) + 0.5f;
				float fx2 = MAX(g_selectzone.x,g_selectzone.u) + 0.5f;
				float fy1 = MIN(g_selectzone.y,g_selectzone.v) + 0.5f;
				float fy2 = MAX(g_selectzone.y,g_selectzone.v) + 0.5f;
				glVertex3f(0.75f + fx2, -0.75f + fy1, 0.f);
				glVertex3f(-0.75f + fx1, -0.75f + fy1, 0.f);
				glVertex3f(-0.75f + fx1, 0.75f + fy2, 0.f);
				glVertex3f(0.75f + fx2, 0.75f + fy2, 0.f);

				int k,l;
				for(k=MIN(g_selectzone.x,g_selectzone.u); k<=MAX(g_selectzone.x,g_selectzone.u); k++)
				for(l=MIN(g_selectzone.y,g_selectzone.v); l<=MAX(g_selectzone.y,g_selectzone.v); l++)
				{
					glColor3f(grid[k][l].h*(palette[grid[k][l].palindex].r), grid[k][l].h*(palette[grid[k][l].palindex].g), grid[k][l].h*(palette[grid[k][l].palindex].b));
					float fx = k + 0.5f;
					float fy = l + 0.5f;
					glVertex3f(0.5f + fx, -0.5f + fy, 0.f);
					glVertex3f(-0.5f + fx, -0.5f + fy, 0.f);
					glVertex3f(-0.5f + fx, 0.5f + fy, 0.f);
					glVertex3f(0.5f + fx, 0.5f + fy, 0.f);
				}
			}

			// the mouse
			glColor3f(1.f,1.f,1.f);
			float fx = g_mx + 0.5f;
			float fy = g_my + 0.5f;
			glVertex3f(0.75f + fx, -0.75f + fy, 0.f);
			glVertex3f(-0.75f + fx, -0.75f + fy, 0.f);
			glVertex3f(-0.75f + fx, 0.75f + fy, 0.f);
			glVertex3f(0.75f + fx, 0.75f + fy, 0.f);

			glColor3f(grid[g_mx][g_my].h*(palette[grid[g_mx][g_my].palindex].r), grid[g_mx][g_my].h*(palette[grid[g_mx][g_my].palindex].g), grid[g_mx][g_my].h*(palette[grid[g_mx][g_my].palindex].b));
			fx = g_mx + 0.5f;
			fy = g_my + 0.5f;
			glVertex3f(0.5f + fx, -0.5f + fy, 0.f);
			glVertex3f(-0.5f + fx, -0.5f + fy, 0.f);
			glVertex3f(-0.5f + fx, 0.5f + fy, 0.f);
			glVertex3f(0.5f + fx, 0.5f + fy, 0.f);

			// the buffer
			int k,l;
			if (buffer!=NULL && g_actmode != A_DRAW)
			{
				glColor4f(1.0f,1.0f,1.0f,0.3f);
				for(k=0; k<=bufsq.u-bufsq.x; k++)
				for(l=0; l<=bufsq.v-bufsq.y; l++)
				{
					if (buffer[k][l].h==1.f)
					{
						fx = g_mx + k + 0.5f;
						fy = g_my + l + 0.5f;
						glVertex3f(0.5f + fx, -0.5f + fy, 0.f);
						glVertex3f(-0.5f + fx, -0.5f + fy, 0.f);
						glVertex3f(-0.5f + fx, 0.5f + fy, 0.f);
						glVertex3f(0.5f + fx, 0.5f + fy, 0.f);
					}
				}
			}

			glEnd();

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

	glfwTerminate();


	return EXIT_SUCCESS;
}
