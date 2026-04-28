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
#define COVERED 1

// reads input board
char *read_input(FILE *fp, int *W_p, int *H_p, int *L_p) {
	fscanf(fp, "%d %d %d\n", W_p, H_p, L_p);
	char *board = malloc(*W_p * *H_p);
	char *buffer = malloc(*W_p + 2);
	for (int i = 0; i < *H_p; i++) {
		fgets(buffer, *W_p + 2, fp);
		strncpy(board + *W_p*i, buffer, *W_p);
	}
	free(buffer);
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

// checks for correctness of solution
int check_correctness(char *board, int W, int H, int *cats, int n_cats, int *lasers, int n_lasers) {
	unsigned char *laser_map = calloc(W * H, sizeof(unsigned char));
	for (int l = 0; l < n_lasers; l++) {
		int pos = lasers[l];
		int dir = board[pos];
		int hit_wall = 0;
		while (!hits_edge(W, H, pos, dir) && !hit_wall) {
			pos = move(W, pos, dir);
			switch(board[pos]) {
				case EMPTY:
				case CAT:
					laser_map[pos] = COVERED;
					break;
				case WALL:
					hit_wall = 1;
					break;
				case MIRROR_1:
				case MIRROR_2:
					dir = change_dir(dir, board[pos]);
					break;
			}
		}
	}
	for (int c = 0; c < n_cats; c++) {
		if (laser_map[cats[c]] != COVERED) {
			free(laser_map);
			return 0;
		}
	}
	free(laser_map);
	return 1;
}

// find indices of cats, lasers and empty_spaces
void find_cats_lasers_empties(char *board, int W, int H, int *cats, int *n_cats, int *lasers, int *n_lasers, int *empties, int *n_empties) {
	*n_cats = 0;
	*n_lasers = 0;
	*n_empties = 0;
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
			case EMPTY:
				empties[(*n_empties)++] = i;
				break;
		}
	}
}

// place mirrors in given positions
void place_mirrors(char *board, int *mirrors, int L, int *empties) {
	for (int i = 0; i < L; i++) board[empties[mirrors[i]]] = MIRROR_1;
}

// rotate placed mirrors, returns 0 if at last rotation
int rotate_mirrors(char *board, int *mirrors, int mirror, int *empties) {
	if (mirror < 0) return 0;
	if (board[empties[mirrors[mirror]]] == MIRROR_2) {
		board[empties[mirrors[mirror]]] = MIRROR_1;
		return rotate_mirrors(board, mirrors, mirror-1, empties);
	}
	board[empties[mirrors[mirror]]] = MIRROR_2;
	return 1;
}

// remove mirrors from board
void remove_mirrors(char *board, int *mirrors, int L, int *empties) {
	for (int i = 0; i < L; i++) board[empties[mirrors[i]]] = EMPTY;
}

// next combination of mirror placements
void next_combination(int *mirrors, int L, int n_empties) {
	int mirror = L-1;
	while (mirror > 0 && mirrors[mirror] == n_empties - L + mirror) mirror--;
	mirrors[mirror]++;
	for (int i = mirror+1; i < L; i++) mirrors[i] = mirrors[i-1]+1;
}

// solves the problem
void solve(char *board, int W, int H, int L) {
	// find indices of cats, lasers and empty spaces
	int *cats = malloc(sizeof(int) * W*H);
	int *lasers = malloc(sizeof(int) * W*H);
	int *empties = malloc(sizeof(int) * W*H);
	int n_cats, n_lasers, n_empties;
	find_cats_lasers_empties(board, W, H, cats, &n_cats, lasers, &n_lasers, empties, &n_empties);
	cats = realloc(cats, sizeof(int) * n_cats);
	lasers = realloc(lasers, sizeof(int) * n_lasers);
	empties = realloc(empties, sizeof(int) * n_empties);

	// BRUTEFORCE

	// place mirrors in initial positions
	L = n_empties < L ? n_empties : L;
	if (L < 1) return;

	int *mirrors = malloc(sizeof(int) * L);
	for (int i = 0; i < L; i++) {
		mirrors[i] = i;
	}
	place_mirrors(board, mirrors, L, empties);

	while (!check_correctness(board, W, H, cats, n_cats, lasers, n_lasers)) {
		// check all rotations of mirrors
		while (rotate_mirrors(board, mirrors, L-1, empties)) {
			if (check_correctness(board, W, H, cats, n_cats, lasers, n_lasers)) {
				free(cats);
				free(lasers);
				free(empties);
				free(mirrors);
				return;
			}
		}

		// remove placed mirrors
		remove_mirrors(board, mirrors, L, empties);

		// if no solutions found - try placing less mirrors
		if (mirrors[0] == n_empties-L) {
			free(cats);
			free(lasers);
			free(empties);
			free(mirrors);
			solve(board, W, H, L-1);
			return;
		}

		// find next placement combination for mirrors
		next_combination(mirrors, L, n_empties);
		// place mirrors in new placement combination
		place_mirrors(board, mirrors, L, empties);
	}

	free(cats);
	free(lasers);
	free(empties);
	free(mirrors);
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
}
