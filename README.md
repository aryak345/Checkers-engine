# Checkers-engine

Checkers Game with AI (C): 

This is a terminal-based Checkers game implemented in C, featuring full rule enforcement and an AI opponent powered by the Minimax algorithm with alpha-beta pruning. The project uses bitboard representations for efficient board state management and includes functionality for forced captures, king promotion, and undo/redo support.

Features:
1. Single-player gameplay with an AI opponent (Human vs Computer)
2. Fully functional Checkers rules:
   a. Legal move validation
   b. Mandatory captures (forced jumps)
   c. King promotion
   d. Game over detection (win, lose, draw)
3. AI opponent using Minimax search with alpha-beta pruning
4. Compact representation of board state using 32-bit bitboards
5. Undo and redo functionality implemented using linked stacks
6. Text-based board rendering and command-line input

Technologies Used:
1. Language: C
2. Bit manipulation for board representation
3. Custom data structures (linked lists for move history, structs for board state)
4. Minimax algorithm with alpha-beta pruning for AI move selection

How It Works:
1. The board is represented using a 32-square model (only valid playing squares).
2. Game logic uses bitboard operations (bitwise shifts, masks) for efficient move generation.
3. The AI chooses optimal moves by evaluating board states recursively using Minimax.
4. Players can enter moves like b6 c5, or type commands like undo, redo, and quit.

AI Strategy:
The AI uses a depth-limited Minimax algorithm with alpha-beta pruning to evaluate possible moves. The evaluation function considers the number of pawns, kings, and central position control, with weighted scores. Forced captures are generated and evaluated during move generation.

Sample Output:

  +---+---+---+---+---+---+---+---+
8 | . | O | . | O | . | O | . | O |
7 | O | . | O | . | O | . | O | . |
6 | . | O | . | O | . | O | . | O |
5 | . | . | . | . | . | . | . | . |
4 | . | . | . | . | . | . | . | . |
3 | X | . | X | . | X | . | X | . |
2 | . | X | . | X | . | X | . | X |
1 | X | . | X | . | X | . | X | . |
  +---+---+---+---+---+---+---+---+
    A   B   C   D   E   F   G   H  


Its Player X's turn
From:

Compilation:
Make sure you have a C compiler like gcc installed.

gcc -o checkers main.c board.c game_logic.c game_loop.c minimax.c

./checkers

Requirements:
GCC or any C99-compatible compiler

Works on Linux, macOS, and Windows (with terminal support)

Future Improvements:
1. Support for two-player mode
2. Configurable AI difficulty (search depth)
3. Enhanced input parsing (to support full move notation)
4. Support save/load game state from file

License:
This project is open-source.

Credits:
1. Developed as a personal project to explore:
2. Low-level game development
3. AI algorithms (Minimax + Alpha-Beta)
4. Memory-efficient data modeling using bitboards
