// This program solves the gopher puzzle from Binary Arts.
// There are six plastic pieces.  Each piece is a strip of three squares.
// A square can have a whole or a gopher head sticking out of one side.
// if a head sticks out one side, the other side is flat.  The other
// option for a square is that it has a hole that can fit a gopher head.
// The bottom and top layers are each a 3x3 grid of holes.  The object
// is the place the six pieces in a way that fits into a cube.
// Copyright Scott Johnson 1998-2023

#include <stdio.h>

enum spaceType { SPACE_EMPTY, SPACE_HOLE, SPACE_BUMP_UP, SPACE_BUMP_DOWN };
enum { TILE_LEFT_RIGHT, TILE_UP_DOWN };

int	iterations;

class CPiece
{
public:
	CPiece();
	CPiece(const CPiece &);
	CPiece(int s1, int s2, int s3);

	int		operator[](int index);
	CPiece&		operator=(const CPiece&);

	void		Flip();
	void		Reverse();

private:

	int		flipped;
	int		reversed;
	spaceType	piece[3];
};

CPiece::CPiece()
{
	flipped = 0;
	reversed = 0;

	piece[0] = piece[1] = piece[2] = SPACE_EMPTY;
}

CPiece::CPiece(const CPiece &p)
{
	*this = p;
}

CPiece::CPiece(int s1, int s2, int s3)
{
	flipped = 0;
	reversed = 0;

	piece[0] = (spaceType) s1;
	piece[1] = (spaceType) s2;
	piece[2] = (spaceType) s3;
}

int	CPiece::operator[](int index)
{
	spaceType	piecePart;

	if (reversed)
	{
		piecePart = piece[3 - index - 1];
	} else
	{
		piecePart = piece[index];
	}
	if (flipped)
	{
		if (piecePart == SPACE_BUMP_UP)
		{
			piecePart = SPACE_BUMP_DOWN;
		
		} else if (piecePart == SPACE_BUMP_DOWN)
		{
			piecePart = SPACE_BUMP_UP;
		}
	}
	return((int) piecePart);
}

CPiece&	CPiece::operator=(const CPiece &p)
{
	if (&p == this) return(*this);

	flipped = p.flipped;
	reversed = p.reversed;

	for (int i = 0; i < 3; i++)
	{
		piece[i] = p.piece[i];
	}

	return(*this);
}

void CPiece::Reverse()
{
	reversed = !reversed;
}

void CPiece::Flip()
{
	flipped = !flipped;
}

/////////////////////////////////////////////////////////////////////////////

class CPuzzle
{
public:
	CPuzzle();
	CPuzzle(const CPuzzle &);

	int	Solve();
	void	Print();

	CPuzzle&	operator=(const CPuzzle &);

private:
	int	AlreadyPlaced(int pieceIndex);
	int	PlacePiece(CPiece &);
	char	GetChar(int code);

private:
	enum	{ LEVEL_COUNT = 4, DIM = 3 };

	spaceType	spaces[DIM][LEVEL_COUNT][DIM];	// represents each space of the puzzle

	int	pieceCount;				// how many total pieces exist
	int	placedCount;				// how many pieces have been placed in the puzzle
	int	placed[6];
	int	tileDirection[LEVEL_COUNT];		// tells if each level tiles left/right or up/down

	int	level;					// the current level being worked on

	static	CPiece	pieces[6];

};

CPiece CPuzzle::pieces[6] =
{
	CPiece(SPACE_BUMP_UP	, SPACE_BUMP_DOWN, SPACE_BUMP_UP),
	CPiece(SPACE_BUMP_DOWN	, SPACE_HOLE     , SPACE_BUMP_UP),
	CPiece(SPACE_BUMP_DOWN	, SPACE_BUMP_DOWN, SPACE_BUMP_UP),
	CPiece(SPACE_BUMP_DOWN	, SPACE_HOLE     , SPACE_HOLE   ),
	CPiece(SPACE_BUMP_DOWN	, SPACE_BUMP_UP  , SPACE_HOLE   ),
	CPiece(SPACE_BUMP_DOWN	, SPACE_BUMP_UP  , SPACE_HOLE   )
};

CPuzzle::CPuzzle()
{
	int	i, j, k;

	pieceCount = 6;
	placedCount = 0;

	for (i = 0; i < DIM; i++)
	{
		for (j = 0; j < LEVEL_COUNT; j++)
		{
			for (k = 0; k < DIM; k++)
			{
				if (j == 0 || j == 3)
				{
					// 1st level is all holes
					spaces[i][j][k] = SPACE_HOLE;
				} else
				{
					// other levels are initially empty
					spaces[i][j][k] = SPACE_EMPTY;
				}
			}
		}
	}
	
	for (i = 0; i < LEVEL_COUNT; i++)
	{
		tileDirection[i] = TILE_LEFT_RIGHT;
	}

	for (i = 0; i < pieceCount; i++)
	{
		placed[i] = -1;
	}

	level = 0;
}

CPuzzle::CPuzzle(const CPuzzle &p)
{
	*this = p;
}

CPuzzle& CPuzzle::operator=(const CPuzzle &p)
{
	int	i, j, k;

	if (&p == this) return(*this);

	for (i = 0; i < DIM; i++)
	{
		for (j = 0; j < LEVEL_COUNT; j++)
		{
			for (k = 0; k < DIM; k++)
			{
				spaces[i][j][k] = p.spaces[i][j][k];
			}
		}
	}
	pieceCount = p.pieceCount;
	placedCount = p.placedCount;

	for (i = 0; i < pieceCount; i++)
	{
		placed[i] = p.placed[i];
	}

	for (i = 0; i < LEVEL_COUNT; i++)
	{
		tileDirection[i] = p.tileDirection[i];
	}

	level = p.level;

	return(*this);
}

int	fitsAbove(int pieceCode, int pieceBelowCode)
{
	if (pieceBelowCode == SPACE_EMPTY || pieceCode == SPACE_EMPTY)
	{
		throw("puzzle solution is messed up");
	}

	// Anything goes over a hole
	if (pieceBelowCode == SPACE_HOLE)
	{
		return(1);
	}

	if (pieceBelowCode == SPACE_BUMP_UP)
	{
		return(pieceCode == SPACE_HOLE);
	}

	if (pieceBelowCode == SPACE_BUMP_DOWN)
	{
		return(pieceCode == SPACE_HOLE || pieceCode == SPACE_BUMP_UP);
	}

	throw("fitsAbove: illegal input");
	return(0);	// compiler wants a return value
}

int	CPuzzle::Solve()
{
	int	solved = 0;
	int	i;
	int	directionIter;
	int	orientation;
	CPuzzle	newPuzzle;
	CPiece	newPiece;

	Print();
	//getchar();

	// current level
	level = placedCount / DIM + 1;
	
	if (placedCount < pieceCount)
	{
		for (directionIter = 0; directionIter < 2 && !solved; directionIter++)
		{
			// Try each piece in the next place

			for (i = 0; i < pieceCount && !solved; i++)
			{
				// if piece i not already placed in the current puzzle...

				if (!AlreadyPlaced(i))
				{
					// Try each piece in every orientation
					
					for (orientation = 0; orientation < 4 && ! solved; orientation++)
					{
						newPiece = pieces[i];

						if (orientation & 0x1) 
						{
							newPiece.Flip();
						}
						if (orientation & 0x02)
						{
							newPiece.Reverse();
						}

						newPuzzle = *this;
					
						// add piece i to the new puzzle

						if (newPuzzle.PlacePiece(newPiece))
						{
							// addition of that piece is valid

							newPuzzle.placed[placedCount] = i;
							newPuzzle.placedCount++;

							// Solve the new puzzle

							solved = newPuzzle.Solve();
							if (solved)
							{
								*this = newPuzzle;
							}
						}
					}
				}
			}

			// If it is not solved and you are trying to place one
			// at the start of a level other than the first then
			// try tiling in the other direction.

			if (!solved && ((placedCount % DIM) == 0) && placedCount >= DIM)
			{
				if (tileDirection[level] == TILE_LEFT_RIGHT)
				{
					tileDirection[level] = TILE_UP_DOWN;
				}
			} else
			{
				break;
			}
		}
	} else
	{
		// Puzzle is solved
		return(1);
	}

	return(solved);
}

// based on the puzzle's state, add the new piece

int	CPuzzle::PlacePiece(CPiece &piece)
{
	int		i;
	int		fit = 0;
	spaceType	piecePart;
				
	iterations++;
	
	// current level
	level = placedCount / DIM + 1;
		
	// iterate across the portions of a piece
	for (i = 0; i < DIM; i++)
	{
		piecePart = (spaceType) piece[i];

		if (tileDirection[level] == TILE_LEFT_RIGHT)
		{
			// piece's length goes left to right

			fit = fitsAbove(piecePart, spaces[i][level - 1][placedCount % 3]);
			if (!fit)
			{
				// doesn't fit
				break;
			}
		} else
		{
			// length goes up/down

			fit = fitsAbove(piecePart, spaces[placedCount % 3][level - 1][i]);
			if (!fit)
			{
				// doesn't fit
				break;
			}
		}
	}

	if (fit)
	{
		// Place the piece into the puzzle space

		for (i = 0; i < 3; i++)
		{
			piecePart = (spaceType) piece[i];
			if (tileDirection[level] == TILE_LEFT_RIGHT)
			{
				spaces[i][level][placedCount % 3] = piecePart;

			} else
			{
				spaces[placedCount % 3][level][i] = piecePart;
			}
		}
	}
	
	return(fit);
}

int	CPuzzle::AlreadyPlaced(int piece)
{
	int i;

	for (i = 0 ; i < pieceCount; i++)
	{
		if (placed[i] == piece)
		{
			return(1);
		}
	}

	return(0);
}

char	CPuzzle::GetChar(int code)
{
	switch(code)
	{
		case SPACE_EMPTY: return('_');
		case SPACE_BUMP_UP: return('^');
		case SPACE_BUMP_DOWN : return('v');
		case SPACE_HOLE : return('O');
	}

	return('?');
}

void	CPuzzle::Print()
{
	int	l;
		
	printf("======\n");
	
	for (l = 0; l < 4; l++)
	{
		if (tileDirection[l] == TILE_UP_DOWN)
		{
			printf("layer #%d : tiled up/down\n", l + 1);
		} else
		{
			printf("layer #%d : tiled left/right\n", l + 1);
		}
		printf("%c %c %c\n", GetChar(spaces[0][l][0]), GetChar(spaces[1][l][0]), GetChar(spaces[2][l][0]));
		printf("%c %c %c\n", GetChar(spaces[0][l][1]), GetChar(spaces[1][l][1]), GetChar(spaces[2][l][1]));
		printf("%c %c %c\n", GetChar(spaces[0][l][2]), GetChar(spaces[1][l][2]), GetChar(spaces[2][l][2]));
	}
}

int main(int argc, char *argv[])
{
	CPuzzle	puzzle;

	iterations = 0;

	if (puzzle.Solve())
	{
		// solved
		printf("Final puzzle\n");
		puzzle.Print();
	} else
	{
		printf("No solution\n");
	}

	printf("iterations: %d\n", iterations);
    
	return(0);
}