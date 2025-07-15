#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "board.h"
#include "game_logic.h"
#include "game_loop.h"
#include "minimax.h"

game_mode_t game_mode = AI_PLAYER;

int main(void) {
	// Starting player = PLAYER_1
	AI_player(PLAYER_1);
	return 0;
}

void init_board(board_t *board, game_history *history) {
	unsigned short i = 0;
	
	//Initialise checkers board to the start configuration.
	board -> squares = (square_profile *)malloc(sizeof(square_profile) * BOARD_SIZE);
	while(i < BOARD_SIZE) {
		if(i < 12)
			board -> squares[i].player = PLAYER_1;
		else if(i > 19)
			board -> squares[i].player = PLAYER_2;
		else
			board -> squares[i].player = EMPTY;
		board -> squares[i].king = FALSE;
		i++;
	}
	
	//Initialize game history stacks.
	history -> undo_top = NULL;
	history -> redo_top = NULL;
	return;
}

void init_bitboard(bitboard_t *b) {
	b -> player1_pawns = P1_PAWNS;
	b -> player2_pawns = P2_PAWNS;
	b -> player1_kings = 0;
	b -> player2_kings = 0;
	return;
}

unsigned short interpreter(unsigned short col, unsigned short row) {
	if(row % 2 == col % 2)
		return row % 2 ? (((row - 1) * 4) + (col / 2)) : (((row - 1) * 4) + (col / 2) - 1);
	else
		return INVALID;
}

void display_board(board_t *board) {
	short row, col;
	square_profile square;
    printf("  +---+---+---+---+---+---+---+---+\n");
    for (row = 8; row >= 1; row--) {
        // row labels from 1 to 8
        printf("%d |", row);
        for (col = 1; col <= 8; col++) {
            square = board -> squares[interpreter(col, row)]; //storing the location of piece at square
            if (square.player == PLAYER_1) {
                if (square.king)
                    printf(" KX|");  // king for PLAYER_1
                else
                    printf(" X |");  // normal pawn for PLAYER_1
            }
            else if (square.player == PLAYER_2) {
                if (square.king)
                    printf(" KO|");  // king for PLAYER_2
                else
                    printf(" O |");  // normal pawn for PLAYER_2
            } 
            else
                printf(" . |");  // empty square
        }
        printf("\n  +---+---+---+---+---+---+---+---+\n");
    }
    printf("    A   B   C   D   E   F   G   H  \n");
    return;
}

bitboard_t arr_to_bitboard(board_t *board) {
	short i = 0;
	bitboard_t bitboard = {0};
	while(i < BOARD_SIZE) {
		if(board -> squares[i].player == PLAYER_1) {
			if(!board -> squares[i].king)
				bitboard.player1_pawns = bitboard.player1_pawns | 1 << i;
			else
				bitboard.player1_kings = bitboard.player1_kings | 1 << i;
		}
		else if(board -> squares[i].player == PLAYER_2) {
			if(!board -> squares[i].king)
				bitboard.player2_pawns = bitboard.player2_pawns | 1 << i;
			else
				bitboard.player2_kings = bitboard.player2_kings | 1 << i;
		}
		i++;
	}
	return bitboard;
}

void bitboard_to_arr(bitboard_t *bitboard, board_t *board) {
    short i = 0;
    while (i < BOARD_SIZE) {
        if (bitboard -> player1_pawns & (1 << i)) {
            board -> squares[i].player = PLAYER_1;
            board -> squares[i].king = FALSE;
        } 
        else if (bitboard -> player2_pawns & (1 << i)) {
            board -> squares[i].player = PLAYER_2;
            board -> squares[i].king = FALSE;
        }
        else if (bitboard -> player1_kings & (1 << i)) {
            board -> squares[i].player = PLAYER_1;
            board -> squares[i].king = TRUE;
        } 
        else if (bitboard -> player2_kings & (1 << i)) {
            board -> squares[i].player = PLAYER_2;
            board -> squares[i].king = TRUE;
        }
        else {
            board -> squares[i].player = EMPTY;
            board -> squares[i].king = FALSE;
        }
        i++;
    }
    return;
}

void init_board_state(board_state *bs) {
	bs -> next = NULL;
	return;
}

void free_redo(game_history *history) {
	if(is_redo_empty(history))
		return;
	board_state *p = history -> redo_top;
	while(p != NULL) {
		history -> redo_top = p -> next;
		free(p);
		p = history -> redo_top;
	}
	history -> redo_top = NULL;
	return;
}

void free_undo(game_history *history) {
	board_state *p = history -> undo_top;
	while(p != NULL) {
		history -> undo_top = p -> next;
		free(p);
		p = history -> undo_top;
	}
	history -> undo_top = NULL;
	return;
}

void save_progress(bitboard_t *bitboard, game_history *history, player_t current_player) {
    // Allocate memory for the new board state.
    board_state *current_board = (board_state *)malloc(sizeof(board_state));

    // Initialize and configure board state.
    init_board_state(current_board);
    current_board -> bitboard = *bitboard;
    current_board -> player_turn = current_player;

    // Purge redo stack.
    if (!is_redo_empty(history))
        free_redo(history);

    // Push latest board state onto undo stack.
    current_board -> next = history -> undo_top;
    history -> undo_top = current_board;

    return;
}


bool is_undo_empty(game_history *history) {
	if(history -> undo_top -> next == NULL)
		return 1;
	return 0;
}

bool is_redo_empty(game_history *history) {
	if(history -> redo_top == NULL)
		return 1;
	return 0;
}

bool undo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player) {
	if (is_undo_empty(history)) 
		return FALSE;

	board_state *p = history -> undo_top;

	history -> undo_top = p -> next;
	p -> next = history -> redo_top;
	history -> redo_top = p;

	*current_player = history -> redo_top -> player_turn;
	
	p = history -> undo_top;
	bitboard_to_arr(&(p -> bitboard), board);
	are_forced_jumps(&(p -> bitboard), fj, *current_player);
	
	return TRUE;
}

bool redo_action(board_t *board, game_history *history, fj_array *fj, player_t *current_player) {
	if(is_redo_empty(history))
		return FALSE;
	else {
		board_state *p = history -> redo_top;
		
		history -> redo_top = p -> next;
		p -> next = history -> undo_top;
		history -> undo_top = p;
		
		bitboard_to_arr(&(p -> bitboard), board);
		
		if(is_redo_empty(history)) {
			are_forced_jumps(&(p -> bitboard), fj, p -> player_turn);
			if(is_fj_empty(fj) && game_mode == TWO_PLAYER) {
				switch_player(current_player);
				are_forced_jumps(&(p -> bitboard), fj, p -> player_turn);
			}
			else
				*current_player = p -> player_turn;
		}	
		else {
			*current_player = history -> redo_top -> player_turn;
			are_forced_jumps(&(p -> bitboard), fj, *current_player);
		}
		return TRUE;
	}
}

void funeral(board_t *board, game_history *history, fj_array *fj, unsigned short game_state) {
	clear_screen();
	display_board(board);
	printf("\n============ GAME OVER ============\n\n");
	free(board -> squares);
	//free_undo(history);
	//free_redo(history);
	free(fj -> jumps);
	if(game_state == QUIT)
		return;
	else if(game_state == 1)
		printf("Player 1 won the game.\n");
	else if(game_state == 2)
		printf("Player 2 won the game.\n");
	else if(game_state == 3)
		printf("Draw.\n");
	return;
}


unsigned short get_row(unsigned short index) {
	return index / 4;
}

unsigned short get_col(unsigned short index) {
	return (index / 4) % 2 ? (index % 4) * 2 + 1 : (index % 4) * 2;
}

//in this function, no pieces are moved. only checks validity of the movements
bool is_valid_move(board_t *board, player_t player, unsigned short from_index, unsigned short to_index, fj_array *fj) {

	// Check if indices are valid or not.
	if(from_index > LAST_INDEX || to_index > LAST_INDEX) {
		return FALSE;
	}

	// Checking if player has chosen their own pieces.
	if(board -> squares[from_index].player != player) {
		return FALSE;
	}

	// checking if the destination square is empty
	if (board -> squares[to_index].player != EMPTY) {
        return FALSE;
    }

    // Check if any forced jumps are avaiable.
    if(!is_fj_empty(fj)) {
    	for(int i = 0; i < fj -> size; i++) {
    		if(from_index == fj -> jumps[i].from_index && to_index == fj -> jumps[i].to_index) {
    			fj -> index = i;
    			return TRUE;
    		}
    	}
    	return FALSE;
    }

     // calculating 2D coordinates of concerned squares.
    unsigned short from_x = get_col(from_index);
    unsigned short from_y = get_row(from_index);
    unsigned short to_x = get_col(to_index);
    unsigned short to_y = get_row(to_index);

    short dx = to_x - from_x;
    short dy = to_y - from_y;

    square_profile from_square = board -> squares[from_index];

    // for a regular piece(not a king)
    if (!from_square.king) {
        // PLAYER_1 can only move downwards and PLAYER_2 can only move upwards (granted there is no king)
        if ((from_square.player == PLAYER_1 && dy != 1) || (from_square.player == PLAYER_2 && dy != -1)) {
            return FALSE;  // invalid movement
        }
    }

    // simple singular square movement
    if (abs(dx) == 1 && abs(dy) == 1) {
        return TRUE;
    }
    
    return FALSE;
}

//used exclusively for moving pieces. it doesn't check any specific cases.
void move_piece(board_t *board, unsigned short from_index, unsigned short to_index, fj_array *fj) {

    // moving piece
    board -> squares[to_index] = board -> squares[from_index];

    //emptying out piece's old position.
    board -> squares[from_index].player = EMPTY;
    board -> squares[from_index].king = FALSE;
    
    if(fj -> index != -1)
     {
    	//emptying out captured piece's position.
    	unsigned short capture_index = fj -> jumps[fj -> index].capture_index;
        board -> squares[capture_index].player = EMPTY;
        board -> squares[capture_index].king = FALSE;
    }
    return;
}

void kinging(board_t *board, unsigned short index) {
    square_profile *square = &(board -> squares[index]);

    //if PLAYER_1 reached the last row, it gets promoted to king
    if (square -> player == PLAYER_1 && index > 27) {
        square -> king = TRUE;  
    }
    //if PLAYER_2 reached the first row, it gets promoted to king
    else if (square -> player == PLAYER_2 && index < 4) {
        square -> king = TRUE;  
    }
    return;
}

bool is_fj_empty(fj_array *fj) {
	if(fj -> size == 0)
		return TRUE;
	return FALSE;
}

void init_fj_array(fj_array *fj) {
	fj -> jumps = (forced_jump *)malloc(sizeof(forced_jump) * MAX_FORCED_JUMPS);
	fj -> size = 0;
	fj -> index = -1;
	return;
}

void fj_append(fj_array *fj, unsigned short from_index, unsigned short capture_index, unsigned short to_index) {
	if(fj -> size < MAX_FORCED_JUMPS) {
		fj -> jumps[fj -> size].from_index = from_index;
		fj -> jumps[fj -> size].capture_index = capture_index;
		fj -> jumps[fj -> size].to_index = to_index;
		fj -> size++;
	}
	else
		printf("Error : Max forced jump limit exceeded | fn : fj_append");
	return;
}

void find_forward_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj) {
	short i;
	for(i = 24; i >= 0; i--) {
		if(player_pieces & (1 << i)) {
			if((i / 4) % 2) {
				if(i % 8 != 7 && (opp_pieces & (1 << (i + 5))) && (empty_squares & (1 << (i + 9))))
					fj_append(fj, i, i + 5, i + 9);
				if(i % 8 != 4 && (opp_pieces & (1 << (i + 4))) && (empty_squares & (1 << (i + 7))))
					fj_append(fj, i, i + 4, i + 7);
			}
			else {
				if(i < 20 && (i % 8 != 0 && (opp_pieces & (1 << (i + 3))) && (empty_squares & (1 << (i + 7)))))
					fj_append(fj, i, i + 3, i + 7);
				if(i < 20 && (i % 8 != 3 && (opp_pieces & (1 << (i + 4))) && (empty_squares & (1 << (i + 9)))))
					fj_append(fj, i, i + 4, i + 9);
			}
		}
	}
	return;
}

void find_reverse_fj(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares, fj_array *fj) {
	short i;
	for(i = 7; i < BOARD_SIZE; i++) {
		if(player_pieces & (1 << i)) {
			if((i / 4) % 2 == 0) {
				if(i % 8 != 0 && (opp_pieces & (1 << (i - 5))) && (empty_squares & (1 << (i - 9))))
					fj_append(fj, i, i - 5, i - 9);
				if(i % 8 != 3 && (opp_pieces & (1 << (i - 4))) && (empty_squares & (1 << (i - 7))))
					fj_append(fj, i, i - 4, i - 7);
			}
			else {
				if(i > 11 && (i % 8 != 7 && (opp_pieces & (1 << (i - 3))) && (empty_squares & (1 << (i - 7)))))
					fj_append(fj, i, i - 3, i - 7);
				if(i > 11 && (i % 8 != 4 && (opp_pieces & (1 << (i - 4))) && (empty_squares & (1 << (i - 9)))))
					fj_append(fj, i, i - 4, i - 9);
			}
		}
	}
	return;
}

void are_forced_jumps(const bitboard_t *b, fj_array *fj, player_t player) {
	uint32_t opp_pieces, player_pieces, empty_squares;
	
	empty_squares = ~(b -> player1_pawns | b -> player2_pawns | b -> player1_kings | b -> player2_kings);
	
	// With every function call reset size and index of fj_array.
	fj -> size = 0;
	fj -> index = -1;
	
	if(player == PLAYER_1) {
		// Create bitboard for opponent's pieces (here Player 2).
		opp_pieces = b -> player2_pawns | b -> player2_kings;
		player_pieces = b -> player1_pawns | b -> player1_kings;
		
		// Find if any forced jumps are available for Player 1.
		find_forward_fj(player_pieces, opp_pieces, empty_squares, fj);
		
		// If Player 1 has kings, check forced jumps for them as well.
		if(b -> player1_kings)
			find_reverse_fj(b -> player1_kings, opp_pieces, empty_squares, fj);
	}
	else if(player == PLAYER_2) {
		// Create bitboard for opponent's pieces (here Player 1).
		opp_pieces = b -> player1_pawns | b -> player1_kings;
		player_pieces = b -> player2_pawns | b -> player2_kings;
		
		// Find if any forced jumps are available for Player 2.
		find_reverse_fj(player_pieces, opp_pieces, empty_squares, fj);
		
		// If Player 2 has kings, check forced jumps for them as well.
		if(b -> player1_kings)
			find_forward_fj(b -> player2_kings, opp_pieces, empty_squares, fj);
	}
	return;
}


void print_player(player_t player) {
    if(player == PLAYER_1)
        printf("\nIts Player X's turn\n\n\n");
    else
        printf("\nIts Player O's turn\n\n\n");
    return;   
}//checked

unsigned short get_index(void) {
	char *input = (char *)calloc(4, sizeof(char));
	scanf("%s", input);
	
	if(strcmp(input, "undo") == 0)
		return UNDO;
	else if(strcmp(input, "redo") == 0)
		return REDO;
	else if(strcmp(input, "quit") == 0)
		return QUIT;
		
	char col = tolower(input[0]);
	char row = input[1];
	short colin = (int)col - 96;
	short rolin = (int)row - '0';
	free(input);
	return interpreter(colin, rolin);
}

void get_move(board_t *board, unsigned short *from_index, unsigned short *to_index, fj_array *fj, player_t player) {
	bool move_status;
    do {
        do {
            printf("From: ");
            *from_index = get_index();
            if (*from_index == INVALID)
                printf("Invalid move. Please enter again.\n");
            if(*from_index > INVALID)
            	return;
        } while (*from_index == INVALID); // Repeat until a valid "from" index is entered
        
        do {
            printf("To: ");
            *to_index = get_index();
            if (*to_index == INVALID)
                printf("Invalid move. Please enter again.\n");
            if(*to_index > INVALID)
            	return;
        } while (*to_index == INVALID);
        
        move_status = is_valid_move(board, player, *from_index, *to_index, fj);
        if(!move_status)
        	printf("Illegal Move, please enter again.\n");
    } while (!move_status);
    return;
}


// For pieces able move to from 1st row towards the 8th row.
bool forward_moving_pieces(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares) {
	short i;
	for(i = 27; i >= 0; i--) {
		if(player_pieces & (1 << i)) {
			// Check if 4's diagonal is empty.
			if(empty_squares & (1 << (i + 4)))
				return 1;
			else {
			// Check if pawn is on even rows.
				if((i / 4) % 2 == 0) {
				// Check if non 4's diagonal is empty.
					if(i % 8 != 0 && (empty_squares & 1 << (i + 3)))
						return 1;
					// Check if valid capture possible on non 4's diagonal.
					else if(i < 20 && (i % 8 != 0 && (opp_pieces & (1 << (i + 3))) && (empty_squares & (1 << (i + 7)))))
						return 1;
					// Check if valid capture possible on 4's diagonal.
					if(i < 20 && (i % 8 != 3 && (opp_pieces & (1 << (i + 4))) && (empty_squares & (1 << (i + 9)))))
						return 1;
				}
				// Check if pawn on odd rows.
				else if((i / 4) % 2 == 1) {
					// Check if non 4's diagonal is empty.
					if(i % 8 != 7 && (empty_squares & 1 << (i + 5)))
						return 1;
					// Check if valid capture possible on non 4's diagonal.
					else if(i < 24 && (i % 8 != 7 && (opp_pieces & (1 << (i + 5))) && (empty_squares & (1 << (i + 9)))))
						return 1;
					// Check if valid capture possible on 4's diagonal.
					if(i < 24 && (i % 8 != 4 && (opp_pieces & (1 << (i + 4))) && (empty_squares & (1 << (i + 7)))))
						return 1;
				}
			}
		}
	}
	return 0;
}

// For pieces moving from 8th row towards the 1st row.
bool reverse_moving_pieces(uint32_t player_pieces, uint32_t opp_pieces, uint32_t empty_squares) {
	short i;
	for(i = 4; i < BOARD_SIZE; i++) {
		if(player_pieces & (1 << i)) {
			if(empty_squares & (1 << (i - 4)))
				return 1;
			else {
				// Check if pawn is on even rows.
				if((i / 4) % 2 == 0) {
					// Check if non 4's diagonal is empty.
					if((i % 8 != 0) && (empty_squares & 1 << (i - 5)))
						return 1;
					// Check if valid capture possible on non 4's diagonal.
					else if(i > 7 && (i % 8 != 0 && (opp_pieces & (1 << (i - 5))) && (empty_squares & (1 << (i - 9)))))
						return 1;
					// Check if valid capture possible on 4's diagonal.
					if(i > 7 && (i % 8 != 3 && (opp_pieces & (1 << (i - 4))) && (empty_squares & (1 << (i - 7)))))
						return 1;
				}
				else if((i / 4) % 2 == 1) {
					// Check if non 4's diagonal is empty.
					if((i % 8 != 7) && (empty_squares & 1 << (i - 3)))
						return 1;
					// Check if valid capture possible on non 4's diagonal.
					else if(i > 11 && (i % 8 != 7 && (opp_pieces & (1 << (i - 3))) && (empty_squares & (1 << (i - 7)))))
						return 1;
					// Check if valid capture possible on 4's diagonal.
					if(i > 11 && (i % 8 != 4 && (opp_pieces & (1 << (i - 4))) && (empty_squares & (1 << (i - 9)))))
						return 1;
				}
			}
		}
	}
	return 0;
}

// Function to check if game is over.
unsigned short is_game_over(const bitboard_t *bitboard) {
	if(bitboard -> player1_pawns == 0 && bitboard -> player1_kings == 0)
		return 2;
	else if (bitboard -> player2_pawns == 0 && bitboard -> player2_kings == 0)
		return 1;
	else {
		bool player1_moves_left = FALSE;
		bool player2_moves_left = FALSE;
		uint32_t empty_squares = ~(bitboard -> player1_pawns | bitboard -> player2_pawns | bitboard -> player1_kings | bitboard -> player2_kings);
		uint32_t player1_pieces = bitboard -> player1_pawns | bitboard -> player1_kings;
		uint32_t player2_pieces = bitboard -> player2_pawns | bitboard -> player2_kings;

		// For player 1 moves.
		if(bitboard -> player1_pawns)
			player1_moves_left = forward_moving_pieces(bitboard -> player1_pawns, player2_pieces, empty_squares);

		// Check if kings have moves left if there are no playable pawns.
		if(bitboard -> player1_kings && player1_moves_left == FALSE)
			player1_moves_left = reverse_moving_pieces(bitboard -> player1_kings, player2_pieces, empty_squares) || forward_moving_pieces(bitboard -> player1_kings, player2_pieces, empty_squares);

		// For player 2 moves.
		if(bitboard -> player2_pawns)
			player2_moves_left = reverse_moving_pieces(bitboard -> player2_pawns, player1_pieces, empty_squares);

		// Check if kings have moves left if there are no playable pawns.
		if(bitboard -> player2_kings && player2_moves_left == FALSE)
			player2_moves_left = forward_moving_pieces(bitboard -> player2_kings, player1_pieces, empty_squares) || reverse_moving_pieces(bitboard -> player2_kings, player1_pieces, empty_squares);

		if(player1_moves_left && player2_moves_left)
			return 0; // game not over yet.
		else if(player1_moves_left && !player2_moves_left)
			return 1; // player 1 won the game.
		else if(!player1_moves_left && player2_moves_left)
			return 2; // player 2 won the game.
		else
			return 3; // possible draw.
	}
}

void switch_player(player_t *player) {
	if(*player == PLAYER_1)
		*player = PLAYER_2;
	else if(*player == PLAYER_2)
		*player = PLAYER_1;
}

void clear_screen() {
    printf("\033[2J");
    printf("\033[H");
}

void inform_fj(fj_array *fj) {
	if(!is_fj_empty(fj)) {
		printf("\n");
		printf("Forced jump available : ");
		for(int i = 0; i < fj -> size; i++) {
			if(i > 0)
				printf(", ");
			printf("%c", (char)(get_col(fj -> jumps[i].from_index) + 97));
			printf("%d", (get_row(fj -> jumps[i].from_index) + 1));
			printf(" -> ");
			printf("%c", (char)(get_col(fj -> jumps[i].to_index) + 97));
			printf("%d", (get_row(fj -> jumps[i].to_index) + 1));
		}
		printf(".\n");
	}
	return;
}

void init_mt(move_table *mt) {
	mt -> children = (bitboard_t *)malloc(sizeof(bitboard_t) * MT_MAX_SIZE);
	mt -> size = 0;
	mt -> capacity = MT_MAX_SIZE;
}

void expand_mt_if_full(move_table *mt) {
	if(mt -> size == mt -> capacity) {
		mt -> capacity *= 2;
		mt -> children = (bitboard_t *)realloc(mt -> children, sizeof(bitboard_t) * mt -> capacity);
	}
	return;
}

bitboard_t move_fj(bitboard_t b, forced_jump *jump, player_t player) {
	uint32_t *from_pieces;
	uint32_t *to_pieces;
	uint32_t *opp_pieces;
	
	if(player == PLAYER_1) {
		if(b.player1_pawns & 1U << jump -> from_index) {
			from_pieces = &(b.player1_pawns);
			to_pieces = (jump -> to_index) > 27 ? &(b.player1_kings) : from_pieces;
		}
		else {
			from_pieces = &(b.player1_kings);
			to_pieces = from_pieces;
		}
		opp_pieces = b.player2_pawns & 1U << (jump -> capture_index) ? &(b.player2_pawns) : &(b.player2_kings);
	}
	else {
		if(b.player2_pawns & 1U << jump -> from_index) {
			from_pieces = &(b.player2_pawns);
			to_pieces = (jump -> to_index) < 4 ? &(b.player2_kings) : from_pieces;
		}
		else {
			from_pieces = &(b.player2_kings);
			to_pieces = from_pieces;
		}
		opp_pieces = b.player1_pawns & 1U << (jump -> capture_index) ? &(b.player1_pawns) : &(b.player1_kings);
	}
	*to_pieces |= 1U << jump -> to_index;
	*from_pieces &= ~(1U << jump -> from_index);
	*opp_pieces &= ~(1U << jump -> capture_index);
	return b;
}

bitboard_t generate_bitboard(bitboard_t b, unsigned short from_index, unsigned short to_index, player_t player) {
	uint32_t *from_pieces, *to_pieces;
	
	if(player == PLAYER_1) {
		if(b.player1_pawns & 1U << from_index) {
			from_pieces = &(b.player1_pawns);
			to_pieces = to_index > 27 ? &(b.player1_kings) : from_pieces;
		}
		else {
			from_pieces = &(b.player1_kings);
			to_pieces = from_pieces;
		}
	}
	else {
		if(b.player2_pawns & 1U << from_index) {
			from_pieces = &(b.player2_pawns);
			to_pieces = to_index < 4 ? &(b.player1_kings) : from_pieces;
		}
		else {
			from_pieces = &(b.player2_kings);
			to_pieces = from_pieces;
		}
	}
	*to_pieces |= 1U << to_index;
	*from_pieces &= ~(1U << from_index);
	return b;
}

void append_forward_moves(const bitboard_t *bitboard, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player) {
	short i;
	for(i = 27; i >= 0; i--) {
		if(player_pieces & (1 << i)) {
			expand_mt_if_full(mt);
			if(empty_squares & (1 << (i + 4))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i + 4, player);
				mt -> size++;
			}
			if((i / 4) % 2 == 0 && (i % 8 != 0 && (empty_squares & 1 << (i + 3)))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i + 3, player);
				mt -> size++;
			}
			if((i / 4) % 2 == 1 && (i % 8 != 7 && (empty_squares & 1 << (i + 5)))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i + 5, player);
				mt -> size++;
			}
		}
	}
	return;
}

void append_reverse_moves(const bitboard_t *bitboard, move_table *mt, uint32_t player_pieces, uint32_t empty_squares, player_t player) {
	short i;
	for(i = 4; i < BOARD_SIZE; i++) {
		if(player_pieces & (1 << i)) {
			expand_mt_if_full(mt);
			if(empty_squares & (1 << (i - 4))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i - 4, player);
				mt -> size++;
			}	
			if((i / 4) % 2 == 0 && (i % 8 != 0 && (empty_squares & 1 << (i - 5)))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i - 5, player);
				mt -> size++;
			}
			if((i / 4) % 2 == 1 && (i % 8 != 7 && (empty_squares & 1 << (i - 3)))) {
				mt -> children[mt -> size] = generate_bitboard(*bitboard, i, i - 3, player);
				mt -> size++;
			}
		}
	}
	return;
}

void generate_fj_moves(const bitboard_t *b, fj_array *fj, move_table *mt, player_t player) {
	short i;
	bitboard_t temp_bitboard;
	fj_array new_fj;
	init_fj_array(&new_fj);
	
	for(i = 0; i < fj -> size; i++) {
		temp_bitboard = move_fj(*b, &(fj -> jumps[i]), player);
		are_forced_jumps(&temp_bitboard, &new_fj, player);
		if(new_fj.size > 0) {
			for(short j = 0; j < new_fj.size; j++) {
				if(new_fj.jumps[j].from_index == fj -> jumps[i].to_index)
					generate_fj_moves(&temp_bitboard, &new_fj, mt, player);
			}
		}
		else {
			expand_mt_if_full(mt);
			mt -> children[mt -> size] = temp_bitboard;
			mt -> size++;
		}
	}
	
	free(new_fj.jumps);
	return;
}

void generate_moves(const bitboard_t *b, move_table *mt, fj_array *fj, player_t player) {
	are_forced_jumps(b, fj, player);
	init_mt(mt);
	
	if(is_fj_empty(fj)) {
		uint32_t empty_squares = ~(b -> player1_pawns | b -> player1_kings | b -> player2_pawns | b -> player2_kings);
		
		if(player == PLAYER_1) {
			uint32_t player_pieces = b -> player1_pawns | b -> player1_kings;
			if(b -> player1_kings)
				append_reverse_moves(b, mt, b -> player1_kings, empty_squares, player);
			append_forward_moves(b, mt, player_pieces, empty_squares, player);
		}
		else if(player == PLAYER_2) {
			uint32_t player_pieces = b -> player2_pawns | b -> player2_kings;
			if(b -> player2_kings)
				append_forward_moves(b, mt, b -> player2_kings, empty_squares, player);
			append_reverse_moves(b, mt, player_pieces, empty_squares, player);
		}
	}
	else {
		generate_fj_moves(b, fj, mt, player);
	}
	return;
}

short evaluate_board(const bitboard_t *b, player_t player, bool max_player, unsigned short game_result) {
	if(game_result) {
		if ((game_result == 1 && player == PLAYER_1 && max_player) || (game_result == 2 && player == PLAYER_2 && max_player))
        	return SHRT_MAX;
    	else
        	return SHRT_MIN;
	}
	
	short player1_score = 0;
	short player2_score = 0;
	
	player1_score = __builtin_popcount(b -> player1_pawns) * PAWN_VALUE + __builtin_popcount(b -> player1_kings) * KING_VALUE;
	player2_score = __builtin_popcount(b -> player2_pawns) * PAWN_VALUE + __builtin_popcount(b -> player2_kings) * KING_VALUE;
	player1_score += __builtin_popcount((b -> player1_pawns | b -> player1_kings) & CENTER_SQ) * CENTER_BONUS;
	player2_score += __builtin_popcount((b -> player2_pawns | b -> player2_kings) & CENTER_SQ) * CENTER_BONUS;
	
	if(max_player)
		return player == PLAYER_2 ? player2_score - player1_score : player1_score - player2_score;
	else
		return player == PLAYER_1 ? player2_score - player1_score : player1_score - player2_score;
}

short minimax(const bitboard_t *b, unsigned short depth, short alpha, short beta, bool max_player, player_t player, bitboard_t *best_move) {
	unsigned short game_status = is_game_over(b);
	if(depth == 0 || game_status)
		return evaluate_board(b, player, max_player, game_status);

	short score, best_score;

	move_table mt;
	init_mt(&mt);

	fj_array fj;
	init_fj_array(&fj);

	generate_moves(b, &mt, &fj, player);
	player_t next_player = player;
	switch_player(&next_player);

	// If maximizing player.
	if(max_player) {
		best_score = SHRT_MIN;
		for (int i = 0; i < mt.size; i++) {
			score = minimax(&(mt.children[i]), depth - 1, alpha, beta, FALSE, next_player, NULL);
			if(score > best_score) {
				best_score = score;
				if(best_move)
					*best_move = mt.children[i];
			}
			alpha = (alpha > best_score) ? alpha : best_score;
			if (beta <= alpha) {
       			break;
			}
		}
	}

	// If minimizing player.
	else {
		best_score = SHRT_MAX;
		for (int i = 0; i < mt.size; i++) {
			score = minimax(&(mt.children[i]), depth - 1, alpha, beta, TRUE, next_player, NULL);
			if(score < best_score) {
				best_score = score;
				if(best_move)
					*best_move = mt.children[i];
			}
			beta = (beta < best_score) ? beta : best_score;
			if (beta <= alpha) {
				break;
			}
		}
	}

	free(mt.children);
	free(fj.jumps);
    return best_score;
}

unsigned short human_move(board_t *board, game_history *history, fj_array *fj, player_t *current_player, bitboard_t *bitboard) {
		unsigned short from_index, to_index, game_status;

		get_move(board, &from_index, &to_index, fj, *current_player);
		if(from_index > INVALID || to_index > INVALID) {
			if(from_index < INVALID)
				return to_index;
			return from_index;
		}
		move_piece(board, from_index, to_index, fj);
		kinging(board, to_index);

		*bitboard = arr_to_bitboard(board);

		game_status = is_game_over(bitboard);
		if(game_status != 0) {
			funeral(board, history, fj, game_status);
			return FALSE;
		}

		if(!is_fj_empty(fj)) {
			are_forced_jumps(bitboard, fj, *current_player);
			if(is_fj_empty(fj))
				switch_player(current_player);
			else
				save_progress(bitboard, history, PLAYER_1);
		}
		else
			switch_player(current_player);
		return TRUE;
}

// Here PLAYER_2 is assumed as the AI player.
void AI_player(player_t starting_player) {
	board_t board;
	game_history history;
	fj_array fj;
	bitboard_t bitboard, ai_move;
	player_t current_player = starting_player;
	unsigned short action = 1, game_status = 0;

	init_board(&board, &history);
	init_bitboard(&bitboard);
	init_fj_array(&fj);

	if(starting_player == PLAYER_1)
		save_progress(&bitboard, &history, PLAYER_1);

	while(action) {
		clear_screen();
		display_board(&board);	
		inform_fj(&fj);
		printf("\n\n");
		if(current_player == PLAYER_1) {
			action = human_move(&board, &history, &fj, &current_player, &bitboard);
		
			if(action == UNDO)
				undo_action(&board, &history, &fj, &current_player);
			else if(action == REDO)
				redo_action(&board, &history, &fj, &current_player);
			if(action == QUIT) {
				funeral(&board, &history, &fj, QUIT);
				break;
			}
		}

		if(current_player == PLAYER_2) {
			minimax(&bitboard, DEPTH, SHRT_MIN, SHRT_MAX, TRUE, PLAYER_2, &ai_move);
			game_status = is_game_over(&ai_move);
			if(game_status) {
				funeral(&board, &history, &fj, game_status);
				break;
			}
			switch_player(&current_player);
			save_progress(&ai_move, &history, PLAYER_1);
			are_forced_jumps(&ai_move, &fj, PLAYER_1);
			bitboard_to_arr(&ai_move, &board);
		}
	}
	return;
}
