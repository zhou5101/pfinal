/* compute optimal solutions for sliding block puzzle. */
#define ROWS 5
#define COLS 4
#include <SDL2/SDL.h>
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

//#define FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP
#define FULLSCREEN_FLAG 0

/* NOTE: ssq == "small square", lsq == "large square" */
enum bType {hor,ver,ssq,lsq};
struct block {
	SDL_Rect R; /* screen coords + dimensions */
	bType type; /* shape + orientation */
	/* TODO: you might want to add other useful information to
	 * this struct, like where it is attached on the board.
	 * (Alternatively, you could just compute this from R.x and R.y,
	 * but it might be convenient to store it directly.) */
	int r,c; //block's coordinates
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
	int h = H*3/4;
	int w = 4*h/5;
	int u = h/5-2*ep;
	int mw = (W-w)/2;
	int mh = (H-h)/2;

	/* setup bounding rectangle of the board: */
	bframe.x = (W-w)/2;
	bframe.y = (H-h)/2;
	bframe.w = w;
	bframe.h = h;

	/* NOTE: there is a tacit assumption that should probably be
	 * made explicit: blocks 0--4 are the rectangles, 5-8 are small
	 * squares, and 9 is the big square.  This is assumed by the
	 * drawBlocks function below. */

	for (size_t i = 0; i < 5; i++) {
		B[i].R.x = (mw-2*u)/2;
		B[i].R.y = mh + (i+1)*(u/5) + i*u;
		B[i].R.w = 2*(u+ep);
		B[i].R.h = u;
		B[i].type = hor;
	}
	B[4].R.x = mw+ep;
	B[4].R.y = mh+ep;
	B[4].R.w = 2*(u+ep);
	B[4].R.h = u;
	B[4].type = hor;
	/* small squares */
	for (size_t i = 0; i < 4; i++) {
		B[i+5].R.x = (W+w)/2 + (mw-2*u)/2 + (i%2)*(u+u/5);
		B[i+5].R.y = mh + ((i/2)+1)*(u/5) + (i/2)*u;
		B[i+5].R.w = u;
		B[i+5].R.h = u;
		B[i+5].type = ssq;
	}
	B[9].R.x = B[5].R.x + u/10;
	B[9].R.y = B[7].R.y + u + 2*u/5;
	B[9].R.w = 2*(u+ep);
	B[9].R.h = 2*(u+ep);
	B[9].type = lsq;
 //initial configuration
	for (int i = 0; i< 10; i++){
		if (i<4)
			B[i].rotate();
		}
		int uw = bframe.w/COLS;
		int uh = bframe.h/ROWS;
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

	for(int i = 0; i < 10; i++){
			B[i].R.x =  bframe.x + B[i].c*uw + ep;
			B[i].R.y = bframe.y + B[i].r*uh + ep;
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
	int uw = bframe.w/COLS;
	int uh = bframe.h/ROWS;
	/* NOTE: in a perfect world, the above would be equal. */
	int i = (y+uh/2)/uh; /* row */
	int j = (x+uw/2)/uw; /* col */
	if (0 <= i && i < ROWS && 0 <= j && j < COLS) {
		b->R.x = bframe.x + j*uw + ep;
		b->R.y = bframe.y + i*uh + ep;
		b->r = i;
		b->c = j;
	}
	else{
		b->r = ROWS;
		b->c = COLS;
	}
}

class solver {
public:
	void clear() {
		for (int i = 0; i < ROWS; i++) {
			for (int j = 0; j < COLS; j++)
				board[i][j] = -1;
		}
	}
	void read() {
		for (int i = 0; i < 10; i++) {
			int r = B[i].r, c = B[i].c;
			if (board[r][c] == -1) {
				board[r][c] = i;
				if (B[i].type == hor) {
					board[r][c + 1] = i;
				}
				else if (B[i].type == ver) {
					board[r + 1][c] = i;
				}
				else {
					if (B[i].type == lsq) {
						board[r + 1][c] = i;
						board[r][c + 1] = i;
						board[r + 1][c + 1] = i;
					}
				}
			}
		}
	}

	void print() {
		for (int i = 0; i < ROWS; i++) {
			for (int j = 0; j < COLS; j++)
				cout << board[i][j] << ' ';
			cout << '\n';
		}
	}

	void neighbors() {
		n.clear();
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 4; j++) {
				if (board[i][j] == -1) {
					if (i - 1 >= 0)
						if (board[i - 1][j] > -1)
							n.insert(board[i - 1][j]);
					if (i + 1 < ROWS)
						if (board[i + 1][j] > -1)
							n.insert(board[i + 1][j]);
					if (j - 1 >= 0)
						if (board[i][j - 1] > -1)
							n.insert(board[i][j - 1]);
					if (j + 1 < COLS)
						if (board[i][j + 1] > -1)
							n.insert(board[i][j + 1]);
				}
			}
		}
	}
	void printN() {
		cout << "neighbors: ";
		for (auto i = n.begin(); i != n.end(); i++) {
			cout << *i << " ";
		}
		cout << "\n";
	}

	void coordinate() {
		c.clear();
		for (int i = 0; i < ROWS; i++)
			for (int j = 0; j < COLS; j++)
				if(board[i][j] > -1)
					c[board[i][j]].push_back( make_pair(i, j));
	}
	void printC() {
		for (auto i = c.begin(); i != c.end(); i++) {
			cout << i->first << ": ";
			for (auto j = i->second.begin(); j != i->second.end(); j++)
				cout <<'(' <<j->first << ',' << j->second<<')';
			cout << "\n";
		}
		cout << "\n";
	}

	bool found() {
		return cp[4][1] == 9 && cp[4][2] == 9;
	}

	bool validMove(int row, int col, vector<pair<int,int>> xy) {
		for (auto i = xy.begin(); i != xy.end(); i++) {
			if ((i->first + row) > -1 && (i->first + row) < ROWS && (i->second + col) > -1 && (i->second + col) < COLS)
				if (cp[i->first + row][i->second + col] == -1 || cp[i->first + row][i->second + col] == board[i->first][i->second])
					continue;
				else
					return false;
			else
				return false;
		}
		return true;
	}

	void bfs() {
		size_t counter = 0;
		clear();
		read();
		q.clear();
		p.clear();
		r.clear();
		solution.clear();
		vector<vector<int>> initB = board;
		goal.clear();
		q.push_back(board);
		while (!q.empty()) {
			board = q.front();
			cp = board;
			coordinate();
			neighbors();
			q.pop_front();
			if (found()) {
				cout << "Solution Found!!!\n";
				goal = board;
				break;
			}
			cout << "NO. of Configuration: " << counter++ << '\n';
			for (auto i = n.begin(); i != n.end(); i++) {
				int z = *i;
				for (int j = 0; j < 8; j++) {
					int row = dir[j][0], col = dir[j][1];
					if (validMove(row, col, c[*i])) {
						for (auto i = c[z].begin(); i != c[z].end(); i++) {
							cp[i->first][i->second] = -1;
						}
						for (auto i = c[z].begin(); i != c[z].end(); i++) {
							cp[i->first + row][i->second + col] = z;
						}
						simplfiy(cp);
						if (find(q.begin(), q.end(), cp) == q.end() && find(r.begin(), r.end(), sboard) == r.end()) {
							r.push_back(sboard);
							q.push_back(cp);
							p.insert({ cp,board });
						}
					}
					cp = board;
				}
			}


		}
		if (goal == board) {
			solution.push_back(goal);
			while (p.find(goal)->second != initB) {
				goal = p.find(goal)->second;
				solution.push_front(goal);
			}
			//solution.push_front(goal);
			solution.push_front(initB);
			printf("Solved in %zu steps.\n", solution.size());
		}
		else
			cout << "No Solution!!!\n";

	}

	void simplfiy(vector<vector<int>> g) {

		for (int i = 0; i < ROWS; i++)
			for (int j = 0; j < COLS; j++)
				if(g[i][j]>-1)
					g[i][j] = B[g[i][j]].type;
		sboard = g;
	}

	void assignToBoard(vector<vector<int>> g) {
		board = g;
		coordinate();
		int uw = bframe.w / COLS;
		int uh = bframe.h / ROWS;
		for (int i = 0; i < NBLOCKS; i++)
		{
			B[i].r = c[i].at(0).first;
			B[i].c = c[i].at(0).second;
			B[i].R.x = bframe.x + B[i].c * uw + ep;
			B[i].R.y = bframe.y + B[i].r * uh + ep;
		}
	}

	void prev() {
		if (index <= 0){
			assignToBoard(solution[0]);
			return;
		}

		assignToBoard(solution[index--]);
	}

	void next() {
		if (index >= solution.size()-1){
			assignToBoard(solution[solution.size()-1]);
			return;
			}

		assignToBoard(solution[index++]);
	}

	void oneClickSolve() {
		int i = 0;
		while (i != solution.size()) {
			//printf("# of Configuration: %i\n", i);
			assignToBoard(solution[i]);
			//std::this_thread::sleep_for(std::chrono::seconds(5));
			i++;
		}


	}


	size_t steps() {
		return solution.size();
	}

	solver() {
		vector<vector<int>> temp(5, vector<int>(4, -1));
		board = temp;
		cp = temp;
		index = 0;
	}

private:
	vector<vector<int>> board;
	vector<vector<int>> cp;//copy of board;
	vector<vector<int>> sboard;//simplified board;
	vector<vector<int>> goal;
	deque<vector<vector<int>>> q;//for bfs
	unordered_set<int> n;//neighbours
	deque<vector<vector<int>>> solution;
	map<vector<vector<int>>, vector<vector<int>>> p;//record parent of generated state
	map<int,vector<pair<int, int>>> c;
	vector<vector<int>> dir = { {0,1}, {0,-1}, {1,0}, {-1,0}, {0,2}, {0,-2}, {2,0}, {-2,0} };
	deque<vector<vector<int>>> r; //eliminate repeated conggiguration
	int index;
};

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
	solver s;

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
						s.prev();
						break;
					case SDLK_RIGHT:
						/* TODO: show next step of solution */
						s.next();
						break;
					case SDLK_p:
						/* TODO: print the state to stdout
						 * (maybe for debugging purposes...) */
						s.print();
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

	printf("total frames rendered: %i\n",fcount);
	return 0;
}
