/* compute optimal solutions for sliding block puzzle. */
#define ROW 5
#define COL 4
#include <SDL.h>
#include <stdio.h>
#include <cstdlib>
#include <algorithm>
using std::swap;
using namespace std;
#include <cassert>
#include <unordered_set>
using std::unordered_set;
#include <map>
using std::map;
#include <deque>
using std::deque;
#include <vector>
using std::vector;
using std::find;
#include<iostream>
using std::cout;
#include <chrono>
#include <thread>
#include <inttypes.h>



/* SDL reference: https://wiki.libsdl.org/CategoryAPI */

/* initial size; will be set to screen size after window creation. */
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
int fcount = 0;
int mousestate = 0;
SDL_Point lastm = { 0,0 }; /* last mouse coords */
SDL_Rect bframe; /* bounding rectangle of board */
static const int ep = 2; /* epsilon offset from grid lines */

bool init(); /* setup SDL */
void initBlocks();

//#define FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP
#define FULLSCREEN_FLAG 0

/* NOTE: ssq == "small square", lsq == "large square" */
enum bType { hor, ver, ssq, lsq };
struct block {
	SDL_Rect R; /* screen coords + dimensions */
	bType type; /* shape + orientation */
	/* TODO: you might want to add other useful information to
	 * this struct, like where it is attached on the board.
	 * (Alternatively, you could just compute this from R.x and R.y,
	 * but it might be convenient to store it directly.) */
	int r, c; //block coordinates
	int p;//index in bit 
	void rotate() /* rotate rectangular pieces */
	{
		if (type != hor && type != ver) return;
		type = (type == hor) ? ver : hor;
		swap(R.w, R.h);
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
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL_Init failed.  Error: %s\n", SDL_GetError());
		return false;
	}
	/* NOTE: take this out if you have issues, say in a virtualized
	 * environment: */
	if (!SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1")) {
		printf("Warning: vsync hint didn't work.\n");
	}
	/* create main window */
	gWindow = SDL_CreateWindow("Sliding block puzzle solver",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN | FULLSCREEN_FLAG);
	if (!gWindow) {
		printf("Failed to create main window. SDL Error: %s\n", SDL_GetError());
		return false;
	}
	/* set width and height */
	SDL_GetWindowSize(gWindow, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	/* setup renderer with frame-sync'd drawing: */
	gRenderer = SDL_CreateRenderer(gWindow, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!gRenderer) {
		printf("Failed to create renderer. SDL Error: %s\n", SDL_GetError());
		return false;
	}
	SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);

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
	bframe.x = (W - w) / 2;
	bframe.y = (H - h) / 2;
	bframe.w = w;
	bframe.h = h;

	/* NOTE: there is a tacit assumption that should probably be
	 * made explicit: blocks 0--4 are the rectangles, 5-8 are small
	 * squares, and 9 is the big square.  This is assumed by the
	 * drawBlocks function below. */

	for (size_t i = 0; i < 5; i++) {
		B[i].R.x = (mw - 2 * u) / 2;
		B[i].R.y = mh + (i + 1) * (u / 5) + i * u;
		B[i].R.w = 2 * (u + ep);
		B[i].R.h = u;
		B[i].type = hor;
	}
	B[4].R.x = mw + ep;
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
	for (int i = 0; i < 10; i++) {
		if (i < 4)
			B[i].rotate();
	}
	int uw = bframe.w / COL;
	int uh = bframe.h / ROW;
	//VER
	B[0].r = 1;
	B[0].c = 0;

	B[1].r = 1;
	B[1].c = 1;

	B[2].r = 3;
	B[2].c = 0;

	B[3].r = 3;
	B[3].c = 1;
	//HOR
	B[4].r = 2;
	B[4].c = 2;
	//SSQ
	B[5].r = 0;
	B[5].c = 0;

	B[6].r = 0;
	B[6].c = 1;

	B[7].r = 3;
	B[7].c = 2;

	B[8].r = 3;
	B[8].c = 3;
	//LSQ
	B[9].r = 0;
	B[9].c = 2;

	for (int i = 0, j =19; i < 10; i++, j-=2) {
		B[i].R.x = bframe.x + B[i].c * uw + ep;
		B[i].R.y = bframe.y + B[i].r * uh + ep;
		B[i].p = j;
	}

}

void drawBlocks()
{
	/* rectangles */
	SDL_SetRenderDrawColor(gRenderer, 0x43, 0x4c, 0x5e, 0xff);
	for (size_t i = 0; i < 5; i++) {
		SDL_RenderFillRect(gRenderer, &B[i].R);
	}
	/* small squares */
	SDL_SetRenderDrawColor(gRenderer, 0x5e, 0x81, 0xac, 0xff);
	for (size_t i = 5; i < 9; i++) {
		SDL_RenderFillRect(gRenderer, &B[i].R);
	}
	/* large square */
	SDL_SetRenderDrawColor(gRenderer, 0xa3, 0xbe, 0x8c, 0xff);
	SDL_RenderFillRect(gRenderer, &B[9].R);
}

/* return a block containing (x,y), or NULL if none exists. */
block* findBlock(int x, int y)
{
	/* NOTE: we go backwards to be compatible with z-order */
	for (int i = NBLOCKS - 1; i >= 0; i--) {
		if (B[i].R.x <= x && x <= B[i].R.x + B[i].R.w &&
			B[i].R.y <= y && y <= B[i].R.y + B[i].R.h)
			return (B + i);
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
	rframe.w += 2 * e;
	rframe.h += 2 * e;
	SDL_RenderDrawRect(gRenderer, &rframe);

	/* draw some grid lines: */
	SDL_Point p1, p2;
	SDL_SetRenderDrawColor(gRenderer, 0x19, 0x19, 0x1a, 0xff);
	/* vertical */
	p1.x = (W - w) / 2;
	p1.y = (H - h) / 2;
	p2.x = p1.x;
	p2.y = p1.y + h;
	for (size_t i = 1; i < 4; i++) {
		p1.x += w / 4;
		p2.x += w / 4;
		SDL_RenderDrawLine(gRenderer, p1.x, p1.y, p2.x, p2.y);
	}
	/* horizontal */
	p1.x = (W - w) / 2;
	p1.y = (H - h) / 2;
	p2.x = p1.x + w;
	p2.y = p1.y;
	for (size_t i = 1; i < 5; i++) {
		p1.y += h / 5;
		p2.y += h / 5;
		SDL_RenderDrawLine(gRenderer, p1.x, p1.y, p2.x, p2.y);
	}
	SDL_SetRenderDrawColor(gRenderer, 0xd8, 0xde, 0xe9, 0x7f);
	SDL_Rect goal = { bframe.x + w / 4 + ep, bframe.y + 3 * h / 5 + ep,
					 w / 2 - 2 * ep, 2 * h / 5 - 2 * ep };
	SDL_RenderDrawRect(gRenderer, &goal);

	/* now iterate through and draw the blocks */
	drawBlocks();
	/* finally render contents on screen, which should happen once every
	 * vsync for the display */
	SDL_RenderPresent(gRenderer);
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
	int uw = bframe.w / COL;
int uh = bframe.h / ROW;
/* NOTE: in a perfect world, the above would be equal. */
int i = (y + uh / 2) / uh; /* row */
int j = (x + uw / 2) / uw; /* col */
if (0 <= i && i < ROW && 0 <= j && j < COL) {
	b->R.x = bframe.x + j * uw + ep;
	b->R.y = bframe.y + i * uh + ep;
	b->r = i;
	b->c = j;
}
else {
	b->r = ROW;
	b->c = COL;
}
}

class solver {
public:
	bool isFound(uint64_t s) {
		int y = s & 7;
		int x = (s >> 3) & 7;
		return x == 3 && y == 1;
	}

	void readG() {
		state = 0;
		for (int i = 0; i < NBLOCKS; i++) {
			state <<= 3;
			state |= B[i].r;
			state <<= 3;
			state |= B[i].c;
		}
	}

	void simpleG() {
		clear();
		uint64_t cp = state;
		for (int i = 0; i < NBLOCKS; i++) {
			int r, c;
			r = (cp>>(B[i].p*3)) & 7;
			c = (cp >> ((B[i].p-1) * 3)) & 7;
			//printf("i: %i, r: %i, c: %i\n", i, r, c);
			
				graph[r][c] = 1;
				if (B[i].type == ver) {
					graph[r + 1][c] = 1;
				}
				else if (B[i].type == hor) {
					graph[r][c + 1] = 1;
				}
				else if (B[i].type == lsq) {
					graph[r + 1][c] = 1;
					graph[r][c + 1] = 1;
					graph[r + 1][c + 1] = 1;
				}
		
		}
	}

	void printG() {
		for (int i = 0; i < ROW; i++) {
			for (int j = 0; j < COL; j++)
				printf("[%d]", graph[i][j]);
			printf("\n");
		}
	}

	void printState() {
		printf("Current State of Graph: " "%" PRIu64 "\n", state);
	}

	void clear() {
		for (int i = 0; i < ROW; i++) {
			for (int j = 0; j < COL; j++)
				graph[i][j] = 0;
		}
	}

	void sToB(uint64_t s) {//convert uint64_t value to real configuration
		int uw = bframe.w / COL;
		int uh = bframe.h / ROW;
		for (int i = 0, j = 19; i < NBLOCKS && j >= 1; i++, j -= 2) {
			B[i].r = (s >> (j * 3)) & 7;
			B[i].c = (s >> ((j - 1) * 3)) & 7;
			B[i].R.x = bframe.x + B[i].c * uw + ep;
			B[i].R.y = bframe.y + B[i].r * uh + ep;
		}

	}

	void moveBlock() {
		for (int i = 0; i < NBLOCKS; i++) {
			if (B[i].type == ssq)
				moveSsq(i);
			else if (B[i].type == ver)
				moveVer(i);
			else if (B[i].type == hor)
				moveHor(i);
			else if (B[i].type == lsq)
				moveLsq(i);
		}
	}
	
	void moveSsq(int i) {
		int r, c;
		uint64_t cp = 0, eraser = 0, newRC = 0;
		r = (state >> (B[i].p * 3)) & 7;
		c = (state >> ((B[i].p - 1) * 3)) & 7;
		graph[r][c] = 0;

	/*	createSsq(i, r, c, 0, 1);
		createSsq(i, r, c, 0, -1);
		createSsq(i, r, c, 1, 0);
		createSsq(i, r, c, -1, 0);
		createSsq(i, r, c, 0, 2);
		createSsq(i, r, c, 0, -2);
		createSsq(i, r, c, 2, 0);
		createSsq(i, r, c, -2, 0);*/

		for (int d = 0, counter = 0; d < 8; d++, counter++)
		{
			if (d > 3 && counter < 1)break;
			int x = dir[d][0], y = dir[d][1];
			if (r + x >= 0 && r + x < ROW && c + y < COL && c + y >= 0) {
				if (graph[r + x][c + y] == 0) {
					cp = state;
					eraser = 0;
					newRC = 0;
					eraser |= 63;
					eraser <<= ((B[i].p - 1) * 3);
					
					eraser = ~eraser;
					cp &= eraser;

					/*printf("Initial r: %i, c: %i \n", r, c);
					printf("After r+x: %i, c+y: %i \n", r + x, c + y);
					printf("Eraser: %" PRId64"\n", eraser);*/
					newRC |= (r + x);
					newRC <<= 3;
					newRC |= (c + y);
					newRC <<= ((B[i].p - 1) * 3);
					cp |= newRC;
					if (visited.find(cp) == visited.end())
					{
						q.push_back(cp);
						p[cp] = state;
						visited.insert(cp);
					}

				}
			}

		}

		graph[r][c] = 1;
	}

	void moveHor(int i) { //hor rectangle --
		int r, c;
		uint64_t cp = 0, eraser = 0, newRC = 0;
		r = (state >> (B[i].p * 3)) & 7;
		c = (state >> ((B[i].p - 1) * 3)) & 7;

		graph[r][c] = 0;
		graph[r][c + 1] = 0;
		
		for (int d = 0, counter = 0; d < 8; d++, counter++)
		{
			if (d > 3 && counter < 1)break;
			int x = dir[d][0], y = dir[d][1];
			if (r + x >= 0 && r + x < ROW && c + y < COL && c + y >= 0) {
				if (graph[r + x][c + y] == 0 && graph[r + x][c + y + 1] == 0) {
					cp = state;
					eraser = 0;
					newRC = 0;
					eraser |= 63;
					eraser <<= ((B[i].p - 1) * 3);

					eraser = ~eraser;
					cp &= eraser;

					/*printf("Initial r: %i, c: %i \n", r, c);
					printf("After r+x: %i, c+y: %i \n", r + x, c + y);
					printf("Eraser: %" PRId64"\n", eraser);*/
					newRC |= (r + x);
					newRC <<= 3;
					newRC |= (c + y);
					newRC <<= ((B[i].p - 1) * 3);
					cp |= newRC;
					
					if (visited.find(cp) == visited.end())
					{
						q.push_back(cp);
						p[cp] = state;
						visited.insert(cp);
						
					}

				}
			}
		}

		graph[r][c] = 1;
		graph[r][c + 1] = 1;
	}

	void moveVer(int i) { //ver rectangle |
		int r, c;
		uint64_t cp = 0, eraser = 0, newRC = 0;
		r = (state >> (B[i].p * 3)) & 7;
		c = (state >> ((B[i].p - 1) * 3)) & 7;
		
		graph[r][c] = 0;
		graph[r + 1][c] = 0;
		
		for (int d = 0, counter = 0; d < 8; d++, counter++)
		{
			if (d > 3 && counter < 1)break;
			int x = dir[d][0], y = dir[d][1];
			if (r + x >= 0 && r + x < ROW && c + y < COL && c + y >= 0) {
				if (graph[r + x][c + y] == 0 && graph[r + x + 1][c + y] == 0) {
					cp = state;
					eraser = 0;
					newRC = 0;
					eraser |= 63;
					eraser <<= ((B[i].p - 1) * 3);

					eraser = ~eraser;
					cp &= eraser;

					//printf("Initial r: %i, c: %i \n", r, c);
					//printf("After r+x: %i, c+y: %i \n", r + x, c + y);
					newRC |= (r + x);
					newRC <<= 3;
					newRC |= (c + y);
					newRC <<= ((B[i].p - 1) * 3);
					cp |= newRC;
				
					if (visited.find(cp) == visited.end())
					{
						q.push_back(cp);
						p[cp] = state;
						visited.insert(cp);
					
					}
				}
			}
		}
		graph[r][c] = 1;
		graph[r + 1][c] = 1;
	}

	void moveLsq(int i) {
		int r, c;
		uint64_t cp = 0, eraser = 0, newRC = 0;
		r = (state >> (B[i].p * 3)) & 7;
		c = (state >> ((B[i].p - 1) * 3)) & 7;
		graph[r][c] = 0;
		graph[r + 1][c] = 0;
		graph[r][c + 1] = 0;
		graph[r + 1][c + 1] = 0;
		
		for (int d = 0, counter = 0; d < 8; d++, counter++)
		{
			if (d > 3 && counter < 1)break;
			int x = dir[d][0], y = dir[d][1];
			if (r + x >= 0 && r + x < ROW && c + y < COL && c + y >= 0) {
				if (graph[r + x][c + y] == 0 && graph[r + x + 1][c + y] == 0
					&& graph[r + x][c + y + 1] == 0 && graph[r + x + 1][c + y + 1] == 0) {
					cp = state;
					eraser = 0;
					newRC = 0;
					eraser |= 63;
					eraser <<= ((B[i].p - 1) * 3);

					eraser = ~eraser;
					cp &= eraser;

					/*printf("Initial r: %i, c: %i \n", r, c);
					printf("After r+x: %i, c+y: %i \n", r + x, c + y);
					printf("Eraser: %" PRId64"\n", eraser);*/
					newRC |= (r + x);
					newRC <<= 3;
					newRC |= (c + y);
					newRC <<= ((B[i].p - 1) * 3);
					cp |= newRC;
					
					if (visited.find(cp) == visited.end())
					{
						q.push_back(cp);
						p[cp] = state;
						visited.insert(cp);
						
					}
				}

			}
		}

		graph[r][c] = 1;
		graph[r + 1][c] = 1;
		graph[r][c + 1] = 1;
		graph[r + 1][c + 1] = 1;
	}

	void bfs() {
		q.clear();
		p.clear();
		index = 0;
		state = 0;
		int counter = 0;
		uint64_t temp = 0, goal = 0;
		readG();
		q.push_back(state);
		visited.insert(state);
		while (!q.empty()) {
			state = q.front();
			if (isFound(state)) {
				printf("Solution is Found!!!\n");
				goal = state;
				break;
			}
			printf("No. of Configuration: %i\n", counter++);
			simpleG();
			moveBlock();

			q.pop_front();
		}

	}

	/*void formS(uint64_t temp) {
		string init(20,0);
		s = init;
		int r, c;
		for (int i = 0, j = 19; i < NBLOCKS && j >= 1; i++, j -= 2) {
			r = (temp >> (j * 3)) & 7;
			c = (temp >> ((j - 1) * 3)) & 7;
			s[4 * r + c] = B[i].type + 48;
			if (B[i].type == ver) {
				s[4 * (r + 1) + c] = B[i].type + 48;
			}
			else if (B[i].type == hor) {
				s[4 * r + c + 1] = B[i].type + 48;
			}
			else if (B[i].type == lsq) {
				s[4 * r + c + 1] = B[i].type + 48;
				s[4 * (r + 1) + c] = B[i].type + 48;
				s[4 * (r + 1) + c + 1] = B[i].type + 48;
			}
		}
	}*/


	uint64_t size() {
		return solution.size();
	}

	solver(): graph(),state(0),index(0){
		
	}

private:
	bool graph[ROW][COL]; //indicated taken spot
	uint64_t state; //record state of hashed graph
	deque<uint64_t> q; //bfs
	map<uint64_t, uint64_t> p; //parentship
	deque<uint64_t> solution;//contains all solution step
	size_t index;//index of solution step
	vector<vector<int>> dir = { {0,1}, {0,-1}, {1,0}, {-1,0}, {0,2}, {0,-2}, {2,0}, {-2,0} };//direction
	unordered_set<uint64_t> visited;
};

int main(int argc, char* argv[])
{
	/* TODO: add option to specify starting state from cmd line? */
	/* start SDL; create window and such: */
	if (!init()) {
		printf("Failed to initialize from main().\n");
		return 1;
	}
	atexit(close);
	bool quit = false; /* set this to exit main loop. */
	SDL_Event e;
	solver s;
	s.readG();
	s.simpleG();
	s.printG();
	/* main loop: */
	while (!quit) {
		/* handle events */
		while (SDL_PollEvent(&e) != 0) {
			/* meta-q in i3, for example: */
			if (e.type == SDL_MOUSEMOTION) {
				if (mousestate == 1 && dragged) {
					int dx = e.button.x - lastm.x;
					int dy = e.button.y - lastm.y;
					lastm.x = e.button.x;
					lastm.y = e.button.y;
					dragged->R.x += dx;
					dragged->R.y += dy;
				}
			}
			else if (e.type == SDL_MOUSEBUTTONDOWN) {
				if (e.button.button == SDL_BUTTON_RIGHT) {
					block* b = findBlock(e.button.x, e.button.y);
					if (b) b->rotate();
				}
				else {
					mousestate = 1;
					lastm.x = e.button.x;
					lastm.y = e.button.y;
					dragged = findBlock(e.button.x, e.button.y);
				}
				/* XXX happens if during a drag, someone presses yet
				 * another mouse button??  Probably we should ignore it. */
			}
			else if (e.type == SDL_MOUSEBUTTONUP) {
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
			}
			else if (e.type == SDL_QUIT) {
				quit = true;
			}
			else if (e.type == SDL_KEYDOWN) {
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
					s.bfs();
					break;
				default:
					break;
				}
			}
		}
		fcount++;
		render();
	}

	printf("total frames rendered: %i\n", fcount);
	return 0;
}
