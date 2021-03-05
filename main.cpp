#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <chrono>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdbe.h>

#define ROWS 40
#define COLS 75
#define PROB 3

class node {
	public:
	node* parent = nullptr;
	node* left = nullptr;
	node* right = nullptr;
	node* up = nullptr;
	node* down = nullptr;

	float gcost = 0.0;
	float hcost = 0.0;
	float fcost = 0.0;

	float x = -400;
	float y = -400;

	node()
	{
	}

	bool operator==(node n)
	{
		if(this->x == n.x && this->y == n.y &&
			this->gcost == n.gcost && this->hcost == n.hcost &&
			&(*(this->left)) == &(*(n.left)) && &(*(this->right)) == &(*(n.right)) &&
			&(*(this->up)) == &(*(n.up)) && &(*(this->down)) == &(*(n.down))) {
			
			return true;
		}

		return false;	
	}

	void set_left(node* n)
	{
		left = n;
	}
	void set_right(node* n)
	{
		right = n;
	}
	void set_up(node* n)
	{
		up = n;
	}
	void set_down(node* n)
	{
		down = n;
	}

} nodes[ROWS*COLS];

struct global {
	int xres = 1280;
	int yres = 1080;
	int hover = -1;
	int selected = -1;
	int target = -1;
	int show_path = 0;

	int shift_down = 0;
	node* path = new node[ROWS*COLS];
	int count = 0;

	~global()
	{
		delete[] path;
		path = NULL;
	}
} g;

class X11_wrapper {
private:
        Display *dpy;
        Window win;
        GC gc;
        XdbeBackBuffer backBuffer;
        XdbeSwapInfo swapInfo;
public:
        X11_wrapper() {
                int major, minor;
                XSetWindowAttributes attributes;
                XdbeBackBufferAttributes *backAttr;
                dpy = XOpenDisplay(NULL);
            //List of events we want to handle
                attributes.event_mask = ExposureMask | StructureNotifyMask |
                        PointerMotionMask | ButtonPressMask |
                        ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
                //Various window attributes
                attributes.backing_store = Always;
                attributes.save_under = True;
                attributes.override_redirect = False;
                attributes.background_pixel = 0x00000000;
                //Get default root window
                Window root = DefaultRootWindow(dpy);
                //Create a window
                win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
                                                    CopyFromParent, InputOutput, CopyFromParent,
                                                    CWBackingStore | CWOverrideRedirect | CWEventMask |
                                                        CWSaveUnder | CWBackPixel, &attributes);
                //Create gc
                gc = XCreateGC(dpy, win, 0, NULL);
                //Get back buffer and attributes (used for swapping)
                backBuffer = XdbeAllocateBackBufferName(dpy, win, XdbeUndefined);
                backAttr = XdbeGetBackBufferAttributes(dpy, backBuffer);
            	swapInfo.swap_window = backAttr->window;
                swapInfo.swap_action = XdbeUndefined;
                XFree(backAttr);
                //Map and raise window
		char s[256];
		sprintf(s, "NODE PATHS, SCREEN SIZE: %i X %i", g.xres, g.yres);
                setWindowTitle(s);
		XMapWindow(dpy, win);
                XRaiseWindow(dpy, win);
        }
        ~X11_wrapper() {
                //Do not change. Deallocate back buffer.
                if(!XdbeDeallocateBackBufferName(dpy, backBuffer)) {
                        fprintf(stderr,"Error : unable to deallocate back buffer.\n");
                }
                XFreeGC(dpy, gc);
                XDestroyWindow(dpy, win);
                XCloseDisplay(dpy);
        }
        void swapBuffers() {
                XdbeSwapBuffers(dpy, &swapInfo, 1);
                usleep(4000);
        }
        bool getPending() {
                return XPending(dpy);
        }
        void getNextEvent(XEvent *e) {
                XNextEvent(dpy, e);
        }
        void set_color_3i(int r, int g, int b) {
                unsigned long cref = 0L;
                cref += r;
                cref <<= 8;
                cref += g;
                cref <<= 8;
                cref += b;
                XSetForeground(dpy, gc, cref);
        }
        void setWindowTitle(char* t) {
                XStoreName(dpy, win, t);
        }
        void clear_screen() {
                //XClearWindow(dpy, backBuffer);
                XSetForeground(dpy, gc, 0x00050505);
                XFillRectangle(dpy, backBuffer, gc, 0, 0, g.xres, g.yres);
        }
        void drawString(int x, int y, const char *message) {
                XDrawString(dpy, backBuffer, gc, x, y, message, strlen(message));
        }
        void drawPoint(int x, int y) {
                XDrawPoint(dpy, backBuffer, gc, x, y);
        }
        void drawLine(int x0, int y0, int x1, int y1) {
                XDrawLine(dpy, backBuffer, gc, x0, y0, x1, y1);
        }
 	void drawRectangle(int x, int y, int w, int h) {
                //x,y is upper-left corner
                XDrawRectangle(dpy, backBuffer, gc, x, y, w, h);
        }
        void fillRectangle(int x, int y, int w, int h) {
                //x,y is upper-left corner
                XFillRectangle(dpy, backBuffer, gc, x, y, w, h);
        }
} x11;

void setup()
{
	float sx = 20.0f;
	float sy = 20.0f;
	float inc_x = 50.0f;
	float inc_y = 50.0f;
	float x = sx;
	float y = sy;

	int idx = 0;
	for(int i=0; i<ROWS; i++) {
		x = sx;
		for(int j=0; j<COLS; j++) {
			int skip = rand() % PROB;
			if(skip != 0) {
				nodes[idx].x = x;
				nodes[idx].y = y;
				if(ROWS == g.xres/2 && COLS == g.yres/2)
					g.target = idx;
			}
			x += inc_x;
			idx++;
		}
		y += inc_y;
	}

	idx = 0;
	for(int i=0; i<ROWS; i++) {
		for(int j=0; j<COLS; j++) {
			if(i < ROWS-1 && nodes[idx+COLS].x > 0)
				nodes[idx].set_down(&nodes[idx+COLS]);
			if(i > 0 && nodes[idx-COLS].x > 0)
				nodes[idx].set_up(&nodes[idx-COLS]);
			if(j > 0 && nodes[idx-1].x > 0)
				nodes[idx].set_left(&nodes[idx-1]);
			if(j < COLS-1 && nodes[idx+1].x > 0)
				nodes[idx].set_right(&nodes[idx+1]);
			idx++;
		}
	}
}

int check_keys(XEvent*);
void check_mouse(XEvent*);
void check_resize(XEvent*);
void physics();
void render();
void find_path();

int main() {
	srand(time(NULL));
	setup();
	int done = 0;
	while(!done) {
		while(x11.getPending()) {
			XEvent e;
			x11.getNextEvent(&e);
			check_resize(&e);
			check_mouse(&e);
			done = check_keys(&e);
		}
		physics();
		render();
		x11.swapBuffers();
	}

	return(0);
}

void check_resize(XEvent *e)
{
        //ConfigureNotify is sent when the window is resized.
        if (e->type != ConfigureNotify)
                return;
        XConfigureEvent xce = e->xconfigure;
        g.xres = xce.width;
        g.yres = xce.height;
	char s[256];
	sprintf(s, "NODE PATHS, SCREEN SIZE: %i X %i", g.xres, g.yres);
        x11.setWindowTitle(s);
}

int check_keys(XEvent *e)
{
        int key = XLookupKeysym(&e->xkey, 0);
	if(e->type == KeyRelease) {
		if(key == XK_Shift_L || key == XK_Shift_R) {
			g.shift_down = 0;
			return 0;
		}
	} else if(key == XK_Shift_L || key == XK_Shift_R) {
		g.shift_down = 1;
		return 0;
	}
        
	//a key was pressed
        switch (key) {
		case XK_f:
			find_path();
			break;
		case XK_c:
			g.show_path = 0;
			char s[256];
			sprintf(s, "NODE PATHS, SCREEN SIZE: %i X %i", g.xres, g.yres);
			x11.setWindowTitle(s);
			break;
		case XK_Escape:
			return(1);
			break;
	}

	return(0);
}

void check_mouse(XEvent *e)
{
        //Did the mouse move?
        //Was a mouse button clicked?
        static int savex = 0;
        static int savey = 0;
        //
        if (e->type == ButtonRelease) {
                return;
        }
        if (e->type == ButtonPress) {
		if (e->xbutton.button == 1 && g.shift_down) {
			g.target = -1;
			for(int i=0; i<ROWS*COLS; i++) {
				float xdiff = savex - nodes[i].x;
				float ydiff = savey - nodes[i].y;
				float dist = sqrt(xdiff*xdiff + ydiff*ydiff);
				if(dist <= 20.0)
					g.target = i;
			}
		}
		else if (e->xbutton.button==1) {
                        //Left button is down
			g.selected = -1;
			for(int i=0; i<ROWS*COLS; i++) {
				float xdiff = savex - nodes[i].x;
				float ydiff = savey - nodes[i].y;
				float dist = sqrt(xdiff*xdiff + ydiff*ydiff);
				if(dist <= 20.0)
					g.selected = i;
			}
                }
                if (e->xbutton.button==3) {
                        //Right button is down
			g.target = -1;
			for(int i=0; i<ROWS*COLS; i++) {
				float xdiff = savex - nodes[i].x;
				float ydiff = savey - nodes[i].y;
				float dist = sqrt(xdiff*xdiff + ydiff*ydiff);
				if(dist <= 20.0)
					g.target = i;
			}
                }
        }

	g.hover = -1;
	for(int i=0; i<ROWS*COLS; i++) {
		float xdiff = savex - nodes[i].x;
		float ydiff = savey - nodes[i].y;
		float dist = sqrt(xdiff*xdiff + ydiff*ydiff);
		if(dist <= 20.0)
			g.hover = i;
	}

        if (savex != e->xbutton.x || savey != e->xbutton.y) {
                //Mouse moved
                savex = e->xbutton.x;
                savey = e->xbutton.y;
        }
}

void add_node(node* list, int &count, node obj)
{
	list[count] = obj;
	count++;
}

void pop_node(node* list, int &count, node n)
{
	bool found = false;
	for(int i=0; i<count; i++) {
		if((list[i] == n) || found) {
			list[i] = list[i+1];
			found = true;
		}
	}
	count--;
}

float get_dist(node n1, node n2)
{
	float xdiff = n1.x - n2.x;
	float ydiff = n1.y - n2.y;
	return(sqrt(xdiff*xdiff + ydiff*ydiff));
}

void build_path(node n)
{
	while(n.parent != nullptr) {
		g.path[g.count] = *(n.parent);
		n = *n.parent;
		g.count++;
	}
}

void find_path()
{
	if((g.target == -1 || g.selected == -1) ||
			g.show_path) {
		return;
	}

	g.show_path = 1;
	bool do_break = false;
	static int first_time = 1;
	node* open = new node[ROWS*COLS];
	node* close = new node[ROWS*COLS];
	int close_count = 0;
	int open_count = 1;
	open[0] = nodes[g.selected];
	//std::cout << &open << " " << &nodes << std::endl;

	open[0].gcost = 0;
	open[0].hcost = get_dist(open[0], nodes[g.target]);
	open[0].fcost = open[0].gcost + open[0].hcost;

	x11.set_color_3i(255, 255, 255);

	while(open_count > 0) {
		int current;
		float min = 99999999.00;
		for(int i=0; i<open_count; i++) {
			if(open[i].fcost < min) {
				min = open[i].fcost;
				current = i;
			}
		}
		if(open[current] == nodes[g.target]) {
			build_path(open[current]);
			break;
		}
		
		node* children = new node[4];
		if(open[current].left != nullptr)
			children[0] = *open[current].left;
		else
			memset(children, '\0', sizeof(node));
		if(open[current].right != nullptr)
			children[1] = *open[current].right;
		else
			memset(children+1, '\0', sizeof(node));
		if(open[current].up != nullptr)
			children[2] = *open[current].up;
		else
			memset(children+2, '\0', sizeof(node));
		if(open[current].down != nullptr)
			children[3] = *open[current].down;
		else
			memset(children+3, '\0', sizeof(node));

		for(int i=0; i<4; i++) {
			if(children+i == nullptr) {
				printf("NULL\n");
				continue;
			}
			bool in_close = false;
			for(int j=0; j<close_count; j++) {
				if(children[i] == close[j]) {
					in_close = true;
					break;
				}
			}
			if(in_close)
				continue;

			bool in_open = false;
			children[i].parent = &open[current];
			float cost = open[current].gcost + get_dist(open[current], children[i]);
			children[i].fcost = cost + get_dist(children[i], nodes[g.target]);
			int j;
			for(j=0; j<open_count; j++) {
				if(open[j] == children[i]) {
					in_open = true;
					break;
				}
			}
			if(!in_open) {
				children[i].parent = new node;
				*children[i].parent = open[current];
				add_node(open, open_count, children[i]);
			}
			else if(in_open) {
				if(open[j].gcost > children[i].gcost) {
					open[j].gcost = children[i].gcost;
					open[j].parent = children[i].parent;
				}
			}
		}
		children = NULL;
	
		add_node(close, close_count, open[current]);
		pop_node(open, open_count, open[current]);

		if(do_break)
			break;


/*		x11.set_color_3i(0, 255, 100);
		for(int i=0; i<open_count; i++) {
			x11.fillRectangle(open[i].x-20, open[i].y-20, 40, 40);
			x11.swapBuffers();
		}
		x11.set_color_3i(200, 100, 100);
		for(int i=0; i<close_count; i++) {
			x11.fillRectangle(close[i].x-20, close[i].y-20, 40, 40);
			x11.swapBuffers();
		}*/
	}

	if(g.count == 0) {
		// No path found
		char s[] = {"No path found :("};
		x11.setWindowTitle(s);
		g.show_path = 0;
	} else {
		// Path found
		x11.set_color_3i(255, 255, 255);
		for(int i=0; i<g.count-1; i++) {
			x11.fillRectangle(g.path[i].x-20, g.path[i].y-20, 40, 40);
			x11.swapBuffers();
		}
		delete[] open;
		delete[] close;
		open = nullptr;
		close = nullptr;
		g.count = 0;
		
		char s[] = {"PRESS 'C' TO CLEAR PATH"};
		//printf("Exited loop\n");
		x11.setWindowTitle(s);
	}
}

void physics() {}

void render()
{
	if(g.show_path)
		return;
	x11.clear_screen();
	for(int i=0; i<COLS*ROWS; i++) {
		if(nodes[i].x < 0)
			continue;
		x11.set_color_3i(0, 0, 255);
		if(nodes[i].right != NULL)
			x11.drawLine(nodes[i].x, nodes[i].y,
					nodes[i].right->x, nodes[i].right->y);
		if(nodes[i].down != NULL)
			x11.drawLine(nodes[i].x, nodes[i].y,
					nodes[i].down->x, nodes[i].down->y);
		if(i == g.hover) {
			x11.set_color_3i(0, 255, 0);
			if(g.shift_down)
				x11.set_color_3i(165, 0, 0);	
		}
		if(i == g.selected)
			x11.set_color_3i(255, 165, 0);
		if(i == g.target)
			x11.set_color_3i(255, 0, 0);
		x11.fillRectangle(nodes[i].x-20, nodes[i].y-20, 40, 40);
	}
}
