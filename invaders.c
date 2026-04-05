// invaders.c
//
// left = left arrow, right = right arrow, fire = space
//
// gcc -o invaders invaders.c -lncurses

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>

// defines for the left, right, and fire keys
// see curses.h for KEY_XXX definitions
//
// move left = left arrow key
#define LEFT 	KEY_LEFT
// move right = right arrow key 
#define RIGHT KEY_RIGHT
// fire = space bar
#define FIRE 	' '

// after each display round, we call usleep to pause
// by this many nanoseconds - if we don't the game is
// over in about a second - you may need to tune this parameter
// to your system
#define DELAY 65000

// how many space invaders (SI)?
#define NSIR 4       // number of rows of SI 
#define NSIC 8      // number of columns of SI 

// display size
#define NLS  16      // number of lines on display (need > NSIR+1)
#define NCS  20      // width of display (need > NSIC+1)

// how many timesteps of the game we've gone through
int ifr;
// your position (column number) - initially the middle
int y=NCS/2;
	
int lc=7;
int lr=-1;
int bc=NSIC/2;
int br=0;
int nbc=NSIC/2;
int nbr=0;
int sr=0;
int sc=0;
// number of space invaders remaining
int nsi;
int ir,ic,ib,irow;
int ichar;
int si[NSIR][NSIC];      // store of SI (1 if on, 0 if dead)
char dispf[NLS][NCS+1];  // display buffer

// keep track of how long the player survives
time_t start_time, end_time;

//////////////////////////////////////////////////////////////////////
// initialise screen (uses ncurses)
//////////////////////////////////////////////////////////////////////
void init_screen()
{
	// initialise ncurses
	if(initscr() == NULL ) {
		perror( "error initialising ncurses" );
		exit(EXIT_FAILURE);
	}
	nodelay(stdscr,TRUE);
	raw();	
	keypad(stdscr, TRUE);		
	// don't echo character press to screen 
	noecho( );
	// hide the cursor
	curs_set(0);
	start_color();			/* Start color 			*/

	init_pair(1, COLOR_RED, COLOR_BLACK); // for explosion
	init_pair(2, COLOR_YELLOW, COLOR_BLACK); // for missiles+bombs
	init_pair(3, COLOR_WHITE, COLOR_BLACK); // default
	init_pair(4, COLOR_GREEN, COLOR_BLACK); // invaders
	init_pair(5, COLOR_BLUE, COLOR_BLACK); // you
}

void press_key_to_continue()
{
	attron(A_BOLD);
	attron(A_BLINK);
	
	printw("\n\r        * PRESS ANY KEY TO CONTINUE *");

	attroff(A_BOLD);
	attroff(A_BLINK);
	wrefresh(stdscr);
	while(getch() == ERR)
		{
			usleep(DELAY);
		}
}

void quit()
{ 
	press_key_to_continue();
	clear();
	// get curses to clean up
	endwin();
	// and quit
	exit(1);
}

void welcome_screen()
{
	clear();
	printw("\n\n\n\n\n");
	attron(A_BOLD);
	attron(COLOR_PAIR(1));
	printw("          S P A C E  I N V A D E R S\n\r");
	attroff(COLOR_PAIR(1));	
	attroff(A_BOLD);
	printw("              (on your terminal)\n\r\n\n\n");
	printw("           Left arrow  = move left\n\r");
	printw("           Right arrow = move right\n\r");
	printw("           Space bar   = FIRE!\n\r\n\n\n");
	press_key_to_continue();
	clear();
}

void update_display()
{
			clear();

			printw(" ");
			for (ic=0; ic<(NCS+2); ++ic){
					printw("_");
			}
			for (ir=0; ir<NLS; ++ir){
				printw("\n\r |");
				for (ic=0; ic<NCS; ++ic){
					int output_color;
					switch(dispf[ir][ic]) 
						{
						case 'Y' : // you
							output_color = 5;
							break;
						case 'H' : // space invader
							output_color = 4;
							break;
						case '.' : //background
							output_color = 3;
							break;
						case '*' :
						case '^' :
							output_color = 2;
							break;
						case 'W' :
							output_color = 1;
							break;
						 }
					attron(COLOR_PAIR(output_color));
					printw("%c",dispf[ir][ic]);
					attroff(COLOR_PAIR(output_color));
				}
				
				printw("|");
			}
			printw("\n\n\r Score: %d\n\r",(NSIR * NSIC) - nsi);
			// you have won if the number of space invaders remaining is 0
			if (nsi == 0) {
				time(&end_time);
				printw("G A M E  O V E R  .... enemy destroyed after %d seconds\n\r",
										 	(int)difftime(end_time, start_time));
				refresh();
				usleep(3000000);
				quit();
			}
			refresh();
}

//////////////////////////////////////////////////////////////////////
// MAIN
//////////////////////////////////////////////////////////////////////
int main()
{
	
	init_screen(); // setup curses

	welcome_screen(); // show welcome screen
	
	// SI initialise
	nsi=0;
	for(ir=0; ir<NSIR; ++ir)  
		{
			for (ic=0; ic<NSIC; ++ic){
				si[ir][ic]=1;
				++nsi;
			}
			;
		} 

	// get the current time into variable start_time
	time(&start_time);

  //////////
	// big loop to run game
	//////////
	
	for (ifr=1;ifr<999999;++ifr)
    {
			
			// initialise display buffer - all empty initially
			for(ir=0; ir<NLS; ++ir)
				{
					for (ic=0; ic<NCS; ++ic){
						dispf[ir][ic]='.';
					} 
					dispf[ir][NCS]='\0';
				} 

			// evaluate new bomb position: from top NSIR rows of SI (if above bottom)

			if ( sr+NSIR > NLS-2) {irow= NLS-1-sr;} else {irow=NSIR;}
			ir= y+lr+ifr;                   // quasi-random bomb position
      nbr=sr;nbc=sc+NSIC/2;          // default (should not be needed)
			for (ic=0; ic < (irow*NSIC-1); ++ic)  {
				ib= (ir+ic) % (irow*NSIC) ;    // cycle round irow rows - take first
				if ( *(&si[0][0]+ib) == 1 ) 
					{nbc = (ib % NSIC)+sc; nbr= (ib/NSIC) +sr; break; }
      }


			//----either move SI or move lby  & 31 is 1/32   & 15 is 1/16
      if( (ifr & 15 ) != 0 ) {

				// ------ move lby-------------------------------------------

				//  a key press: see ncurses
				ichar = getch( ); 

				// move y (You)
        if (ichar==LEFT && y > 0){y=y-1;}    //  "1"
        if (ichar==RIGHT && y < NCS-1){y=y+1;}   //  "2"
				dispf[NLS-1][y]='Y';

				// si at y? (should not occur)
        if ( y >= sc && y-sc <= NSIC-1 && NLS-sr <= NSIR ) {
					if ( si[NLS-sr-1][y-sc] == 1 ){ 
						printw("\n si lands on you \n\r"); refresh(); quit();}
        }

				//move b:  bomb    or new bomb if at bottom
        if (br > NLS-2 )
					{
							br=nbr;
							bc=nbc;
					}
        else
					{
						br=br+1;
						dispf[br][bc]='*';
					}
				// has a bomb landed on you...?
        if( (br == NLS-1) && (bc == y) ) {
					// yes - you've been hit by a bomb
						time(&end_time);
						if((int)difftime(end_time, start_time) < 2)
							printw("G A M E  O V E R  .... destroyed before you even started!\n\r");
						else
							printw("G A M E  O V E R  .... destroyed after %d seconds\n\r",
										 	(int)difftime(end_time, start_time));
						refresh();
						usleep(3000000);
						quit();
				}

				// move l (Your laser)
        if (ichar==FIRE && lr < 0 ){lc=y; lr=NLS-1; }     // fire 
        if(lr > -1)
					{
					lr=lr-1;                          // l moves up
					if( lr >= 0) { dispf[lr][lc]='^';}
					}
				

				// si at l ?   and set up display of SI
				for(ir=0; ir<NSIR; ++ir)
					{ for (ic=0; ic<NSIC; ++ic){
							if( si[ir][ic]==1 ) {                  
								if( lr == ir+sr && lc == ic+sc) 
									{si[ir][ic]=0; lr=-1; --nsi; dispf[ir+sr][ic+sc]='W';} 
								else { dispf[ir+sr][ic+sc]='H' ;}   ;}
							; }; } 

				;}
			else  {
				// move SI --------------------------------------------------------
				if ( (sr & 1 ) == 1){
					//                                          odd row 1 3
					if( sc > 0 ) {sc=sc-1; } else {sr=sr+1;} 
					;}
				else {
					//                                         even row 0 2
					if( sc + NSIC < NCS ) {sc=sc+1; } else {sr=sr+1;}
					;}
      
				for(ir=0; ir<NSIR; ++ir)
					{ for (ic=0; ic<NSIC; ++ic){
							if( si[ir][ic]==1 ) {           // test SI reach bottom line
								if( ir+sr == NLS-1 ) {
										printw("\n\r SI on bottom \n\r");
										refresh();
										quit();
								}
								dispf[ir+sr][ic+sc]='H'
									;} ; }; } 

				;}	

			// update the display

			update_display();

			// introduce a delay: if we don't, then game is over in < a second
			usleep(DELAY);			
    }
		quit();
}

// END
