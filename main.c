#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define EMPTY '.'
#define UP_LASER 'A'
#define DOWN_LASER 'V'
#define LEFT_LASER '<'
#define RIGHT_LASER '>'
#define CAT 'O'
#define MIRROR_1 '\\'
#define MIRROR_2 '/'
#define WALL '#'

#define MIRROR_TYPE_COUNT 2
char mirror_types[MIRROR_TYPE_COUNT] = {MIRROR_1, MIRROR_2};

// reads input board
char *read_input(FILE *fp, int *W_p, int *H_p, int *L_p) {
	fscanf(fp, "%d %d %d", W_p, H_p, L_p);
	char *board = malloc(*W_p * *H_p);
	for (int row = 0; row < *H_p; row++) {
		for (int col = 0; col < *W_p; col++) {
			fscanf(fp, " %c", board + row * *W_p + col);
		}
	}
	return board;
}

// prints board
void print_board(char *board, int W, int H, int L) {
	printf("%d %d %d\n", W, H, L);
	for (int r = 0; r < H; r++) {
		for (int c = 0; c < W; c++) {
			printf("%c", board[r*W + c]);
		}
		printf("\n");
	}
}

// check if the move will hit the edge of the board
int hits_edge(int W, int H, int pos, char dir) {
	switch(dir) {
		case UP_LASER:
			return pos < W;
		case DOWN_LASER:
			return pos >= W*(H-1);
		case LEFT_LASER:
			return pos%W == 0;
		case RIGHT_LASER:
			return pos%W == W-1;
	}
}

// move position
int move(int W, int pos, char dir) {
	switch(dir) {
		case UP_LASER:
			return pos-W;
		case DOWN_LASER:
			return pos+W;
		case LEFT_LASER:
			return pos-1;
		case RIGHT_LASER:
			return pos+1;
	}
}

// change direction of laser
char change_dir(char dir, char mirror) {
	switch(dir) {
		case UP_LASER:
			return mirror == MIRROR_1 ? LEFT_LASER : RIGHT_LASER;
		case DOWN_LASER:
			return mirror == MIRROR_1 ? RIGHT_LASER : LEFT_LASER;
		case LEFT_LASER:
			return mirror == MIRROR_1 ? UP_LASER : DOWN_LASER;
		case RIGHT_LASER:
			return mirror == MIRROR_1 ? DOWN_LASER : UP_LASER;
	}
}

// checks if given solution is correct
int is_correct(unsigned int *laser_map, int *cats, int n_cats) {
	for (int c = 0; c < n_cats; c++) {
		if (laser_map[cats[c]] < 1) return 0;
	}
	return 1;
}

// find indices of cats and lasers
void find_cats_and_lasers(char *board, int W, int H, int *cats, int *n_cats, int *lasers, int *n_lasers) {
	*n_cats = 0;
	*n_lasers = 0;
	for (int i = 0; i < W*H; i++) {
		switch(board[i]) {
			case CAT:
				cats[(*n_cats)++] = i;
				break;
			case UP_LASER:
			case DOWN_LASER:
			case LEFT_LASER:
			case RIGHT_LASER:
				lasers[(*n_lasers)++] = i;
				break;
		}
	}
}

// update laser map - value is the amount of times a laser passes through a field
void update_laser_map(unsigned int *laser_map, char *board, int W, int H, int *lasers, int n_lasers) {
	memset(laser_map, 0, W*H * sizeof(unsigned int));
	for (int i = 0; i < n_lasers; i++) {
		int pos = lasers[i];
		char dir = board[pos];
		int hit_wall = 0;
		while (!hits_edge(W, H, pos, dir) && !hit_wall) {
			pos = move(W, pos, dir);
			// prevent a loop
			if (pos == lasers[i] && dir == board[lasers[i]]) break;

			switch (board[pos]) {
				case WALL:
					hit_wall = 1;
					break;
				case MIRROR_1:
				case MIRROR_2:
					dir = change_dir(dir, board[pos]);
				default:
					laser_map[pos]++;
			}
		}
	}
}

// bruteforce
int bruteforce(char *board, int W, int H, int L, unsigned int *laser_map, int *cats, int n_cats, int *lasers, int n_lasers) {
	// if no more mirrors to place - check if the solution is correct
	if (L < 1) return is_correct(laser_map, cats, n_cats);

	// place recursively mirrors on places that have lasers passing through them
	for (int pos = 0; pos < W*H; pos++) {
		if (board[pos] != EMPTY || laser_map[pos] == 0) continue;
		for (int i = 0; i < 2; i++) {
			board[pos] = mirror_types[i];
			update_laser_map(laser_map, board, W, H, lasers, n_lasers);
			if(bruteforce(board, W, H, L-1, laser_map, cats, n_cats, lasers, n_lasers))
				return 1;
		}
		board[pos] = EMPTY;
		update_laser_map(laser_map, board, W, H, lasers, n_lasers);
	}
	return 0;
}

// solves the problem
void solve(char *board, int W, int H, int L) {
	// find indices of cats, lasers
	int *cats = malloc(sizeof(int) * W*H);
	int *lasers = malloc(sizeof(int) * W*H);
	int n_cats, n_lasers;
	find_cats_and_lasers(board, W, H, cats, &n_cats, lasers, &n_lasers);
	cats = realloc(cats, sizeof(int) * n_cats);
	lasers = realloc(lasers, sizeof(int) * n_lasers);

	// define map of lasers
	unsigned int *laser_map = malloc(W*H * sizeof(unsigned int));
	update_laser_map(laser_map, board, W, H, lasers, n_lasers);

	// check for solutions using mirrors from L to 1
	while (!bruteforce(board, W, H, L, laser_map, cats, n_cats, lasers, n_lasers) && --L >= 0);

	free(cats);
	free(lasers);
	free(laser_map);
}

int main(int argc, char *argv[]) {
	int W, H, L;
	
	// read board from input/file
	FILE *fp;
	if (argc > 1) fp = fopen(argv[1], "r");
	else fp = stdin;
	char *board = read_input(fp, &W, &H, &L);
	if (argc > 1) fclose(fp);

	solve(board, W, H, L);
	print_board(board, W, H, L);
	free(board);

	return 0;
}
