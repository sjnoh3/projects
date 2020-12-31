package src;

/*

AUTHOR:      John Lu

DESCRIPTION: This file contains your agent class, which you will
             implement.

NOTES:       - If you are having trouble understanding how the shell
               works, look at the other parts of the code, as well as
               the documentation.

             - You are only allowed to make changes to this portion of
               the code. Any changes to other portions of the code will
               be lost when the tournament runs your code.
*/

import src.Action.ACTION;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Random;
import java.util.stream.Collectors;

public class MyAI extends AI {
    // ########################## INSTRUCTIONS ##########################
    // 1) The Minesweeper Shell will pass in the board size, number of mines
    // 	  and first move coordinates to your agent. Create any instance variables
    //    necessary to store these variables.
    //
    // 2) You MUST implement the getAction() method which has a single parameter,
    // 	  number. If your most recent move is an src.Action.UNCOVER action, this value will
    //	  be the number of the tile just uncovered. If your most recent move is
    //    not src.Action.UNCOVER, then the value will be -1.
    //
    // 3) Feel free to implement any helper functions.
    //
    // ###################### END OF INSTURCTIONS #######################

    // This line is to remove compiler warnings related to using Java generics
    // if you decide to do so in your implementation.
    @SuppressWarnings("unchecked")

    private static final int FLAGGED = -2;
    private static final int COVERED = -1;

    private final int rowDimension;
    private final int colDimension;
    private final int totalMines;

    private final int[][] currentMap;

    private int uncovered;
    private Move currMove;
    private ACTION currAction;

    public MyAI(int rowDimension, int colDimension, int totalMines, int startX, int startY) {
        // ################### Implement Constructor (required) ####################
        this.rowDimension = rowDimension;
        this.colDimension = colDimension;
        this.totalMines = totalMines;

        currentMap = new int[rowDimension][colDimension];
        for (int i = 0; i < rowDimension; i++) {
            for (int j = 0; j < colDimension; j++) {
                currentMap[i][j] = COVERED;
            }
        }
        uncovered = 0;
    }

    // ################## Implement getAction(), (required) #####################
    public Action getAction(int number) {
        if (currAction == ACTION.UNCOVER)
            currentMap[currMove.x][currMove.y] = number;

        //print();
        currMove = null;
        if (uncovered == rowDimension * colDimension - totalMines)
            return new Action(ACTION.LEAVE);

        currMove = findFlagMove();
        if (currMove != null) {
            currentMap[currMove.x][currMove.y] = FLAGGED;
            currAction = ACTION.FLAG;
            return new Action(currAction, currMove.x + 1, currMove.y + 1);
        }

        uncovered++;
		currMove = findUncoverMove();
		currAction = ACTION.UNCOVER;

		if (currMove != null) {
            return new Action(currAction, currMove.x + 1, currMove.y + 1);
        }
        currMove = findRandomMove();
        return new Action(currAction, currMove.x + 1, currMove.y + 1);
    }

    // ################### Helper Functions Go Here (optional) ##################
    // ...
    private Move findFlagMove() {
        for (int i = 0; i < rowDimension; i++) {
            for (int j = 0; j < colDimension; j++) {
                int number = currentMap[i][j];
                if (number == COVERED || number == FLAGGED)
                    continue;
                Collection<Move> adjacent = getAdjacentMoves(i, j);
                Collection<Move> covered = adjacent.stream().filter(move -> currentMap[move.x][move.y] == COVERED).collect(Collectors.toList());
                if (covered.isEmpty())
                    continue;
                long flagged = adjacent.stream().filter(move -> currentMap[move.x][move.y] == FLAGGED).count();
                if (covered.size() + flagged == number) {
                    return covered.iterator().next();
                }
            }
        }
        return null;
    }

    private Move findUncoverMove() {
        for (int i = 0; i < rowDimension; i++) {
            for (int j = 0; j < colDimension; j++) {
                int number = currentMap[i][j];
                if (number == COVERED || number == FLAGGED)
                    continue;
                Collection<Move> adjacent = getAdjacentMoves(i, j);
                Collection<Move> covered = adjacent.stream().filter(move -> currentMap[move.x][move.y] == COVERED).collect(Collectors.toList());
                if (covered.isEmpty())
                    continue;
                long flagged = adjacent.stream().filter(move -> currentMap[move.x][move.y] == FLAGGED).count();
                if (flagged == number) {
                    return covered.iterator().next();
                }
            }
        }
        return null;
    }

    private Move findRandomMove() {
        List<Move> covered = new ArrayList<>();
        for (int i = 0; i < rowDimension; i++) {
            for (int j = 0; j < colDimension; j++) {
                if (currentMap[i][j] == COVERED)
                    covered.add(new Move(i, j));
            }
        }
        Random random = new Random();
        return covered.get(random.nextInt(covered.size()));
    }

    private Collection<Move> getAdjacentMoves(int i, int j) {
        Collection<Move> result = new ArrayList<>();
        if (i > 0) {
            result.add(new Move(i - 1, j));
        }
        if (i < rowDimension - 1) {
            result.add(new Move(i + 1, j));
        }
        if (j > 0) {
            result.add(new Move(i, j - 1));
        }
        if (j < colDimension - 1) {
            result.add(new Move(i, j + 1));
        }
        if ((i > 0) && (j > 0)) {
            result.add(new Move(i - 1, j - 1));
        }
        if ((i > 0) && (j < colDimension - 1)) {
            result.add(new Move(i - 1, j + 1));
        }

        if ((i < rowDimension - 1) && (j > 0)) {
            result.add(new Move(i + 1, j - 1));
        }

        if ((i < rowDimension - 1) && (j < colDimension - 1)) {
            result.add(new Move(i + 1, j + 1));
        }
        return result;
    }

    private static class Move {
        int x;
        int y;

        Move(int x, int y) {
            this.x = x;
            this.y = y;
        }
    }

    void print() {
        StringBuilder builder = new StringBuilder();
        for (int i = colDimension - 1; i >= 0; i--) {
            for (int j = 0; j < rowDimension; j++) {
                builder.append("[");
                int number = currentMap[j][i];
                if (number >= 0) {
                    builder.append(number);
                } else {
                    if (number == COVERED) {
                        builder.append(' ');
                    } else {
                        builder.append('X');
                    }
                }
                builder.append(']');
            }
            builder.append(System.lineSeparator());
        }
        System.out.println(builder.toString());
    }
}
