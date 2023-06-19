/*
    Written by Grady Fitzpatrick for 
    COMP20007 Assignment 2 2023 Semester 1
    
     Header for module which contains Problem 2-related 
        data structures and functions.
*/
#include <stdio.h>

struct problem;
struct solution;

/* 
    Reads the given text file into a set of tokens in a sentence 
    and the given table file into a set of structs.
*/
struct problem *readProblemA(FILE *textFile, FILE *tableFile);

/* 
    Reads the given text file into a set of tokens in a sentence,
    the given table file and the given transition table into a set 
    of structs.
*/
struct problem *readProblemB(FILE *textFile, FILE *tableFile, 
    FILE *transTable);

/*
    Same as Problem B, but part is set for Part E.
*/
struct problem *readProblemE(FILE *textFile, FILE *tableFile, 
    FILE *transTable);

/*
    Same as Problem B, but part is set for Part F.
*/
struct problem *readProblemF(FILE *textFile, FILE *tableFile, 
    FILE *transTable);

/*
    Solves the given problem according to Part A's definition
    and places the solution output into a returned solution value.
*/
struct solution *solveProblemA(struct problem *p);

/*
    Solves the given problem according to Part B's definition
    and places the solution output into a returned solution value.
*/
struct solution *solveProblemB(struct problem *p);

/*
    Solves the given problem according to Part E's definition
    and places the solution output into a returned solution value.
*/
struct solution *solveProblemE(struct problem *p);

/*
    Solves the given problem according to Part F's definition
    and places the solution output into a returned solution value.
*/
struct solution *solveProblemF(struct problem *p);

/*
    Outputs the given solution to the given file. If colourMode is 1, the
    sentence in the problem is coloured with the given solution colours.
*/
void outputProblem(struct problem *problem, struct solution *solution, FILE *stdout, 
    int colourMode);

/*
    Frees the given solution and all memory allocated for it.
*/
void freeSolution(struct solution *solution, struct problem *problem);

/*
    Frees the given problem and all memory allocated for it.
*/
void freeProblem(struct problem *problem);

/*
    Helper function to create the DP array.
*/
int **createDPArray(struct problem *p);

/*
    Helper function to find the score from transition colour table.
*/
int getTransitionScore(struct problem *p, int prevColour, int currentColour);

/*
     Helper function to initialize the DP array for the first term.
*/
void computeFirstTerm(struct problem *p, int **DP, 
    int *maxColour, int *maxScore, int *prevColour);

/*
     Helper function to check if there is colour table for the term.
*/
struct termColourTable* findTermTable(struct problem *p, int termIndex);

