//Classic Snake clone by Tim Mele and Zachary Bearse
#include <curses.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

//function declarations (Tim/Zach)
void moveCursor(int, int);
void initialize();
void moveSnake();
void on_input();
void enable_kbd_signals();
int setTicker(int);
void printSnake();
void endGame();
void spawnTrophy();
int collosionCheck(int, int);

//Snake start size and amount of memory to allocate for snakeQueue(Tim)
#define SIZE 1000      
#define STARTSIZE 3

//snakeQueue declaration (Tim)
struct snakeQueue* snake;

//starting coordinates and variables to store the current coordinates (Tim)
int prevX;
int prevY;
//snake movement direction(Zach)
int dir;
//varriables for snake(Tim)
int grow; //used to delay pop function in moveSnake()
int maxLength; //when snake length reaches maxLength game will end.

//trophy position (Tim)
int trophyX;
int trophyY;

//trophy timer variables (Zach)
int trophyTTS; //ticks to spawn
int trophyTTG; //ticks to go

//coordinate structure declaration (Tim)
struct coordinate{
    int xCord; 
    int yCord; 
};

//queue structure declaration (Tim)
struct snakeQueue{
    struct coordinate snakeCoords[SIZE]; 
    int front;
    int rear;
    //Timing variables for movement (Zach)
    int ttm;    //ticks to move - movement interval
    int ttg;    // ticks to go
};

//creates and sets up a new snakeQueue (Tim)
struct snakeQueue* createQueue() {
    struct snakeQueue* q = malloc(sizeof *q);
    q->front = 0;
    q->rear = 0;
    q->ttm = 100;
    q->ttg = 100;
    return q;
}

//check if queue is full (Tim)
int full(struct snakeQueue* q){
  return (((q->rear + 1) % maxLength) == q->front); 
}
//checks  if queue is empty (Borrowed)(Tim)
int empty(struct snakeQueue* q){
  return (q->rear == q->front);
}

//Add a new coordinate to head of snake (Tim)
void push(struct snakeQueue* q, int xCord, int yCord){
    if (full(q)){
        setTicker(0);           //disable alarm
        signal(SIGIO, SIG_IGN); //ignore further input
        mvaddstr(LINES /2 , (COLS / 2) - 4, "You Win!");
        refresh();
        sleep(3);
        endwin();
        raise(SIGINT);
        
    }
    q->snakeCoords[q->rear].xCord = xCord;
    q->snakeCoords[q->rear].yCord = yCord;
    q->rear = (q->rear + 1) % maxLength;
}

//returns the coordinates of the tail and removes it from the queue (Tim)
struct coordinate pop(struct snakeQueue* q){
    struct coordinate res;
    if (empty(q)){
        printf("Queue is empty");
    }
    else{
        res = q->snakeCoords[q->front];
        q->front = (q->front + 1) % maxLength;
    }
    return res;
}

//Check for and handle various collisions (Tim)
int collosionCheck(int x, int y){
    //Check for collision with walls
    if (x >= COLS -1 || x <= 0 || y >= LINES -1 || y <=0)
        endGame();

    char ch = mvinch(y,x) & A_CHARTEXT;
    if (ch!= ' '){
        //Check for collisions with snake body
        if(ch == 'S'){
            endGame();
        }
        int chInt = (int)ch - '0'; //Turn ch into int
        grow += chInt;             //add trophy value to grow
        spawnTrophy();             //spawn another trophy
    }
    return 0;
}

//moves the snake, responds to alarm signal (Tim)(Zach)
void moveSnake(){
    struct coordinate erase;
    
    if (dir == 1){
        prevX ++;
        collosionCheck(prevX, prevY);
        push(snake, prevX, prevY);
        if(grow > 0){
            grow --;
            snake->ttm -= 2; //increase speed
        }else {
            erase = pop(snake);
            mvwaddch(stdscr, erase.yCord, erase.xCord, ' ');
        }
        wrefresh(stdscr);
    }
    else if (dir == 2){
        prevY ++;
        collosionCheck(prevX, prevY);
        push(snake, prevX, prevY);
        if(grow > 0){
            grow --;
            snake->ttm -= 2;
        }else {
            erase = pop(snake);
            mvwaddch(stdscr, erase.yCord, erase.xCord, ' ');
        }
        wrefresh(stdscr);
    }
    else if (dir == 3){
        prevX --;
        collosionCheck(prevX, prevY);
        push(snake, prevX, prevY);
        if(grow > 0){
            grow --;
            snake->ttm -=2;
        }else {
            erase = pop(snake);
            mvwaddch(stdscr, erase.yCord, erase.xCord, ' ');
        }
        wrefresh(stdscr);
    }
    else if (dir == 4){
        prevY --;
        collosionCheck(prevX, prevY);   
        push(snake, prevX, prevY);
        if(grow > 0){
            grow --;
            snake->ttm -=2;
        }else {
            erase = pop(snake);
            mvwaddch(stdscr, erase.yCord, erase.xCord, ' ');
        }
        wrefresh(stdscr);
    }
    printSnake(snake);
}

//prints the entire snake from snakeQueue (Tim)
void printSnake(struct snakeQueue* q){
    int tmp = q->front;

    while(tmp != q->rear)
    {
        int x = q->snakeCoords[tmp].xCord;
        int y = q->snakeCoords[tmp].yCord;
        moveCursor(x, y);
        waddch(stdscr, 'S');

        tmp = (tmp + 1) % maxLength;
    }
    wrefresh(stdscr); 
}

//Tells kernel to send signals on keyboard input (Zach)
//from Molay
void enable_kbd_signals(){
    int fd_flags;
    fcntl(0, F_SETOWN, getpid());
    fd_flags = fcntl(0, F_GETFL);
    fcntl(0, F_SETFL, (fd_flags|O_ASYNC));
}

/* 
changes snake direction on player input (Zach)
directions represented as numbers starting with right = 1
and rotating clockwise.
dir = 1 -> right, 
dir = 2 -> down,
dir = 3 -> left,
dir = 4 -> up
*/
void on_input(){
    int c = getch();
    if(c == KEY_RIGHT){
        if(dir == 3){
            endGame(); // if player tries to reverse direction, end game
        }
        else{
            dir = 1;
        }
    }
    else if (c == KEY_DOWN){
        if(dir == 4){
            endGame();
        }
        else{
            dir = 2;
        }
    }
    else if (c == KEY_LEFT){
        if(dir == 1){
            endGame();
        }
        else{
            dir = 3;
        }
    }
    else if (c == KEY_UP){
        if(dir == 2){
            endGame();
        }
        else{
            dir = 4;
        }
    }
}

//Spawns a trophy at a random location (Tim)
void spawnTrophy(){
    trophyY = (rand()  % LINES-1) +1;
    trophyX = (rand()  % COLS-1) +1;
    char ch = mvinch(trophyY,trophyX) & A_CHARTEXT;
    if (ch== ' '){ //If blank space spawn trophy. Else make recursive call to try again.
        int trophy = (rand()  % 9) +1;
        char trophyChar;
        switch (trophy) {
            case 1:
                trophyChar = '1';
                break;
            case 2:
                trophyChar = '2';
                break;
            case 3:
                trophyChar = '3';
                break;
            case 4:
                trophyChar = '4';
                break;
            case 5:
                trophyChar = '5';
                break;
            case 6:
                trophyChar = '6';
                break;
            case 7:
                trophyChar = '7';
                break;
            case 8:
                trophyChar = '8';
                break;
            case 9:
                trophyChar = '9';
                break;
        }
        // timing (Zach)
        trophyTTS = (1000*(rand()%9)+1);
        trophyTTG = trophyTTS;
        mvwaddch(stdscr, trophyY, trophyX, trophyChar);
        refresh();
    }else {
        spawnTrophy();
    }
}

//set up timer(Zach)
//from Bruce Molay - Understanding Unix/Linux Programming
int setTicker(int n_msecs){
    struct itimerval new_timeset;
    long n_sec, n_usecs;

    n_sec = n_msecs / 1000;
    n_usecs = (n_msecs % 1000) * 1000L;

    new_timeset.it_interval.tv_sec = n_sec;
    new_timeset.it_interval.tv_usec = n_usecs;
    new_timeset.it_value.tv_sec = n_sec;
    new_timeset.it_value.tv_usec = n_usecs;

    return setitimer(ITIMER_REAL, &new_timeset, NULL);
}
//handles snake and trophy timers (Zach)
void timeHandler(){
    if(trophyTTG-- == 1){ //decrement and check
        mvwaddch(stdscr, trophyY, trophyX, ' ');
        refresh();
        spawnTrophy();    //spawnTrophy handles new timer
    }

    if(snake->ttg-- == 1){//decrement counter and check
        moveSnake();
        snake->ttg = snake->ttm; //reset ttg
    }
}

//move cursor to designated x and y coordinate (Tim)
void moveCursor(int x, int y){
    wmove(stdscr, y, x);
    wrefresh(stdscr);
}

//Ends the game (Tim) 
void endGame(){
    setTicker(0);           //disable alarm
    signal(SIGIO, SIG_IGN); //ignore further input
    mvaddstr(LINES /2 , (COLS / 2) - 5, "GAME OVER!");
    refresh();
    sleep(3);
    endwin();
    raise(SIGINT);
}

//set curses settings, add initial length to snake. (Tim/Zach)
void initialize(){
    initscr(); //Initialize the screen
    clear();
    noecho(); //No automatic Echo
    curs_set(0);
    cbreak(); //No buffering
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    //Set starting coordinates to center of current window (Tim)
    prevY = LINES / 2;
    prevX = COLS / 2;
    
    //input handling (Zach)
    signal(SIGIO, on_input);
    enable_kbd_signals();

    //Initialize snake coordinate queue (Tim)
    maxLength = LINES + COLS;
    snake = createQueue();

    //Get random number between 1 and 4 to randomly select starting direction for snake (Tim)
    srand(time(NULL));   
    dir = (rand() % 4) + 1; 
    
    //add length to snake = to STARTSIZE (Tim)
    if(dir == 3){ //reverse snake if initial direction is left
        for (int i =0; i<STARTSIZE; i++){
            push(snake, prevX--, prevY);
        }   
    }else{
        for (int i =0; i<STARTSIZE; i++){
            push(snake, prevX++, prevY);
        }
    }
    //spawn first trophy
    wborder(stdscr, 0, 0, 0, 0, 0, 0, 0, 0);  //Draw the border
    spawnTrophy();
    wrefresh(stdscr);
    printSnake(snake);
}

//main method, starts game
int main(){
    initialize();
    // connect alarm signal to timeHandler (Zach)
    signal(SIGALRM, timeHandler);
    //schedule alarm signal for every 1ms == 0.001 seconds (Zach)
    if(setTicker(1) == -1){
        perror("setTicker");
    }
    else {
        while (1) {
            pause();
        }
    }
endwin();
}