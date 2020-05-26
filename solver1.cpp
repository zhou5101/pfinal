/* compute optimal solutions for sliding block puzzle. */
#define NROWS 5
#define NCOLS 4
#include <SDL2/SDL.h>
#include <stdio.h>
#include <cstdlib>   /* for atexit() */
#include <algorithm>
using std::swap;
using namespace std;
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <queue> 
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
	int i,j; //board coordinates
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
	int uw = bframe.w/NCOLS;
	int uh = bframe.h/NROWS;
	/* NOTE: in a perfect world, the above would be equal. */
	int i = (y+uh/2)/uh; /* row */
	int j = (x+uw/2)/uw; /* col */
	if (0 <= i && i < NROWS && 0 <= j && j < NCOLS) {
		b->R.x = bframe.x + j*uw + ep;
		b->R.y = bframe.y + i*uh + ep;
		b->i = i;
		b->j = j;
	}
	else{
		b->i = NROWS;
		b->j = NCOLS;
	}
}

class Solution{
	int state_index;
	bool board[NROWS][NCOLS];
	unordered_set<long int> visited;
	unordered_map<long int,long int> parentOf;
	queue<long int> Q;
	vector<long int> states;
	bool found;
	void clearBoard(){
		for(int i=0;i<NROWS;i++){
			for(int j =0; j<NCOLS;j++){
				board[i][j] = false;
			}
		}
	}
		
	bool isFound(long int state){
		int y = state & 7;
		state >>= 3;
		int x = state & 7;
		return x == 3 and y == 1;
	}
	
	void assignToBoard(long int state){
		clearBoard();
		for(int i=0;i<NBLOCKS;i++){
			int y = state & 7;
			state >>= 3;
			int x = state & 7;
			state >>= 3;
			if(x == NROWS){ //Piece is outside of board
				continue;
			}
			if(i == 0){ //Piece is big square
				for(int j=0;j<2;j++){
					for(int k=0;k<2;k++){
						if(x+i < NROWS && y + j < NCOLS){
							board[x+i][y+j] = true;
						}
					}
				}
			}
			else if(i < 5){ //Piece is small square
				board[x][y] = true;
			}
			else{ //Piece is rectangle
				board[x][y] = true;
				if(B[NBLOCKS-i-1].type == ver){
					if(x+1 < NROWS){ 
						board[x+1][y] = true;
					}
				}
				else{
					if(y+1 < NCOLS){ 
						board[x][y+1] = true;
					}
				}
			}
		}
	}
	
	void generateChildStates(long int state){
		long int temp = state;
		for(int i=0;i<NBLOCKS;i++){
			int y = temp & 7; //
			temp >>= 3;
			int x = temp & 7;
			temp >>= 3;
			if(i == 0){ //Big square
				generateBigSquare(state,x,y);
			}
			else if(i < 5){ //Small square
				generateSmallSquare(state,x,y,i);
			}
			else{ //Rectangle
				generateRectangle(state,x,y,i);
			}
		}
	}
	
	void generateRectangle(long int state,int x, int y, int i){
		//Remove from board
		board[x][y] = false;
		if(B[NBLOCKS-i-1].type == ver){
			if(x+1 < NROWS){ 
				board[x+1][y] = false;
			}
		}
		else{
			if(y+1 < NCOLS){ 
				board[x][y+1] = false;
			}
		}
		
		tryToPlaceRectangle(state,x-1,y,i);
		tryToPlaceRectangle(state,x+1,y,i);
		tryToPlaceRectangle(state,x,y-1,i);
		tryToPlaceRectangle(state,x,y+1,i);
		
		//Add back to board
		board[x][y] = true;
		if(B[NBLOCKS-i-1].type == ver){
			if(x+1 < NROWS){ 
				board[x+1][y] = true;
			}
		}
		else{
			if(y+1 < NCOLS){ 
				board[x][y+1] = true;
			}
		}
	}
	
	void tryToPlaceRectangle(long int state,int x,int y,int i){
		if(x < 0 || x >= NROWS || y < 0 || y >= NCOLS || board[x][y]){
			return;
		}
		if(B[NBLOCKS-i-1].type == ver){
			if(x+1 >= NROWS || board[x+1][y]){ 
				return;
			}
		}
		else{
			if(y+1 >= NCOLS || board[x][y+1]){ 
				return;
			}
		}
		long int old_state = state;
		long int mask = 0;
		mask |= 7;
		mask <<= 3;
		mask |= 7;
		mask <<= i * 6;
		long int notmask = ~mask;
		state &= notmask; //Erase current state
		long int xy = 0;
		xy |= x;
		xy <<= 3;
		xy |= y;
		xy <<= i * 6;
		state |= xy;
		
		if(visited.find(state) == visited.end()){
			visited.insert(state);
			Q.push(state);
			parentOf[state] = old_state;
		}
		
	}
	
	void generateSmallSquare(long int state,int x,int y,int i){
		//Remove from board
		board[x][y] = false;
		
		tryToPlaceSmallSquare(state,x-1,y,i);
		tryToPlaceSmallSquare(state,x+1,y,i);
		tryToPlaceSmallSquare(state,x,y-1,i);
		tryToPlaceSmallSquare(state,x,y+1,i);
				
		//Add back to board
		board[x][y] = true;
	}
	
	void tryToPlaceSmallSquare(long int state,int x,int y,int i){
		if(x < 0 || x >= NROWS || y < 0 || y >= NCOLS || board[x][y]){
			return;
		}
		long int old_state = state;
		long int mask = 0;
		mask |= 7;
		mask <<= 3;
		mask |= 7;
		mask <<= i * 6;
		long int notmask = ~mask;
		state &= notmask; //Erase current state
		long int xy = 0;
		xy |= x;
		xy <<= 3;
		xy |= y;
		xy <<= i * 6;
		state |= xy;
		
		if(visited.find(state) == visited.end()){
			visited.insert(state);
			Q.push(state);
			parentOf[state] = old_state;
		}
		
	}
	
	void generateBigSquare(long int state,int x, int y){
		//Remove from board
		for(int i=0;i<2;i++){
			for(int j=0;j<2;j++){
				if(x+i < NROWS && y+j < NCOLS){
					board[x+i][y+j] = false;
				}
			}
		}
		
		tryToPlaceBigSquare(state,x-1,y);
		tryToPlaceBigSquare(state,x+1,y);
		tryToPlaceBigSquare(state,x,y-1);
		tryToPlaceBigSquare(state,x,y+1);
		//Add back to board
		for(int i=0;i<2;i++){
			for(int j=0;j<2;j++){
				if(x+i < NROWS && y+j < NCOLS){
					board[x+i][y+j] = true;
				}
			}
		}
	}
	
	void tryToPlaceBigSquare(long int state,int x, int y){
		for(int i=0;i<2;i++){
			for(int j=0;j<2;j++){
				if(x+i < NROWS && y+j < NCOLS && x+i >= 0 && y+j >= 0){
					if(board[x+i][y+j]){
						return; //Another piece is already placed here
					}
				}
				else{//Outside of board
					return;
				}
			}
		}
		long int old_state = state;
		//Erase previous pos
		state >>= 6;
		//Add x coordinate
		state <<= 3;
		state |= x;
		//Add y coordinate
		state <<= 3;
		state |= y;
		if(visited.find(state) == visited.end()){
			visited.insert(state);
			Q.push(state);
			parentOf[state] = old_state;
		}
	}
	
	void assignStateToBlocks(long int state){
		int uw = bframe.w/NCOLS;
		int uh = bframe.h/NROWS;
		for(int i=9;i>=0;i--){
			int y = state & 7;
			state >>= 3;
			int x = state & 7;
			state >>= 3;
			if(x == NROWS){ //If piece is outside of board skip it
				continue;
			}
			B[i].i = x;
			B[i].j = y;
			B[i].R.x = bframe.x + y*uw + ep;
			B[i].R.y = bframe.y + x*uh + ep;
		}
	}
	
public:
	
	Solution(){
		state_index = 0;
		states.clear();
		parentOf.clear();
		visited.clear();
		Q = queue<long int>();
		clearBoard();
		found = false;
	}
	
	void solve(){
		state_index = 0;
		Q = queue<long int>();
		states.clear();
		parentOf.clear();
		visited.clear();
		clearBoard();
		long int start = readCurrentState();
		visited.insert(start);
		Q.push(start);
		found = false;
		long int found_state = 0;
		while(!Q.empty() && !found){
			long int state = Q.front();
			found = isFound(state);
			if(found){
				found_state = state;
				break;
			}
			Q.pop();
			assignToBoard(state);
			generateChildStates(state);
		}
		if(found){
			printf("Solution found.\n");
			while(parentOf.find(found_state) != parentOf.end()){ //State is not starting state
				states.push_back(found_state);
				found_state = parentOf[found_state];
			}
			states.push_back(start);
			state_index = states.size() - 1;
			
		}
		else{
			printf("Solution not found.\n");
		}
	}
	
	void next_state(){
		if(state_index <= 0){
			return;
		}
		state_index--;
		long int state = states.at(state_index);
		assignStateToBlocks(state);
	}
	
	void previous_state(){
		if(state_index >= states.size()-1){
			return;
		}
		state_index++;
		long int state = states.at(state_index);
		assignStateToBlocks(state);
	}
	
	long int readCurrentState(){
		long int state = 0;
		for(int i=0;i<NBLOCKS;i++){
			state <<= 3;
			state |= B[i].i;
			state <<= 3;
			state |= B[i].j;
		}
		return state;
	}
	
};

int main(int argc, char *argv[])
{
	Solution s;
	/* TODO: add option to specify starting state from cmd line? */
	/* start SDL; create window and such: */
	if(!init()) {
		printf( "Failed to initialize from main().\n" );
		return 1;
	}
	atexit(close);
	bool quit = false; /* set this to exit main loop. */
	SDL_Event e;
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
						s.previous_state();
						break;
					case SDLK_RIGHT:
						/* TODO: show next step of solution */
						s.next_state();
						break;
					case SDLK_p:
						/* TODO: print the state to stdout
						 * (maybe for debugging purposes...) */
						printf("Current state is %ld\n",s.readCurrentState());
						break;
					case SDLK_s:
						/* TODO: try to find a solution */
						s.solve();
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
//void createSsq(int i, int r, int c, int x, int y) {
	//	uint64_t cp = 0, eraser = 0, newRC = 0;

	//	//printf("r+x: %i, c+y: %i\n", r + x, c + y);
	//	if (r + x >= 0 && r + x < ROW && c + y < COL && c + y >= 0) {
	//		if (graph[r + x][c + y] == 0) {
	//			cp = state;
	//			eraser = 0;
	//			newRC = 0;

	//			eraser |= 63;
	//			eraser <<= ((B[i].p - 1) * 3);
	//			printf("Eraser: %" PRId64"\n", eraser);
	//			eraser = ~eraser;
	//			cp &= eraser;

	//			printf("Initial r: %i, c: %i \n",r,c);
	//			printf("After r+x: %i, c+y: %i \n", r + x, c + y);
	//			printf("Eraser: %" PRId64"\n", eraser);
	//			newRC |= (r + x);
	//			newRC <<= 3;
	//			newRC |= (c + y);
	//			newRC <<= ((B[i].p - 1) * 3);
	//			cp |= newRC;

	//			if (find(visited.begin(), visited.end(), cp) == visited.end())
	//			{
	//				q.push_back(cp);
	//				p[cp] = state;
	//				visited.push_back(cp);
	//				printf("Parent Configuration: %" PRId64"\n", state);
	//				printf("Child Configuration: %" PRId64"\n", cp);
	//			}

	//		}
	//	}

	//}