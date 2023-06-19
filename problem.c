/*
    Written by Grady Fitzpatrick for 
    COMP20007 Assignment 2 2023 Semester 1
    
    Implementation for module which contains  
        Problem 2-related data structures and 
        functions.
    
    Code implemented by <YOU>
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "problem.h"
#include "problemStruct.c"
#include "solutionStruct.c"

/* Number of terms to allocate space for initially. */
#define INITIALTERMS 64

/* Number of colour transitions to allocate space for initially. */
#define INITIALTRANSITIONS 16

/* Number of possible colour index cases. */
#define COLOURCASES 4

/* -1 to show the colour hasn't been set. */
#define DEFAULTCOLOUR (-1)
/* -1 to be lower than zero to highlight in case accidentally used. */
#define DEFAULTSCORE (-1)

/* No colour is assigned where no highlighting rules are present. */
#define NO_COLOUR (0)

/* Marker for non-allowed colours. */
#define NONALLOWED (INT_MIN / 2)

struct problem;
struct solution;

/* Sets up a solution for the given problem. */
struct solution *newSolution(struct problem *problem);

/* 
    Reads the given text file into a set of tokens in a sentence 
    and the given table file into a set of structs.

    Assumption: Tables are always contiguous, meaning the table never
    needs to be constructed 
*/
struct problem *readProblemA(FILE *textFile, FILE *tableFile){
    struct problem *p = (struct problem *) malloc(sizeof(struct problem));
    assert(p);

    /* Part B onwards so set as empty. */
    p->colourTransitionTable = NULL;

    int termCount = 0;
    char *text = NULL;
    char **terms = NULL;

    int termColourTableCount = 0;
    struct termColourTable *colourTables = NULL;

    /* Read in text. */
    size_t allocated = 0;
    /* Exit if we read no characters or an error caught. */
    int success = getdelim(&text, &allocated, '\0', textFile);

    if(success == -1){
        /* Encountered an error. */
        perror("Encountered error reading text file");
        exit(EXIT_FAILURE);
    } else {
        /* Assume file contains at least one character. */
        assert(success > 0);
    }

    char *tableText = NULL;
    /* Reset allocated marker. */
    allocated = 0;
    success = getdelim(&tableText, &allocated, '\0', tableFile);

    if(success == -1){
        /* Encountered an error. */
        perror("Encountered error reading table file");
        exit(EXIT_FAILURE);
    } else {
        /* Assume file contains at least one character. */
        assert(success > 0);
    }

    /* Read term table first. */
    int allocatedColourTables = 0;
    /* Progress through string. */
    int progress = 0;
    /* Table string length. */
    int tableTextLength = strlen(tableText);
    char *lastToken = NULL;
    struct termColourTable *lastTable = NULL;

    while(progress < tableTextLength){
        char *token = NULL;
        int score;
        int colour;
        int nextProgress;
        /* Make sure a token, colour and score are grabbed for each line. For simplicity, freshly allocate the token. */
        assert(sscanf(tableText + progress, "%m[^,],%d,%d %n", &token, &colour, &score, &nextProgress) == 3);
        assert(nextProgress > 0);
        progress += nextProgress;

        if(lastToken == NULL || strcmp(token, lastToken) != 0){
            /* New token, so build new table and add it to problem. */
            // fprintf(stderr, "New token: %s (colour #%d) (%d)\n", token, colour, score);
            lastToken = token;

            if(termColourTableCount == 0){
                /* Allocate initial. */
                colourTables = (struct termColourTable *) malloc(sizeof(struct termColourTable) * INITIALTERMS);
                assert(colourTables);
                allocatedColourTables = INITIALTERMS;
            } else {
                if((termColourTableCount + 1) >= allocatedColourTables){
                    /* Need more space for next table. */
                    colourTables = (struct termColourTable *) realloc(colourTables, sizeof(struct termColourTable) * allocatedColourTables * 2);
                    assert(colourTables);
                    allocatedColourTables = allocatedColourTables * 2;
                }
            }
            /* Set last table as fresh table. */
            lastTable = &(colourTables[termColourTableCount]);
            termColourTableCount++;
            /* Initialise table. */
            lastTable->term = token;
            lastTable->colourCount = 0;
            lastTable->colours = NULL;
            lastTable->scores = NULL;
        } else {
            /* Same as last token add info to new table. */
            // fprintf(stderr, "Same token: %s (colour #%d) (%d)\n", token, colour, score);
            /* Don't need token so free it. */
            free(token);
            token = NULL;
            /* Connect with existing token. */
            token = lastTable->term;
        }
        /* See if we need to increase the space for colours and scores for those colours. */
        if(lastTable->colourCount <= colour){
            int lastCount = lastTable->colourCount;
            lastTable->colours = (int *) realloc(lastTable->colours, sizeof(int) * (colour + 1));
            assert(lastTable->colours);
            lastTable->scores = (int *) realloc(lastTable->scores, sizeof(int) * (colour + 1));
            assert(lastTable->scores);
            for(int i = lastCount; i < colour; i++){
                lastTable->colours[i] = DEFAULTCOLOUR;
                lastTable->scores[i] = DEFAULTSCORE;
            }
            lastTable->colourCount = colour + 1;
        }
        /* Store info. */
        (lastTable->colours)[colour] = colour;
        (lastTable->scores)[colour] = score;

        /* Print new updated table */
        // fprintf(stderr, "Term: %s\n", lastTable->term);
        // fprintf(stderr, "Colour | Score\n");
        // for(int i = 0; i < lastTable->colourCount; i++){
        //     fprintf(stderr, "%6d | %d\n", lastTable->colours[i], lastTable->scores[i]);
        // }
    }

    /* Compress table to not have empty tables. */
    if(colourTables){
        colourTables = (struct termColourTable *) realloc(colourTables, sizeof(struct termColourTable) * termColourTableCount);
        assert(colourTables);
        allocatedColourTables = termColourTableCount;
    }
    
    /* Done with tableText */
    if(tableText){
        free(tableText);
    }
    
    /* Now split into terms */
    progress = 0;
    int termsAllocated = 0;
    int textLength = strlen(text);
    while(progress < textLength){
        /* This does greedy term matching - this generally follows the specification
            but also allows for more complex cases (e.g. "Big Oh"). */
        char *nextTerm = NULL;
        int start;
        int maxLengthGreedyMatch = 0;
        int nextProgress;
        while(text[progress] != '\0' && ! isalpha(text[progress])){
            progress++;
        }
        start = progress;
        /* Calculate remaining character count to avoid edge case complications */
        int remChars = textLength - start;
        /* See if any of the terms in the table match. */
        for(int i = 0; i < termColourTableCount; i++){
            char *term = colourTables[i].term;
            int termLen = strlen(term);
            if(termLen > remChars){
                /* Not enough characters to fit term. */
                continue;
            }
            /* Check if word boundary. */
            if(!isalpha(text[progress + termLen])){
                /* Yep! See if the term matches */
                int matching = 1;
                for(int j = 0; j < termLen; j++){
                    if(tolower(text[progress + j]) != tolower(term[j])){
                        /* Non-match */
                        matching = 0;
                        break;
                    }
                }
                if(matching){
                    /* Match, see if better than our current best. */
                    if(termLen > maxLengthGreedyMatch){
                        maxLengthGreedyMatch = termLen;
                        nextTerm = term;
                    }
                }
            }
        }
        if(! nextTerm){
            /* No match found, try finding word. This may consume punctuation, 
                this doesn't really matter. */
            assert(sscanf(text + start, " %ms %n", &nextTerm, &nextProgress) == 1);
            progress += nextProgress;
        } else {
            progress += maxLengthGreedyMatch;
            /* Move over punctuation if needed. */
            while(text[progress] != '\0' && ! isalpha(text[progress])){
                progress++;
            }
        }
        if(termsAllocated == 0){
            terms = (char **) malloc(sizeof(char *) * INITIALTERMS);
            assert(terms);
            termsAllocated = INITIALTERMS;
        } else if(termCount >= termsAllocated) {
            terms = (char **) realloc(terms, sizeof(char *) * termsAllocated * 2);
            assert(terms);
            termsAllocated = termsAllocated * 2;
        }
        terms[termCount] = nextTerm;
        // fprintf(stderr, "(%s) ", nextTerm);
        termCount++;
    }
    // fprintf(stderr, "\n");

    p->termCount = termCount;
    p->text = text;
    p->terms = terms;

    p->termColourTableCount = termColourTableCount;
    p->colourTables = colourTables;

    p->part = PART_A;

    return p;
}

struct problem *readProblemB(FILE *textFile, FILE *tableFile, 
    FILE *transTable){
    /* Fill in Part A sections. */
    struct problem *p = readProblemA(textFile, tableFile);

    /* Fill in Part B sections. */
    p->colourTransitionTable = (struct colourTransitionTable *) malloc(sizeof(struct colourTransitionTable));
    assert(p->colourTransitionTable);
    int transitionCount = 0;
    int transitionAllocated = 0;
    int *prevColours = NULL;
    int *colours = NULL;
    int *scores = NULL;

    int prevColour;
    int colour;
    int score;

    while(fscanf(transTable, "%d,%d,%d ", &prevColour, &colour, &score) == 3){
        if(transitionAllocated == 0){
            prevColours = (int *) malloc(sizeof(int) * INITIALTRANSITIONS);
            assert(prevColours);
            colours = (int *) malloc(sizeof(int) * INITIALTRANSITIONS);
            assert(colours);
            scores = (int *) malloc(sizeof(int) * INITIALTRANSITIONS);
            assert(scores);
            transitionAllocated = INITIALTRANSITIONS;
        } else if(transitionCount >= transitionAllocated) {
            prevColours = (int *) realloc(prevColours, sizeof(int) * transitionAllocated * 2);
            assert(prevColours);
            colours = (int *) realloc(colours, sizeof(int) * transitionAllocated * 2);
            assert(colours);
            scores = (int *) realloc(scores, sizeof(int) * transitionAllocated * 2);
            assert(scores);
            transitionAllocated = transitionAllocated * 2;
        }
        prevColours[transitionCount] = prevColour;
        colours[transitionCount] = colour;
        scores[transitionCount] = score;
        transitionCount++;
    }

    p->colourTransitionTable->transitionCount = transitionCount;
    p->colourTransitionTable->prevColours = prevColours;
    p->colourTransitionTable->colours = colours;
    p->colourTransitionTable->scores = scores;

    p->part = PART_B;
    return p;
}

struct problem *readProblemE(FILE *textFile, FILE *tableFile, 
    FILE *transTable){
    /* Interpretation of inputs is same as Part B. */
    struct problem *p = readProblemB(textFile, tableFile, transTable);
    
    p->part = PART_E;
    return p;
}

struct problem *readProblemF(FILE *textFile, FILE *tableFile, 
    FILE *transTable){
    /* Interpretation of inputs is same as Part B. */
    struct problem *p = readProblemB(textFile, tableFile, transTable);
    
    p->part = PART_F;
    return p;
}

/*
    Outputs the given solution to the given file. If colourMode is 1, the
    sentence in the problem is coloured with the given solution colours.
*/
void outputProblem(struct problem *problem, struct solution *solution, FILE *stdout, 
    int colourMode){
    assert(problem->termCount == solution->termCount);
    if(! colourMode){
        switch(problem->part){
            case PART_A:
            case PART_B:
            case PART_F:
                for(int i = 0; i < problem->termCount; i++){
                    if(i != 0){
                        printf(" ");
                    }
                    printf("%d", solution->termColours[i]);
                }
                printf("\n");
                break;

            case PART_E:
                printf("%d\n", solution->score);
                break;
        }
    } else {
        const char *(COLOURS_FG[]) = { "\033[38;5;0m"  , "\033[38;5;0m",  "\033[38;5;0m",  "\033[38;5;0m"  };
        const char *(COLOURS_BG[]) = { "\033[48;5;231m", "\033[48;5;10m", "\033[48;5;11m", "\033[48;5;12m" };
        const char *COLOURS_FG_ERROR = "\033[38;5;1m";
        const char *ENDCODE = "\033[0m";
        const int colourCount = (int) (sizeof(COLOURS_FG) / sizeof(COLOURS_FG[0]));

        for(int i = 0; i < problem->termCount; i++){
            if(i != 0){
                printf(" ");
            }
            /* Place colour code */
            if(solution->termColours[i] < 0 || solution->termColours[i] >= colourCount){
                printf("%s%s%s", COLOURS_FG_ERROR, problem->terms[i], ENDCODE);
            } else {
                printf("%s%s%s%s", COLOURS_FG[solution->termColours[i]], COLOURS_BG[solution->termColours[i]], problem->terms[i], ENDCODE);
            }
        }
        printf("\n");
    }
}

/*
    Frees the given solution and all memory allocated for it.
*/
void freeSolution(struct solution *solution, struct problem *problem){
    if(solution){
        if(solution->termColours){
            free(solution->termColours);
        }
        free(solution);
    }
}

/*
    Frees the given problem and all memory allocated for it.
*/
void freeProblem(struct problem *problem){
    if(problem){
        /* Free terms. */
        for(int i = 0; i < problem->termCount; i++){
            /* Don't free terms in colour table as we'll get them later. */
            int inTable = 0;
            for(int j = 0; j < problem->termColourTableCount; j++){
                /* Note we do == because we care about the pointer not the contents. */
                if(problem->terms[i] == problem->colourTables[j].term){
                    inTable = 1;
                    break;
                }
            }
            if(! inTable){
                free(problem->terms[i]);
            }
        }
        if(problem->terms){
            free(problem->terms);
        }

        for(int i = 0; i < problem->termColourTableCount; i++){
            free(problem->colourTables[i].term);
            if(problem->colourTables[i].colours){
                free(problem->colourTables[i].colours);
                free(problem->colourTables[i].scores);
            }
        }
        if(problem->colourTables){
            free(problem->colourTables);
        }
        if(problem->colourTransitionTable){
            free(problem->colourTransitionTable->prevColours);
            free(problem->colourTransitionTable->colours);
            free(problem->colourTransitionTable->scores);
            free(problem->colourTransitionTable);
        }
        if(problem->text){
            free(problem->text);
        }
        free(problem);
    }
}

/* Sets up a solution for the given problem */
struct solution *newSolution(struct problem *problem){
    struct solution *s = (struct solution *) malloc(sizeof(struct solution));
    assert(s);
    s->termCount = problem->termCount;
    s->termColours = (int *) malloc(sizeof(int) * s->termCount);
    assert(s->termColours);
    for(int i = 0; i < s->termCount; i++){
        s->termColours[i] = DEFAULTCOLOUR;
    }
    s->score = DEFAULTSCORE;
    return s;
}


/*
    Solves the given problem according to Part A's definition
    and places the solution output into a returned solution value.
*/
struct solution *solveProblemA(struct problem *p) {
    struct solution *s = newSolution(p);
    /* Fill in: Part A */

    /* Loop through every term */
    for (int i = 0; i < p->termCount; i++) {
        /* Initialise the maximum colour and score of the term*/
        int maxColour = 0;
        int maxScore = 0;
        /* Loop through every table*/
        for (int j = 0; j < p->termColourTableCount; j++) {
            /* Check if this term is the same as the current term in the table */
            if (strcmp(p->terms[i], p->colourTables[j].term) == 0) {
                /* Loop through every attribute of the term in the color table */
                for (int k = 0; k < p->colourTables[j].colourCount; k++) {
                    /* Find the color and score of the term */
                    int colour = p->colourTables[j].colours[k];
                    int score = p->colourTables[j].scores[k];

                    /* Compare current score and maximum score */
                    if (score > maxScore) {
                        maxScore = score;
                        maxColour = colour;
                    }
                }
            }
        }
        /* Store the color with highest score into the solution */
        s->termColours[i] = maxColour;
        /* Update the total score of the solution */
        s->score += maxScore;
    }
    return s;
}


struct solution *solveProblemB(struct problem *p) {
    struct solution *s = newSolution(p);
    /* Fill in: Part B */

    /* Initialise the previous colour */
    int prevColour = -1;
    /* Loop through every term */
    for (int i = 0; i < p->termCount; i++) {
        /* Initialise the maximum colour and score of the term*/
        int maxColour = 0;
        int maxScore = 0;
        /* Loop through every table */
        for (int j = 0; j < p->termColourTableCount; j++) {
            /* Check if this term is the same as the current term in the table */
            if (strcmp(p->terms[i], p->colourTables[j].term) == 0) {
                /* Loop through every attribute of the term in the color table */
                for (int k = 0; k < p->colourTables[j].colourCount; k++) {
                    /* Find the color of the term */
                    int colour = p->colourTables[j].colours[k];
                    /* Initialize the score of the term */
                    int score = 0;
                    score += p->colourTables[j].scores[k];
                    /* Update the score with the score of transition table */
                    for (int t = 0; t < p->colourTransitionTable->transitionCount; t++) {
                        if (prevColour != -1 &&
                            prevColour == p->colourTransitionTable->prevColours[t] &&
                            colour == p->colourTransitionTable->colours[t]) {
                            score += p->colourTransitionTable->scores[t];
                        }
                    }
                    /* Update the best colour and its score */
                    if (score > maxScore) {
                        maxScore = score;
                        maxColour = colour;
                    }
                }
            }
        }
        /* Store the color with the highest score into the solution */
        s->termColours[i] = maxColour;
        /* Update the total score of the solution */
        s->score += maxScore;
        /* Store the previous color with the highest score into the solution */
        prevColour = maxColour;
    }
    return s;
}



struct solution *solveProblemE(struct problem *p) {
    struct solution *s = newSolution(p);
    /* Fill in: Part E */

    /* Create a DP array to store the scores */
    int **DP = createDPArray(p);

    /* Initialize the maximum colour and score */
    int maxColour;
    int maxScore;
    /* Initialize the previous colour */
    int prevColour = -1;

    /* Compute the first term according to the word table */
    computeFirstTerm(p, DP, &maxColour, &maxScore, &prevColour);

    /* Loop through each term starting from the second term */
    for (int i = 1; i < p->termCount; i++) {
        maxColour = 0;
        maxScore = 0;
        /* Check if the current term matches any term in the color table */
        struct termColourTable* currentTable = findTermTable(p, i);
        if (currentTable == NULL) {
            int maxNoTable = 0;
            /* If no colour table is found for the current term */
            for (int c = 0; c < p->colourTables[i - 1].colourCount; c++) {
                int ctScore0 = getTransitionScore(p, c, 0);
                int scoreNoTable = DP[i - 1][c] + ctScore0;
                /* Find the best score based on the colours of previous term */
                if(scoreNoTable > maxNoTable){
                    maxNoTable = scoreNoTable;
                }
            }
            /* Update the total score */
            DP[i][0] = maxNoTable; 
        } else {
            /* Loop through each color in the color table */
            for (int k = 0; k < currentTable->colourCount; k++) {
                int colour = currentTable->colours[k];

                /* Option 1: Calculate score using the previous color */
                int ctScore1 = getTransitionScore(p, prevColour, colour);
                int option1 = DP[i - 1][prevColour] + currentTable->scores[k] + ctScore1;

                /* Option 2: Calculate score considering other previous colors */
                int maxOption2 = 0;
                struct termColourTable* prevTable = findTermTable(p, i - 1);
                for (int prevColour2 = 0; prevColour2 < prevTable->colourCount; prevColour2++) {
                    /* Avoid checking the previous color */
                    if (prevColour2 != prevColour) {

                        int ctScore2 = getTransitionScore(p, prevColour2, colour);
                        /* Update the total scores received by option2 method */
                        int option2 = DP[i - 1][prevColour2] + currentTable->scores[k] + ctScore2;
                        /* Update the best ct score and best prevColour */
                        if (option2 > maxOption2) {
                            maxOption2 = option2;
                        }
                    }
                }
                /* Choose the maximum score between option 1 and option 2 */
                DP[i][colour] = (option1 > maxOption2) ? option1 : maxOption2;

                /* Update the best score and best color */
                if (DP[i][colour] > maxScore) {
                    maxScore = DP[i][colour];
                    maxColour = colour;
                }
            }
            /* Update the previous color with the highest score */
            prevColour = maxColour;
        }
    }
    /* Store the final calculated answer into the solution */
    s->score = DP[p->termCount - 1][prevColour];

    return s;
}



struct solution *solveProblemF(struct problem *p){
    struct solution *s = newSolution(p);
    /* Fill in: Part F */

    /* Create a DP array to store the scores */
    int **DP = createDPArray(p);
    /* Create a DP array to store the best previous colours */
    int **maxPrevColours = createDPArray(p);

    /* Initialize the maximum colour and score */
    int maxColour;
    int maxScore;
    /* Initialize the previous colour */
    int prevColour = -1;

    /* Compute the first term according to the word table */
    computeFirstTerm(p, DP, &maxColour, &maxScore, &prevColour);

    /* Loop through each term starting from the second term */
    for (int i = 1; i < p->termCount; i++) {
        maxColour = 0;
        maxScore = 0;
        /* Check if the current term matches any term in the color table */
        struct termColourTable* currentTable = findTermTable(p, i);
        if (currentTable == NULL) {
            int maxNoTable = 0;
            int maxPrevColour = 0;
            /* If no colour table is found for the current term */
            for (int c = 0; c < p->colourTables[i - 1].colourCount; c++) {
                int ctScore0 = getTransitionScore(p, c, 0);
                int scoreNoTable = DP[i - 1][c] + ctScore0;
                /* Find the best score based on the colours of previous term */
                if(scoreNoTable > maxNoTable){
                    maxNoTable = scoreNoTable;
                    maxPrevColour = c;
                }
            }
            /* Update the previous color and total score */
            DP[i][0] = maxNoTable;
            maxPrevColours[i][0] = maxPrevColour; 
        }else {
            /* Loop through each color in the color table */
            for (int k = 0; k < currentTable->colourCount; k++) {
                int colour = currentTable->colours[k];

                /* Option 1: Calculate score using the previous color */
                int ctScore1 = getTransitionScore(p, prevColour, colour);
                int option1 = DP[i - 1][prevColour] + currentTable->scores[k] + ctScore1;

                /* Option 2: Calculate score considering other previous colors */
                int maxOption2 = 0;
                int maxPrevColour = 0;
                struct termColourTable* prevTable = findTermTable(p, i - 1);
                for (int prevColour2 = 0; prevColour2 < prevTable->colourCount; prevColour2++) {
                    /* Avoid checking the previous color */
                    if (prevColour2 != prevColour) {

                        int ctScore2 = getTransitionScore(p, prevColour2, colour);
                        /* Update the total scores received by option2 method */
                        int option2 = DP[i - 1][prevColour2] + currentTable->scores[k] + ctScore2;
                        /* Update the best ct score and best prevColour */
                        if (option2 > maxOption2) {
                            maxOption2 = option2;
                            maxPrevColour = prevColour2;
                        }
                    }
                }
                /* Choose the maximum score between option 1 and option 2 */
                DP[i][colour] = (option1 > maxOption2) ? option1 : maxOption2;
                /* Choose the maximum previous colour between option 1 and option 2 */
                maxPrevColours[i][colour] = (option1 > maxOption2) ? prevColour : maxPrevColour;

                /* Update the best score and best color */
                if (DP[i][colour] > maxScore) {
                    maxScore = DP[i][colour];
                    maxColour = colour;
                }
            }
            /* Update the previous color with the highest score */
            prevColour = maxColour;
        }
    }
    /* Store the final calculated answer into the solution */
    s->score = DP[p->termCount - 1][prevColour];

    /* Loop every sequence from the last term */
    for(int m = p->termCount - 1; m >= 0; m--){
        /* get the colour of the term that matches with prevColour */
        s->termColours[m] = prevColour;
        /* Find the colour of the previous term */
        prevColour = maxPrevColours[m][prevColour];
    }

    return s;
}



/*
    Helper function to create the DP array.
*/
int **createDPArray(struct problem *p) {
    int **DP = malloc(sizeof(int *) * p->termCount);
    for (int i = 0; i < p->termCount; i++) {
        DP[i] = malloc(sizeof(int) * COLOURCASES);
        for (int j = 0; j < COLOURCASES; j++) {
            DP[i][j] = 0;
        }
    }
    return DP;
}


/*
    Helper function to compute for the first term.
*/
void computeFirstTerm(struct problem *p, int **DP, int *maxColour, int *maxScore, int *prevColour) {
    /* Loop through every table */
    for (int j = 0; j < p->termColourTableCount; j++) {
        *maxColour = *maxScore = 0;
        /* Check if this term is the same as the current term in the table */
        if (strcmp(p->terms[0], p->colourTables[j].term) == 0) {
            for (int k = 0; k < p->colourTables[j].colourCount; k++) {
                /* Check every colour */
                int colour = p->colourTables[j].colours[k];
                /* Find the wc score of this colour */
                int score = p->colourTables[j].scores[k];
                /* Store the score within the DP array */
                DP[0][colour] = score;
                /* Update the best colour and its score */
                if (score > *maxScore) {
                    *maxScore = score;
                    *maxColour = colour;
                }
            }
        }
    }
    /* Update the previous colour as the colour with maximum score */
    *prevColour = *maxColour;
}


/*
    Helper function to find the score from transition colour table.
*/
int getTransitionScore(struct problem *p, int prevColour, int currentColour) {
    /* Loop through the transition table of the problem */
    for (int i = 0; i < p->colourTransitionTable->transitionCount; i++) {
        /* Check if the previous color and current color match */
        if (p->colourTransitionTable->prevColours[i] == prevColour &&
            p->colourTransitionTable->colours[i] == currentColour) {
            /* Return the corresponding score */
            return p->colourTransitionTable->scores[i];
        }
    }
    /* Return 0 if no matching transition is found */
    return 0;
}


/*
     Helper function to check if there is colour table for the term.
*/
struct termColourTable* findTermTable(struct problem *p, int termIndex) {
    /* Initialise the term colour table */
    struct termColourTable* table = NULL;
    /* Check whether the term has a colour table */
    for (int i = 0; i < p->termColourTableCount; i++) {
        if (strcmp(p->terms[termIndex], p->colourTables[i].term) == 0) {
            /* Set the table variable to point to the current colour table */
            table = p->colourTables + i;
            break;
        }
    }
    return table;
}


