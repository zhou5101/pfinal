/* compute optimal solutions for sliding block puzzle. */
#include <SDL.h>
//#include <SDL_ttf.h>
#include <stdio.h>
#include <cstdlib>   /* for atexit() */
#include <algorithm>
using std::swap;
#include <cassert>
#include <vector>
using std::vector;
#include <deque>
using std::deque;
#include <map>
using std::map;
#include <set>
using std::set;
#include<iostream>;
using std::cout;
using std::find;
typedef vector<vector<int> > graph;

/* SDL reference: https://wiki.libsdl.org/CategoryAPI */

/* initial size; will be set to screen size after window creation. */
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
int fcount = 0;
int mousestate = 0;
SDL_Point lastm = {0,0}; /* last mouse coords */
SDL_Rect bframe; /* bounding rectangle of board */
static const int ep = 2; /* epsilon offset from grid lines */

bool init(); /* setup SDL */
void initBlocks();

#define FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP
// #define FULLSCREEN_FLAG 0

/* NOTE: ssq == "small square", lsq == "large square" */
enum bType {em,hor,ver,ssq,lsq};
struct block {
	SDL_Rect R; /* screen coords + dimensions */
	bType type; /* shape + orientation */
	/* TODO: you might want to add other useful information to
	 * this struct, like where it is attached on the board.
	 * (Alternatively, you could just compute this from R.x and R.y,
	 * but it might be convenient to store it directly.) */
	void rotate() /* rotate rectangular pieces */
	{
		if (type != hor && type != ver) return;
		type = (type==hor)?ver:hor;
		swap(R.w,R.h);
	}
};

#define NBLOCKS 10
block B[NBLOCKS];
block* dragged = NULL;

block* findBlock(int x, int y);
void close(); /* call this at end of main loop to free SDL resources */
SDL_Window* gWindow = 0; /* main window */
SDL_Renderer* gRenderer = 0;

bool init()
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL_Init failed.  Error: %s\n", SDL_GetError());
		return false;
	}
	/* NOTE: take this out if you have issues, say in a virtualized
	 * environment: */
	if(!SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1")) {
		printf("Warning: vsync hint didn't work.\n");
	}
	/* create main window */
	gWindow = SDL_CreateWindow("Sliding block puzzle solver",
								SDL_WINDOWPOS_UNDEFINED,
								SDL_WINDOWPOS_UNDEFINED,
								SCREEN_WIDTH, SCREEN_HEIGHT,
								SDL_WINDOW_SHOWN|FULLSCREEN_FLAG);
	if(!gWindow) {
		printf("Failed to create main window. SDL Error: %s\n", SDL_GetError());
		return false;
	}
	/* set width and height */
	SDL_GetWindowSize(gWindow, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	/* setup renderer with frame-sync'd drawing: */
	gRenderer = SDL_CreateRenderer(gWindow, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(!gRenderer) {
		printf("Failed to create renderer. SDL Error: %s\n", SDL_GetError());
		return false;
	}
	SDL_SetRenderDrawBlendMode(gRenderer,SDL_BLENDMODE_BLEND);

	initBlocks();
	return true;
}

/* TODO: you'll probably want a function that takes a state / configuration
 * and arranges the blocks in accord.  This will be useful for stepping
 * through a solution.  Be careful to ensure your underlying representation
 * stays in sync with what's drawn on the screen... */

void initBlocks()
{
	int& W = SCREEN_WIDTH;
	int& H = SCREEN_HEIGHT;
	int h = H * 3 / 4;
	int w = 4 * h / 5;
	int u = h / 5 - 2 * ep;
	int mw = (W - w) / 2;
	int mh = (H - h) / 2;

	/* setup bounding rectangle of the board: */
	bframe.x = (W - w) / 2; //x-coordinate of large retange in surround the grid
	bframe.y = (H - h) / 2;//y-x-coordinate of large retange in surround the grid
	bframe.w = w; //width of board
	bframe.h = h; //height of board

	int move = bframe.x / 4 + ep; //one grid 

	/* NOTE: there is a tacit assumption that should probably be
	 * made explicit: blocks 0--4 are the rectangles, 5-8 are small
	 * squares, and 9 is the big square.  This is assumed by the
	 * drawBlocks function below. */
	 /*rectangles*/
	for (size_t i = 0; i < 5; i++) {
		B[i].R.x = (mw - 2 * u) / 2;
		B[i].R.y = mh + (i + 1) * (u / 5) + i * u;
		B[i].R.w = 2 * (u + ep);
		B[i].R.h = u;
		B[i].type = hor;
	}
	B[4].R.x = mw + ep; //origin of the board (0,0)
	B[4].R.y = mh + ep;
	B[4].R.w = 2 * (u + ep);
	B[4].R.h = u;
	B[4].type = hor;

	/* small squares */
	for (size_t i = 0; i < 4; i++) {
		B[i + 5].R.x = (W + w) / 2 + (mw - 2 * u) / 2 + (i % 2) * (u + u / 5);
		B[i + 5].R.y = mh + ((i / 2) + 1) * (u / 5) + (i / 2) * u;
		B[i + 5].R.w = u;
		B[i + 5].R.h = u;
		B[i + 5].type = ssq;
	}
	B[9].R.x = B[5].R.x + u / 10;
	B[9].R.y = B[7].R.y + u + 2 * u / 5;
	B[9].R.w = 2 * (u + ep);
	B[9].R.h = 2 * (u + ep);
	B[9].type = lsq;

	//initial configuration
	for (size_t i = 0; i < 10; i++) {
		if (i < 4)
			B[i].rotate();
		B[i].R.x = mw + ep;
		B[i].R.y = mh + ep;
	}
	B[1].R.x += 3 * move + ep;

	B[2].R.y += 3 * move + ep;

	B[3].R.x += 3 * move + ep;
	B[3].R.y += 3 * move + ep;

	B[4].R.x += move + ep;
	B[4].R.y += 2 * move + ep;

	B[5].R.x += move + ep;
	B[5].R.y += 3 * move + ep;

	B[6].R.x += 2 * move + ep;
	B[6].R.y += 3 * move + ep;

	B[7].R.x += move + ep;
	B[7].R.y += 4 * move + ep;

	B[8].R.x += 2 * move + ep;
	B[8].R.y += 4 * move + ep;

	B[9].R.x += move + ep;

	/*for (int i = 0; i < 10; i++) {
		printf("Default B[ %i ].r.x: %i\n", i, B[i].R.x);
		printf("Default B[ %i ].r.y: %i\n", i, B[i].R.y);
	}*/
	/*printf("bframe.x: %i\n", bframe.x);
	printf("B[1] x-coordinate: %i\n", B[1].R.x);*/
	printf("move: %i\n", move);

}

void snap(block* b)
{
	/* TODO: once you have established a representation for configurations,
	 * you should update this function to make sure the configuration is
	 * updated when blocks are placed on the board, or taken off.  */
assert(b != NULL);
/* upper left of grid element (i,j) will be at
 * bframe.{x,y} + (j*bframe.w/4,i*bframe.h/5) */
 /* translate the corner of the bounding box of the board to (0,0). */

int x = b->R.x - bframe.x;
int y = b->R.y - bframe.y;
int uw = bframe.w / 4;
int uh = bframe.h / 5;
/* NOTE: in a perfect world, the above would be equal. */
int i = (y + uh / 2) / uh; /* row */
int j = (x + uw / 2) / uw; /* col */
if (0 <= i && i < 5 && 0 <= j && j < 4) {
	b->R.x = bframe.x + j * uw + ep;
	b->R.y = bframe.y + i * uh + ep;
}
}

int ycoordinate(block b) //convert bframe.x to row of 2d array
{
	int x = (b.R.x - bframe.x) / (bframe.w / 4);
	return x;
}
int xcoordinate(block b)//convert bframe.y to column of 2d array
{
	int y = (b.R.y - bframe.y) / (bframe.w / 4);
	return y;
}
struct coordinate {
	int x, y;
	coordinate(int r, int c) :x(r), y(c) {} //row and col
};

void initialG(graph& g) { //read graph
	for (int i = 0; i < 10; i++) {
		int r = xcoordinate(B[i]);
		int c = ycoordinate(B[i]);
		if (g[r][c] == -1) {
			g[r][c] = i;
			if (B[i].R.w > B[i].R.h) {
				g[r][c + 1] = i;
			}
			else if (B[i].R.w < B[i].R.h) {
				g[r + 1][c] = i;
			}
			else {
				if (B[i].type == lsq) {
					g[r + 1][c] = i;
					g[r][c + 1] = i;
					g[r + 1][c + 1] = i;
				}
			}
		}
	}
}

void printG(const graph& g) { //print graph
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 4; j++) {
			printf("[%i]", g[i][j]);
		}
		printf("\n");
	}
}

bool goal(const graph& g) { //check the state of the solution
	return (g[4][1] == 9 && g[4][2] == 9);
}
void mapG(const graph& g, map<int, vector<coordinate>>& c) {
	c.clear();
	for (int i = 0; i < 5; i++)
		for (int j = 0; j < 4; j++)
			c[g[i][j]].push_back(coordinate(i, j));
}

void neighbor(const graph& g, set<int>& n) {
	n.clear();
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 4; j++) {
			if (g[i][j] == -1) {
				if (i - 1 >= 0)
					if (g[i - 1][j]>-1)
						n.insert(g[i - 1][j]);
				if (i + 1 <= 4)
					if (g[i + 1][j] > -1)
						n.insert(g[i + 1][j]);
				if (j - 1 >= 0)
					if (g[i][j - 1] > -1)
						n.insert(g[i][j - 1]);
				if (j + 1 <= 3)
					if (g[i][j + 1] > -1)
						n.insert(g[i][j + 1]);
			}
		}
	}
}
vector<vector<int>> dir = { {0,1}, {0,-1}, {1,0}, {-1,0}, {0,2}, {0,-2}, {2,0}, {-2,0} };
bool validM(int& row, int& col, vector<coordinate>& c, const int& n, const graph& cp) {
	for (auto i = c.begin(); i != c.end(); i++) {
		if ((i->x + row) > -1 && (i->x + row) < 5 && (i->y + col) > -1 && (i->y + col) < 4)
			if (cp[i->x + row][i->y + col] == -1 || cp[i->x + row][i->y + col] == n)
				continue;
			else
				return false;
		else
			return false;
	}
	return true;
}
void childG(const graph& g, const int& n, map<int, vector<coordinate>>& c, deque<graph>&q, map<graph,graph>& p) {
	graph cp = g;
	int counter = 0;
	for (int j = 0; j < 8; j++, counter++) {
		int row = dir[j][0], col = dir[j][1];
		if (validM(row,col,c[n],n,cp)) {
			for (auto i = c[n].begin(); i != c[n].end(); i++) {
				cp[i->x][i->y] = -1;
			}
			for (auto i = c[n].begin(); i != c[n].end(); i++) {
				cp[i->x + row][i->y + col] = n;
			}
			if (find(q.begin(), q.end(), cp) == q.end()) {
				q.push_back(cp);
				p.insert({ cp,g });
			}
			cp = g;
		}
	}
		
}

map<graph, graph> bfs(deque<graph>& q, const graph& g, map<int, vector<coordinate>>& c, set<int>& n) {
	map<graph, graph> p;
	c.clear();
	n.clear();
	q.clear();
	q.push_back(g);
	graph cp;
	while(!q.empty())
	{	cp = q.front();

		q.pop_front();
		neighbor(cp, n);
		mapG(cp, c);
		if (goal(cp)) {
			graph mark(0);
			p.insert({ mark,cp });
			return p;
		}
		for (auto i = n.begin(); i != n.end(); i++) {
			int z = *i;
			for (int j = 0; j < 8; j++) {
				int row = dir[j][0], col = dir[j][1];
				if (validM(row, col, c[*i], *i, cp)) {
					for (auto i = c[z].begin(); i != c[z].end(); i++) {
						cp[i->x][i->y] = -1;
					}
					for (auto i = c[z].begin(); i != c[z].end(); i++) {
						cp[i->x + row][i->y + col] = z;
					}
					if (find(q.begin(), q.end(), cp) == q.end()) {
						q.push_back(cp);
						p.insert({ cp,g });
					}
				}
			}
		}
		
	}
	
}



void drawBlocks()
{
	/* rectangles */
	SDL_SetRenderDrawColor(gRenderer, 0x43, 0x4c, 0x5e, 0xff);
	for (size_t i = 0; i < 5; i++) {
		SDL_RenderFillRect(gRenderer,&B[i].R);
	}
	/* small squares */
	SDL_SetRenderDrawColor(gRenderer, 0x5e, 0x81, 0xac, 0xff);
	for (size_t i = 5; i < 9; i++) {
		SDL_RenderFillRect(gRenderer,&B[i].R);
	}
	/* large square */
	SDL_SetRenderDrawColor(gRenderer, 0xa3, 0xbe, 0x8c, 0xff);
	SDL_RenderFillRect(gRenderer,&B[9].R);
}

/* return a block containing (x,y), or NULL if none exists. */
block* findBlock(int x, int y)
{
	/* NOTE: we go backwards to be compatible with z-order */
	for (int i = NBLOCKS-1; i >= 0; i--) {
		if (B[i].R.x <= x && x <= B[i].R.x + B[i].R.w &&
				B[i].R.y <= y && y <= B[i].R.y + B[i].R.h)
			return (B+i);
	}
	return NULL;
}

void close()
{
	SDL_DestroyRenderer(gRenderer); gRenderer = NULL;
	SDL_DestroyWindow(gWindow); gWindow = NULL;
	SDL_Quit();
}

void render()
{
	/* draw entire screen to be black: */
	SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xff);
	SDL_RenderClear(gRenderer);

	/* first, draw the frame: */
	int& W = SCREEN_WIDTH;
	int& H = SCREEN_HEIGHT;
	int w = bframe.w;
	int h = bframe.h;
	SDL_SetRenderDrawColor(gRenderer, 0x39, 0x39, 0x39, 0xff);
	SDL_RenderDrawRect(gRenderer, &bframe);
	/* make a double frame */
	SDL_Rect rframe(bframe);
	int e = 3;
	rframe.x -= e; 
	rframe.y -= e;
	rframe.w += 2*e;
	rframe.h += 2*e;
	SDL_RenderDrawRect(gRenderer, &rframe);

	/* draw some grid lines: */
	SDL_Point p1,p2;
	SDL_SetRenderDrawColor(gRenderer, 0x19, 0x19, 0x1a, 0xff);
	/* vertical */
	p1.x = (W-w)/2;
	p1.y = (H-h)/2;
	p2.x = p1.x;
	p2.y = p1.y + h;
	for (size_t i = 1; i < 4; i++) {
		p1.x += w/4;
		p2.x += w/4;
		SDL_RenderDrawLine(gRenderer,p1.x,p1.y,p2.x,p2.y);
	}
	/* horizontal */
	p1.x = (W-w)/2;
	p1.y = (H-h)/2;
	p2.x = p1.x + w;
	p2.y = p1.y;
	for (size_t i = 1; i < 5; i++) {
		p1.y += h/5;
		p2.y += h/5;
		SDL_RenderDrawLine(gRenderer,p1.x,p1.y,p2.x,p2.y);
	}
	SDL_SetRenderDrawColor(gRenderer, 0xd8, 0xde, 0xe9, 0x7f);
	SDL_Rect goal = {bframe.x + w/4 + ep, bframe.y + 3*h/5 + ep,
	                 w/2 - 2*ep, 2*h/5 - 2*ep};
	SDL_RenderDrawRect(gRenderer,&goal);

	/* now iterate through and draw the blocks */
	drawBlocks();
	/* finally render contents on screen, which should happen once every
	 * vsync for the display */
	SDL_RenderPresent(gRenderer);
}


int main(int argc, char *argv[])
{
	/* TODO: add option to specify starting state from cmd line? */
	/* start SDL; create window and such: */
	if(!init()) {
		printf( "Failed to initialize from main().\n" );
		return 1;
	}
	atexit(close);
	bool quit = false; /* set this to exit main loop. */
	SDL_Event e;

	graph g(5, vector<int>(4, -1)); //matrix stores configuration of graph
	set<int> n; //store neighbor of the empty block for use of move block
	map<int, vector<coordinate>> c; //record the cordinate and use for move block
	deque<graph> q; //store all possible configuration of graph;
	map<graph, graph> p; //for tracking parent of graph

	initialG(g);
	printG(g);


	/* main loop: */
	while(!quit) {
		/* handle events */
		while(SDL_PollEvent(&e) != 0) {
			/* meta-q in i3, for example: */
			if(e.type == SDL_MOUSEMOTION) {
				if (mousestate == 1 && dragged) {
					int dx = e.button.x - lastm.x;
					int dy = e.button.y - lastm.y;
					lastm.x = e.button.x;
					lastm.y = e.button.y;
					dragged->R.x += dx;
					dragged->R.y += dy;
				}
			} else if (e.type == SDL_MOUSEBUTTONDOWN) {
				if (e.button.button == SDL_BUTTON_RIGHT) {
					block* b = findBlock(e.button.x,e.button.y);
					if (b) b->rotate();
				} else {
					mousestate = 1;
					lastm.x = e.button.x;
					lastm.y = e.button.y;
					dragged = findBlock(e.button.x,e.button.y);
				}
				/* XXX happens if during a drag, someone presses yet
				 * another mouse button??  Probably we should ignore it. */
			} else if (e.type == SDL_MOUSEBUTTONUP) {
				if (e.button.button == SDL_BUTTON_LEFT) {
					mousestate = 0;
					lastm.x = e.button.x;
					lastm.y = e.button.y;
					if (dragged) {
						/* snap to grid if nearest location is empty. */
						snap(dragged);
					}
					dragged = NULL;
				}
			} else if (e.type == SDL_QUIT) {
				quit = true;
			} else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
					case SDLK_ESCAPE:
					case SDLK_q:
						quit = true;
						break;
					case SDLK_LEFT:
						/* TODO: show previous step of solution */
						break;
					case SDLK_RIGHT:
						/* TODO: show next step of solution */
						break;
					case SDLK_p:
						/* TODO: print the state to stdout
						 * (maybe for debugging purposes...) */
						break;
					case SDLK_s:
						/* TODO: try to find a solution */
						break;
					default:
						break;
				}
			}
		}
		fcount++;
		render();
	}
	printf("total frames rendered: %i\n",fcount);
	p = bfs(q, g, c, n);
	cout << "\nsize: " << p.size() << '\n';
	return 0;
}
