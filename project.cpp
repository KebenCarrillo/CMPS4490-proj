//modified by: Keben Carrillo & Laura Moreno
//date: 04/21/22
//
//author: Gordon Griesel
//date: Spring 2022
//purpose: get openGL working on your personal computer
//
#include <iostream>
#include <fstream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
//for text on the screen
#include "fonts.h"
#include <pthread.h>
//#include <stdlib.h>

//MVC architecture
//M - Model
//V - View
//C - Controller

class Image {
public:
    int width, height, max;
    char *data;
    Image() {}
    Image(const char *fname) {
        bool isPPM = true;
        char str[1200];
        char newfile[200];
        ifstream fin;
        char *p = strstr((char *)fname, ".ppm");
        if (!p) {
            //not a ppm file
            isPPM = false;
            strcpy(newfile, fname);
            newfile[strlen(newfile)-4] = '\0';
            strcat(newfile, ".ppm");
            sprintf(str, "convert %s %s", fname, newfile);
            system(str);
            fin.open(newfile);
        } else {
            fin.open(fname);
        }
        char p6[10];
        fin >> p6;
        fin >> width >> height;
        printf("%i x %i\n", width, height);
        fin >> max;
        data = new char [width * height * 3];
        fin.read(data, 1);
        fin.read(data, width * height * 3);
        fin.close();
        if (!isPPM)
            unlink(newfile);
    }
//Outerspace Background:
//https://opengameart.org/content/space-backgrounds-0
} img("/home/stu/lmoreno/4490/proj/CMPS4490-proj/space.png"), 
  sprite("/home/stu/lmoreno/4490/proj/CMPS4490-proj/greyspaceship.png"),
  blackhole("/home/stu/lmoreno/4490/proj/CMPS4490-proj/blackhole.png"),
  goldrock("/home/stu/lmoreno/4490/proj/CMPS4490-proj/goldrock.png");
  //img("/home/stu/kcarrillo/4490/proj/CMPS4490-proj/space.png"),
  //sprite("/home/stu/kcarrillo/4490/proj/CMPS4490-proj/spaceship.png"),
  //blackhole("/home/stu/kcarrillo/4490/proj/CMPS4490-proj/blackhole.png"),
  //goldrock("/home/stu/kcarrillo/4490/proj/CMPS4490-proj/goldrock.png");
  //planet("/home/stu/kcarrillo/4490/proj/CMPS4490-proj/planet.png");

typedef float Flt;
typedef Flt Vec[2];

struct Vector {
    float x,y,z;
};

struct Point {
    Flt pos[2];
    Flt vel[2];
    Point() { }
};
////////////////////////////
typedef double Flt2;
struct Box {
    Flt2 h, w, pos[3];
};
////////////////////////////

//game object
class Ship {
public:
    Flt pos[3]; //vector
    Flt vel[3]; //vector
    float w, h;
    Box box[5];
    unsigned int color;
    bool alive_or_dead_;
    Flt mass;
    Ship() {
        w = h = 4.0;
        pos[0] = 1.0;
        pos[1] = 200.0;
        vel[0] = 4.0;
        vel[1] = 0.0;
    }
    Ship(int x, int y) {
        w = h = 4.0;
        pos[0] = x;
        pos[1] = y;
        vel[0] = 0.0;
        vel[1] = 0.0;
    }
    void set_dimensions(int x, int y) {
        w = (float)x * 0.05;
        h = w; 
    }
} control(300,200);

enum {
    STATE_TOP,
    STATE_INTRO,
    STATE_INSTRUCTIONS,
    STATE_SHOW_OBJECTIVES,
    STATE_PLAY,
    STATE_GAME_OVER,
    STATE_BOTTOM
};

class Global {
public:
	char keys[65536];
    
    int xres, yres;
    Ship ship[3];
    Ship blhole[3];
    Ship rock[3];
    // the box components
    float pos[2];
    Point p[4];
    float w, h;
    float dir;
    int inside;
    ////////////////////// 
    int up; 
    int down;
    int left;
    int right;
    //////////////////////
    int state;
    int state2;
    int score;
    int lives;
    int starttime;
    int countdown;
    int playtime;
    
    int timeCount;
    time_t timer;
    unsigned int texid;
    unsigned int spriteid;
    unsigned int plid;
    unsigned int bhid;
    unsigned int rockid;
    Flt gravity;
    int frameno;

    Global();
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
    //XdbeBackBuffer backBuffer;
    //XdbeSwapInfo swapInfo;
    //GC gc;
public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
	void reshape_window(int width, int height);
	void check_resize(XEvent *e);
	void check_mouse(XEvent *e);
	int check_keys(XEvent *e);
    //void drawText(int x, int y, char *text) {
    //    XDrawString(dpy, backBuffer, gc, x, y, text, strlen(text));
    //}
} x11;

//Function prototypes
void init_opengl(void);
void physics(void);
void render(void);

void *spriteThread(void *arg)
{
    //-----------------------------------------------------------------------------
    //Setup timers
    //const double OOBILLION = 1.0 / 1e9;
    struct timespec start, end;
    extern double timeDiff(struct timespec *start, struct timespec *end);
    extern void timeCopy(struct timespec *dest, struct timespec *source);
    //clock_gettime(CLOCK_REALTIME, &smokeStart);
    //-----------------------------------------------------------------------------
    clock_gettime(CLOCK_REALTIME, &start);
    double diff;
    while (1) {
        //if some amount of time has passed, change the frame number.
        clock_gettime(CLOCK_REALTIME, &end);
        diff = timeDiff(&start, &end);
        if (diff >= 0.05) {
            //enough time has passed
            ++g.frameno;
            if (g.frameno > 24)
                g.frameno = 1;
            timeCopy(&start, &end);
        }
    }
    return (void *)0;
}

int main()
{
    //start the thread...
    pthread_t th;
    pthread_create(&th, NULL, spriteThread, NULL);

	init_opengl();
	initialize_fonts();
    //main game loop
	int done = 0;
	while (!done) {
		//process events...
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			x11.check_mouse(&e);
			done = x11.check_keys(&e);
		}
        if (g.state == STATE_PLAY) {
            //check countdown timer
            g.countdown = time(NULL) - g.starttime;
            if (g.countdown > g.playtime)
                g.state = STATE_GAME_OVER;
        }
		physics();           //move things
		render();            //draw things
		x11.swapBuffers();   //make video memory visible
		usleep(1000);         //pause to let X11 work better
	}
    cleanup_fonts();
	return 0;
}

Global::Global()
{
	memset(keys, 0, 65536);
    xres = 800;
	yres = 400;
    //box
    w = 20.0f;
    pos[0] = 0.0f + w;
    pos[1] = yres/2.0f;
    dir = 1.0f;     //my machine goes too fast so i need to put a small #
    inside = 0;
    gravity = 80.0;
    frameno = 1;
    score = 0;
    lives = 0;
    state = STATE_INTRO;
    //state2 = STATE_INTRUCTIONS;
    timer = time(NULL);
    timeCount = 0;
    //countTimer= 0;
    //
    up = 0;
    down = 0;
    left = 0;
    right = 0;
}

X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w = g.xres, h = g.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
		InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "4490 Project");
}

bool X11_wrapper::getXPending()
{
	//See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	//Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
	//window has been resized.
	g.xres = width;
	g.yres = height;
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
    g.ship[0].set_dimensions(g.xres, g.yres);
}

void X11_wrapper::check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != g.xres || xce.height != g.yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}
//-----------------------------------------------------------------------------

void X11_wrapper::check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	//Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed.
			int y = g.yres - e->xbutton.y;
			int x = e->xbutton.x;
            if (g.state == STATE_INTRO) {

            }
            if (g.state == STATE_INSTRUCTIONS) {

            }
            if (g.state == STATE_PLAY) {
                if (x >= g.pos[0]-g.w && x <= g.pos[0]+g.w) {
                   if (y >= g.pos[1]-g.w && y <= g.pos[1]+g.w) {
                       //g.inside++;
                       //printf("hits: %i\n", g.inside);
                       g.score += 1;
                       //check for game over
                       if (g.score == 5) {
                            g.state = STATE_GAME_OVER;
                       }
                   }
                }
            }
            /*
            if (x >= g.pos[0]-g.w && x <= g.pos[0]+g.w) {
               if (y >= g.pos[1]-g.w && y <= g.pos[1]+g.w) {
                   g.inside++;
                   printf("hits: %i\n", g.inside);
               }
            } */   
            return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed.
            if (g.state == STATE_GAME_OVER) {
                //restart_game();
                g.score = 0;
                g.state = STATE_INTRO;
            }
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			//Code placed here will execute whenever the mouse moves.


		}
	}
}

int X11_wrapper::check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
    if (e->type == KeyPress)   g.keys[key] = 1;
    if (e->type == KeyRelease) g.keys[key] = 0;
	if (e->type == KeyPress) {
		switch (key) {
			case XK_s:
				//Key s was pressed
                if (g.state == STATE_INTRO) {
                    g.state = STATE_PLAY;
                    g.starttime = time(NULL);
                    g.playtime = 40;
                }
				break;
            case XK_i:
                //Instructions
                if (g.state == STATE_INTRO) {
                    g.state == STATE_INSTRUCTIONS;
                    g.state == STATE_PLAY;
                }
                break;
            ////////////////////////////
            /*case XK_Up:
                //move ship up
                g.up = 1;
                g.ship[0].vel[1] += 10;
                //g.control.vel[1] += 10;
                break;*/
            ////////////////////////////
            case XK_2:
                //Key 2 was pressed
                //move down
                break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

unsigned char *buildAlphaData(Image *img)
{   
    //add 4th component to RGB stream...
    int i;
    int a,b,c;
    unsigned char *newdata, *ptr;
    unsigned char *data = (unsigned char *)img->data;
    newdata = (unsigned char *)malloc(img->width * img->height * 4);
    ptr = newdata;
    for (i=0; i<img->width * img->height * 3; i+=3) {
        a = *(data+0);
        b = *(data+1);
        c = *(data+2);
        *(ptr+0) = a;
        *(ptr+1) = b;
        *(ptr+2) = c;
        //-----------------------------------------------
        //get largest color component...
        //*(ptr+3) = (unsigned char)((
        //      (int)*(ptr+0) +
        //      (int)*(ptr+1) +
        //      (int)*(ptr+2)) / 3);
        //d = a;
        //if (b >= a && b >= c) d = b;
        //if (c >= a && c >= b) d = c;
        //*(ptr+3) = d;
        //-----------------------------------------------
        //this code optimizes the commented code above.
        *(ptr+3) = (a!=0 && b!=0 && c!=0);
        //-----------------------------------------------
        ptr += 4;
        data += 3;
    }
    return newdata;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 0.0);
    //allow 2D texture maps
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
    
    //background - outer space 
    glGenTextures(1, &g.texid);
    glBindTexture(GL_TEXTURE_2D, g.texid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, img.width, img.height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, img.data);

    /*
    //background - planet
    glGenTextures(1, &g.plid);
    glBindTexture(GL_TEXTURE_2D, g.plid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, planet.width, planet.height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, planet.data);
    */
    //ship sprite

    //make a new data stream, with 4 color components
    //add an alpha channel
    //
    // 0 aaabbbccc
    // 1 dddeeefff
    // 0 aaaabbbbcccc
    // 1 ddddeeeeffff
    //
    //#define CALL_FUNC
    //#ifdef CALL_FUNC
    //unsigned char *data2 = buildAlphaData(&sprite);
    //#else
    unsigned char *data2 = new unsigned char [sprite.width * sprite.height * 4];
    for (int i=0; i<sprite.height; i++) {
        for (int j=0; j<sprite.width; j++) {
            int offset  = i*sprite.width*3 + j*3;
            int offset2 = i*sprite.width*4 + j*4;
            data2[offset2+0] = sprite.data[offset+0];
            data2[offset2+1] = sprite.data[offset+1];
            data2[offset2+2] = sprite.data[offset+2];
            data2[offset2+3] =
            ((unsigned char)sprite.data[offset+0] != 0 &&
             (unsigned char)sprite.data[offset+1] != 0 &&
             (unsigned char)sprite.data[offset+2] != 0);
        }
    }

    //blackhole
    unsigned char *data3 = new unsigned char [blackhole.width * blackhole.height * 4];
    for (int i=0; i<blackhole.height; i++) {
        for (int j=0; j<blackhole.width; j++) {
            int offset  = i*blackhole.width*3 + j*3;
            int offset2 = i*blackhole.width*4 + j*4;
            data3[offset2+0] = blackhole.data[offset+0];
            data3[offset2+1] = blackhole.data[offset+1];
            data3[offset2+2] = blackhole.data[offset+2];
            data3[offset2+3] =
            ((unsigned char)blackhole.data[offset+0] != 0 &&
             (unsigned char)blackhole.data[offset+1] != 0 &&
             (unsigned char)blackhole.data[offset+2] != 0);
        }
    }
    

    //rock
    unsigned char *data4 = new unsigned char [goldrock.width * goldrock.height * 4];
    for (int i=0; i<goldrock.height; i++) {
        for (int j=0; j<goldrock.width; j++) {
            int offset  = i*goldrock.width*3 + j*3;
            int offset2 = i*goldrock.width*4 + j*4;
            data3[offset2+0] = goldrock.data[offset+0];
            data3[offset2+1] = goldrock.data[offset+1];
            data3[offset2+2] = goldrock.data[offset+2];
            data3[offset2+3] =
            ((unsigned char)goldrock.data[offset+0] != 0 &&
             (unsigned char)goldrock.data[offset+1] != 0 &&
             (unsigned char)goldrock.data[offset+2] != 0);
        }
    }
    //#endif
    glGenTextures(1, &g.spriteid);
    glBindTexture(GL_TEXTURE_2D, g.spriteid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite.width, sprite.height, 
                                0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
    delete [] data2;
    g.ship[0].set_dimensions(g.xres, g.yres);

    //#endif
    glGenTextures(1, &g.bhid);
    glBindTexture(GL_TEXTURE_2D, g.bhid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, blackhole.width, blackhole.height,
                                0, GL_RGBA, GL_UNSIGNED_BYTE, data3);
    delete [] data3;
    g.blhole[0].set_dimensions(g.xres, g.yres);

    glGenTextures(1, &g.rockid);
    glBindTexture(GL_TEXTURE_2D, g.rockid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, goldrock.width, goldrock.height,
                                0, GL_RGBA, GL_UNSIGNED_BYTE, data4);
    delete [] data4;
    g.rock[0].set_dimensions(g.xres, g.yres);
}

void physics()
{
    //accelerate with spacebar
    if (g.keys[XK_space] == 1)
        control.vel[0] += 1.0;
    control.vel[0] -= 0.1;
    //control.pos[1] += control.vel[1];
    //if (control.pos[1] < 0) {
    //    control.pos[1] = 0;
    //    control.vel[1] = 0.0;
    //}
    // move ship up
    if (g.keys[XK_Up] == 1) 
        control.vel[1] += 1.0;
    control.vel[1] -= 0.1;
    control.pos[1] += control.vel[1];
    if (control.pos[1] < 0) {
        control.pos[1] = 0;
        control.vel[1] = 0.0;
    }
    // move ship down
    if (g.keys[XK_Down] == 1)
        control.vel[1] -= 1.0;
    control.vel[1] -= 0.1;
    control.pos[1] += control.vel[1];
    if (control.pos[1] < 0) {
        control.pos[1] = 0;
        control.vel[1] = 0.0;
    }
    // move ship left
    if (g.keys[XK_Left] == 1)
        control.vel[0] -= 1.0;
    control.vel[0] -= 0.1;
    control.pos[0] += control.vel[0];
    if (control.pos[0] < 0) {
        control.pos[0] = 0;
        control.vel[0] = 0.0;
    }
    // move ship right
    if (g.keys[XK_Right] == 1)
        control.vel[0] += 1.0;
    control.vel[0] -= 0.1;
    control.pos[0] += control.vel[0];
    if (control.pos[0] < 0) {
        control.pos[0] = 0;
        control.vel[0] = 0.0;
    }
    
    /*time_t curr_seconds = time(NULL);
    if (curr_seconds != g.timer) {
        //define a line segment
        for (int i=0; i<2; i++) {
            g.p[i].pos[0] = rand() % g.xres;
            g.p[i].pos[1] = rand() % g.yres;
        }
        g.timer = curr_seconds - g.timeCount;
    }
    //++g.frameno;
    //if (g.frameno > 20)
    //    g.frameno = 1;
    //movement
    g.ship[0].pos[0] += g.ship[0].vel[0];
    g.ship[0].pos[1] += g.ship[0].vel[1];
    //boundary test
    if (g.ship[0].pos[0] >= g.xres) {
        g.ship[0].pos[0] = g.xres;
        g.ship[0].vel[0] = 0.0;
    }
    if (g.ship[0].pos[0] <= 0) {
        g.ship[0].pos[0] = 0;
        g.ship[0].vel[0] = 0.0;
    }
    if (g.ship[0].pos[1] >= g.yres) {
        g.ship[0].pos[1] = g.yres;
        g.ship[0].vel[1] = 0.0;
    }
    if (g.ship[0].pos[1] <= 0) {
        g.ship[0].pos[1] = 0;
        g.ship[0].vel[1] = 0.0;
    }
    //move the bee toward the flower...
    Flt cx = g.xres/2.0;
    Flt cy = g.yres/2.0;
    cx = g.xres * (704.0/768.0);
    cy = g.yres * (706.0/714.0);
    Flt dx = cx - g.ship[0].pos[0];
    Flt dy = cy - g.ship[0].pos[1];
    Flt dist = (dx*dx + dy*dy);
    if (dist < 0.01)
        dist = 0.01; //clamp
    //change in velocity based on a force (gravity)
    //g.ship[0].vel[0] += (dx / dist) * g.gravity;
    //g.ship[0].vel[1] += (dy / dist) * g.gravity;  
    g.ship[0].vel[0] += ((Flt)rand() / (Flt)RAND_MAX) * 0.5 - 0.25;
    g.ship[0].vel[1] += ((Flt)rand() / (Flt)RAND_MAX) * 0.5 - 0.25;
    */
}

void render()
{
	//quick and dirty road scrolling..
    static float xcoord = 0.00f;
    
	glClear(GL_COLOR_BUFFER_BIT);

    glColor3ub(255, 255, 255);
    //glColor3ub(80, 80, 160);
    glBindTexture(GL_TEXTURE_2D, g.texid);
    //glBindTexture(GL_TEXTURE_2D, g.plid);
    glBegin(GL_QUADS);
        glTexCoord2f(xcoord-1.0f,1);      glVertex2i(0,       0);
        glTexCoord2f(xcoord-1.0f,0);      glVertex2i(0,      g.yres);
        glTexCoord2f(xcoord,0); glVertex2i(g.xres, g.yres);
        glTexCoord2f(xcoord,1); glVertex2i(g.xres, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    xcoord += 0.02;

    Rect r;
    if (g.state == STATE_INTRO) {
        //show the intro.
        r.bot = g.yres / 2;
        r.left = g.xres / 2;
        r.center = 1;
        ggprint40(&r, 20, 0x00ffffff, "Welcome To Galactic Ship");
        ggprint12(&r, 30, 0x00008000, "Press 's' to Start");
        ggprint12(&r, 40, 0x00fff8bb30, "Press 'i' to See Instructions");
        return;
    }
    //Intructions
    if (g.state == STATE_INSTRUCTIONS) {
        r.bot = g.yres / 2;
        r.left = g.xres / 2;
        r.center = 1;
        ggprint16(&r, 20, 0x00ff0000, "OBJECTIVE:");
        ggprint16(&r, 40, 0x00ff0000, "The goal of the game is to avoid the orbs" 
                "and collect the most amount of ...");
        ggprint16(&r, 20, 0x00EA1200, "HOW TO PLAY");
        return;
    }
    if (g.state == STATE_GAME_OVER) {
        r.bot = g.yres / 2;
        r.left = g.xres / 2;
        r.center = 1;
        ggprint16(&r, 20, 0x00ff0000, "GAME OVER");
        ggprint13(&r, 50, 0x00ffff00, "Your Score: %i", g.score);
        ggprint12(&r, 0, 0x00bfff, "Right-Click to Play Again");
        return;
    }
    /*r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;

    Rect l;
    l.bot = g.yres - 20;
    l.left = 440;
    l.center = 0;

    Rect t;
    t.bot = g.yres - 40;
    t.left = 10;
    t.center = 0;
    ggprint8b(&r, 0, 0x00f2faff, "Score: %i ", g.score);
    ggprint8b(&l, 0, 0x00FF0000, "Lives: %i ", g.lives);
    ggprint8b(&t, 0, 0x00ACBFFF, "Timer: %i ", g.timer);
    */

    ///////////////////
    //velocity doesnt work
    /*static float move = -100.0f;
    static float up = 0.0f;
    static float vel = 2.0f;
    glTranslatef(g.xres/2 + move, 60.0f + up, 0.0f);
    //
    if (g.up) {
        up += vel;
        if(up > 100)
            vel = -vel;
    }*/


	//Draw ship.
	if (g.state == STATE_PLAY) {
        r.bot = g.yres - 20;
        r.left = 10;
        r.center = 0;
        Rect l;
        l.bot = g.yres - 20;
        l.left = 720;
        l.center = 0;
        ggprint10(&r, 30, 0x00ffffff, "Player's score: %i", g.score);
        ggprint13(&l, 0, 0x00FF0000, "Lives: %i ", g.lives);
        ggprint12(&r, 0, 0x00ACBFFF, "Countdown: %i", g.playtime - g.countdown);
        
        ////// Ship ////////
        glPushMatrix();
        glColor3ub(255, 255, 255);
        //glTranslatef(g.xres/2+20.0f, 60.0f, 0.0f);
        //static float forward = 0.5f;
        //velocity doesnt work
        //static float move = -100.0f;
        //static float up = 0.0f;
        //static float vel = 2.0f;
        glTranslatef(control.pos[0], control.pos[1], 0.0f);
        //
        //if (g.up) {
        //    up += vel;
        //    if(up > 100)
        //        vel = -vel;
        //}
        //glTranslatef(g.ship[0].pos[0]+forward, g.ship[0].pos[1], 0.0f);
        //forward++; 
        //set alpha test
        //https://www.khronos.org/registry/OpenGL-Refpages/gl2.1
        ///xhtml/glAlphaFunc.xml
        glEnable(GL_ALPHA_TEST);
        //transparent if alpha value is greater than 0.0
        glAlphaFunc(GL_GREATER, 0.0f);
        //Set 4-channels of color intensity
        glColor4ub(255,255,255,255);

        //glBegin(GL_TRIANGLE_FAN)
        glBindTexture(GL_TEXTURE_2D, g.spriteid);
        //make texture coordinates based on frame number.
        float tx1 = 0.0f + (float)((g.frameno-1) % 3) * 0.33f;
        float tx2 = tx1 + 0.33f;
        //printf("%f %f\n", tx1, tx2);
        float ty1 = 0.0f + (float)((g.frameno-1) / 8) * 0.125f;
        float ty2 = ty1 + 0.125;
        /*
        if (g.ship[0].vel[0] > 0.0) {
            float tmp = tx1;
            tx1 = tx2;
            tx2 = tmp;
        }
        glBegin(GL_QUADS);
            //glTexCoord2f( 0, 1); glVertex2f(-g.bees[0].w, -g.bees[0].h);
            //glTexCoord2f( 0,.8); glVertex2f(-g.bees[0].w,  g.bees[0].h);
            //glTexCoord2f(.2,.8); glVertex2f( g.bees[0].w,  g.bees[0].h);
            //glTexCoord2f(.2, 1); glVertex2f( g.bees[0].w, -g.bees[0].h);
            glTexCoord2f(tx1, ty2); glVertex2f(-g.ship[0].w, -g.ship[0].h);
            glTexCoord2f(tx1, ty1); glVertex2f(-g.ship[0].w,  g.ship[0].h);
            glTexCoord2f(tx2, ty1); glVertex2f( g.ship[0].w,  g.ship[0].h);
            glTexCoord2f(tx2, ty2); glVertex2f( g.ship[0].w, -g.ship[0].h);
        glEnd();*/
        float w = 40;
        float h = 80;
        glBegin(GL_QUADS);
            glTexCoord2f(tx1, ty2); glVertex2f(-w, -h);
            glTexCoord2f(tx1, ty1); glVertex2f(-w,  h);
            glTexCoord2f(tx2, ty1); glVertex2f( w,  h);
            glTexCoord2f(tx2, ty2); glVertex2f( w, -h);
        glEnd();
        //turn off alpha test
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
        glPopMatrix();


        ///////// Blackhole ////////
        glPushMatrix();
        glColor3ub(255, 255, 255);
        //static float move = 1.0f;
        //glTranslatef(g.blhole[0].pos[0], g.blhole[0].pos[1], 0.0f);
        
        glTranslatef(g.xres/2, 120.0f, 0.0f);        
        //set alpha test
        //https://www.khronos.org/registry/OpenGL-Refpages/gl2.1
        ///xhtml/glAlphaFunc.xml
        glEnable(GL_ALPHA_TEST);
        //transparent if alpha value is greater than 0.0
        glAlphaFunc(GL_GREATER, 0.0f);
        //Set 4-channels of color intensity
        glColor4ub(255,255,255,255);

        //glBegin(GL_TRIANGLE_FAN)
        glBindTexture(GL_TEXTURE_2D, g.bhid);
        //make texture coordinates based on frame number.
        float bx1 = 0.0f + (float)((g.frameno-1) % 4) * 0.25f;
        float bx2 = bx1 + 0.25f;
        //printf("%f %f\n", tx1, tx2);
        float by1 = 0.0f + (float)((g.frameno-1) / 4) * 0.25f;
        float by2 = by1 + 0.25;
        
        float bw = 20;
        float bh = 25;
            glBegin(GL_QUADS);
                glTexCoord2f(bx1, by2); glVertex2f(-bw, -bh);
                glTexCoord2f(bx1, by1); glVertex2f(-bw,  bh);
                glTexCoord2f(bx2, by1); glVertex2f( bw,  bh);
                glTexCoord2f(bx2, by2); glVertex2f( bw, -bh);
            glEnd();
        //turn off alpha test
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
        glPopMatrix();

        //////// Gold Rock /////////////
        glPushMatrix();
        glColor3ub(255, 255, 255);
        //static float move = 1.0f;
        //glTranslatef(g.blhole[0].pos[0], g.blhole[0].pos[1], 0.0f);

        //glTranslatef(g.xres , 120.0f, 0.0f);
        //set alpha test
        //https://www.khronos.org/registry/OpenGL-Refpages/gl2.1
        ///xhtml/glAlphaFunc.xml
        //glEnable(GL_ALPHA_TEST);
        //transparent if alpha value is greater than 0.0
        //glAlphaFunc(GL_GREATER, 0.0f);
        //Set 4-channels of color intensity
        glColor4ub(255,255,255,255);

        //glBegin(GL_TRIANGLE_FAN)
        glBindTexture(GL_TEXTURE_2D, g.rockid);
        //make texture coordinates based on frame number.
        //float rx1 = 0.0f + (float)((g.frameno-1) % 1) * 1.0f;
        //float rx2 = rx1 + 1.0f;
        //printf("%f %f\n", tx1, tx2);
        //float ry1 = 0.0f + (float)((g.frameno-1) / 1) * 1.0f;
        //float ry2 = ry1 + 1.0;

        float rw = 10;
        float rh = 15;
            glBegin(GL_QUADS);
                glTexCoord2f(20, 0.5); glVertex2i(-rw, -rh);
                glTexCoord2f(20, 10); glVertex2i(-rw,  rh);
                glTexCoord2f(10, 15); glVertex2i( rw,  rh);
                glTexCoord2f(10, 15); glVertex2i( rw, -rh);
            glEnd();
        //turn off alpha test
        glBindTexture(GL_TEXTURE_2D, 0);
        //glDisable(GL_ALPHA_TEST);
        glPopMatrix();

    }
}

