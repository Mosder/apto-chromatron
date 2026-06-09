#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

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
// returns the number of lit cats
int update_laser_map(unsigned int *laser_map, char *board, int W, int H, int* lasers, int n_lasers) {
	memset(laser_map, 0, W*H * sizeof(unsigned int));
	int lit_cats = 0;
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
					laser_map[pos]++;
					break;
				case CAT:
					if (laser_map[pos] == 0) lit_cats++;
				default:
					laser_map[pos]++;
			}
		}
	}
	return lit_cats;
}

// bruteforce
int bruteforce(char *board, int W, int H, int L, unsigned int *laser_map, int *cats, int n_cats, int *lasers, int n_lasers, int depth, int lit_cats) {
	// if no more mirrors to place - check if the solution is correct
	if (L < 1 || depth < 1) return is_correct(laser_map, cats, n_cats);

	// place recursively mirrors on places that have lasers passing through them
	for (int pos = 0; pos < W*H; pos++) {
		if (board[pos] != EMPTY || laser_map[pos] == 0) continue;
		for (int i = 0; i < 2; i++) {
			board[pos] = mirror_types[i];
			int new_lit_cats = update_laser_map(laser_map, board, W, H, lasers, n_lasers);
			int new_depth = new_lit_cats <= lit_cats ? depth - 1 : depth;
			if(bruteforce(board, W, H, L-1, laser_map, cats, n_cats, lasers, n_lasers, new_depth, new_lit_cats))
				return 1;
		}
		board[pos] = EMPTY;
		update_laser_map(laser_map, board, W, H, lasers, n_lasers);
	}
	return 0;
}

// initialize random state
void initialize_random(char *board, int W, int H, int L, int *mirror_pos, unsigned int *laser_map, int *lasers, int n_lasers) {
	for (int i = 0; i < L ; i++) {
		int pos;
		do {
			pos = rand() % (W*H);
		} while (board[pos] != EMPTY || laser_map[pos] == 0);
		mirror_pos[i] = pos;
		board[pos] = mirror_types[rand() % 2];
		update_laser_map(laser_map, board, W, H, lasers, n_lasers);
	}
}

// annealing
int annealing(char *board, int W, int H, int L, unsigned int *laser_map, int *cats, int n_cats, int *lasers, int n_lasers) {
	int *mirror_pos = malloc(sizeof(int) * L);
	char *best_board = malloc(W * H);

	// initialize random state
	initialize_random(board, W, H, L, mirror_pos, laser_map, lasers, n_lasers);
	memcpy(best_board,board,W*H);

	int lit = update_laser_map(laser_map, board, W, H, lasers, n_lasers);
	int best_lit = lit;

	// probability to change in percentages
	int only_move_prob = 40;
	int only_rotate_prob = 20;

	// start annealing
	double temp = 1000.0;
	double temp_min = 1e-3;
	double alpha = 0.9995;
	while (temp > temp_min && best_lit < n_cats) {
		int mirror_index = rand() % L;
		int old_pos = mirror_pos[mirror_index];
		char old_type = board[old_pos];
		board[old_pos] = EMPTY;
		int new_pos = old_pos;
		char new_type = old_type;

		// decide what to do
		// 0 - move, 1 - rotate, 2 - move and rotate
		int random = rand() % 100;
		int choice = random < only_move_prob ? 0 : (random < only_move_prob + only_rotate_prob ? 1 : 2);

		// move
		if(choice == 0 || choice == 2) {
			update_laser_map(laser_map, board, W, H, lasers, n_lasers);
			do {
				new_pos = rand() % (W*H);
			} while (board[new_pos] != EMPTY || new_pos == old_pos || laser_map[new_pos] == 0);
		}

		// rotate
		if(choice == 1 || choice == 2) {
			new_type = old_type == MIRROR_1 ? MIRROR_2 : MIRROR_1;
		}

		mirror_pos[mirror_index] = new_pos;
		board[new_pos] = new_type;

		int new_lit = update_laser_map(laser_map, board, W, H, lasers, n_lasers);

		// check if we accept new state
		int accept = 0;
		if (new_lit >= lit) {
			accept = 1;
		}
		else {
			double exponent = -(lit-new_lit) / temp;
			double prob = exp(exponent);
			if (rand() % 100000 < prob * 100000) accept = 1;
		}

		// accept or not
		if (accept) {
			lit = new_lit;
			if (lit > best_lit) {
				best_lit = lit;
				memcpy(best_board, board, W*H);
			}
		}
		else {
			board[new_pos] = EMPTY;
			mirror_pos[mirror_index] = old_pos;
			board[old_pos] = old_type;
		}

		temp *= alpha;
	}

	memcpy(board, best_board, W*H);
	free(best_board);
	free(mirror_pos);
	return best_lit == n_cats;
}

// solves the problem
void solve(char *board, int W, int H, int L) {
	srand(time(NULL));

	// find indices of cats and lasers
	int *cats = malloc(sizeof(int) * W*H);
	int *lasers = malloc(sizeof(int) * W*H);
	int n_cats, n_lasers;
	find_cats_and_lasers(board, W, H, cats, &n_cats, lasers, &n_lasers);
	cats = realloc(cats, sizeof(int) * n_cats);
	lasers = realloc(lasers, sizeof(int) * n_lasers);

	// define map of lasers
	unsigned int *laser_map = malloc(W*H * sizeof(unsigned int));
	int lit_cats = update_laser_map(laser_map, board, W, H, lasers, n_lasers);

	// BRUTEFORCE
	// check for solutions using mirrors L and depth from 1 to 100
	//int depth = 1;
	//while (!bruteforce(board, W, H, L, laser_map, cats, n_cats, lasers, n_lasers, depth, lit_cats) && ++depth <= 100);

	// ANNEALING
	char *board_copy = malloc(W*H);
	memcpy(board_copy, board, W*H);
	while(!annealing(board, W, H, L, laser_map, cats, n_cats, lasers, n_lasers)) {
		memcpy(board, board_copy, W*H);
	}

	free(board_copy);
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
