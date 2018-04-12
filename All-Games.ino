#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

// Attiny85:
#define LEDPIN 1
#define LBUTPIN 0
#define RBUTPIN 2
#define NUMPIXELS 15

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

// Global variables
int brightness = 10;
int level = 0;
int lives = 3;
int pause = 20;
int game = 99;
int gametotal = 2; // Number of games on the system. 0 based. Max is 4 (5 games)
int starter = 0;
int currow = 0;
int curcol = 0;
int resetdelay = 500;
byte reset = 0;

// Global button variables
byte newleftclickholder = 0; // is 1 when a new left click is recorded
byte newrightclickholder = 0; // is 1 when a new right click is recorded
byte newbothclickholder = 0; // is 1 when a new both click is recorded
byte buttonstate = B00000000; // state of buttons during last read. 00 is both off, 01 is right button, 10 is left button, 11 is both buttons
int longpresscounter = 0;  // Keeps track of both buttons held down

// PR variables
int masterspeeddelay = 300; // Speed for Attiny
int speeddelay = 300;
int PRcollision = -1; // -1 indicates no collisions. 0,1,2 indicates collision in that column.
int PRpasscount = 35;
int PRpassed = 0;
int score;

// PI variables
int enemies = 0;
int counter = 0; // A way to slow down the enemy laser fire going down the screen

// PP vars
byte colorcarry = 'w'; // color of the moving cursor
byte colorcarrytemp;
byte randColor;
int colorcount = 0;
char *colors ="crogl"; // Charater array of the color codes
int columns[] = {0, 2};
int pixshift = 2;
int loopspeed = 2000;

// Initialize playing field
byte board[5][3] = {
  {'r', 'r', 'r'},
  {'n', 'n', 'n'},
  {'n', 'n', 'n'},
  {'n', 'n', 'n'},
  {'n', 'n', 'n'},
};

// Setup a second playing field for the PP game to store the pattern in
byte pattern[5][3] = {
  {'n', 'n', 'n'},
  {'n', 'n', 'n'},
  {'n', 'n', 'n'},
  {'n', 'n', 'n'},
  {'n', 'n', 'n'},
};

/* ------------------------------------------------- */
void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  pinMode(LBUTPIN,INPUT_PULLUP); // Left Button on PO
  pinMode(RBUTPIN, INPUT_PULLUP); // Right button on P2

  strip.begin();
  strip.setBrightness(brightness);
  strip.show(); // Initialize all pixels to 'off'
  updatepixels();
}
/* ------------------------------------------------- */
/*   These are general functions used by all games   */
/* ------------------------------------------------- */
void gameselect()
{
  game = 0;
  starter = 0;
  while(1 < 2)
  {
    if (newbothclick())
    { 
      return;
    }
    if (game > gametotal) // Fail safe
    {
      game = 0;
    }
    if (newrightclick() and game < gametotal) // move down the list of games
    {
      game += 1;
      for (int col = 0; col < 3; col++)
      {
        board[game][col] = 'r';
        board[game - 1][col] = 'r';
      }
    }
    if (newleftclick() and game > 0)  // move up the list of games
    {
      game -= 1;
      for (int col = 0; col < 3; col++)
      {
        board[game + 1][col] = 'n';
      }
    }
    updatepixels();
    strip.show();

    // Clear the button queues before moving on
    newrightclickholder = 0;
    newleftclickholder = 0;
    newbothclickholder = 0;
    longpresscounter = 0;
    
    checkbuttons();
  }
}
/* ------------------------------------------------- */
void startpixracer()
{
  for (int row = 0; row < 5; row++)
  {
    board[row][0] = 'o';
    board[row][1] = 'n';
    board[row][2] = 'o';
  }
  board[4][0] = 'n';
  board[4][1] = 'c';
  board[4][2] = 'n';
  
  updatepixels();
  strip.show();
  starter += 1;
  resetdelay = 900;
}
/* ------------------------------------------------- */
void startpixinvaders()
{
  for (int row = 0; row < 2; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      board[row][col] = 'r';
    }
  }
  for (int row = 2; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      board[row][col] = 'n';
    }
  }
  board[4][1] = 'c';
  updatepixels();
  strip.show();
  delay(500); // Fix for the PP bug
  starter += 1;
  resetdelay = 40;
}
/* ------------------------------------------------- */
void startpixpattern()
{
  PP_newlevel();
  updatepixels();
  strip.show();
  starter += 1;
  resetdelay = 9999;
  loopspeed = 2000;
}
/* ------------------------------------------------- */
void levelcleared()
{
  // Show that the level has been cleared by flashing the screen
  int brightnesshigh = brightness + 30;
  for (int x = 0; x < 6; x++)
  {
    strip.setBrightness(brightnesshigh);
    strip.show();
    delay(100);
    strip.setBrightness(brightness);
    strip.show();
    delay(100);
  }
}
/* ------------------------------------------------- */
void boardclear()
{
    for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      board[row][col] = 'n';
    }
  }
  updatepixels();
}
/* ------------------------------------------------- */
void gameover()
{
  game = 99; // Need to set this over here so that when updatepixels is called it shows as red for the PP game. (giggle)
  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      board[row][col] = 'r';
    }
    updatepixels();
    delay(250);
  }
  delay(1000);
  level = 0;
  lives = 3;
  reset = 0;

  board[0][0] = 'r';
  board[0][1] = 'r';
  board[0][2] = 'r';
  
  for (int row = 1; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      board[row][col] = 'n';
    }
  }
  // Clear the button queues before moving on
  newrightclickholder = 0;
  newleftclickholder = 0;
  newbothclickholder = 0;
  longpresscounter = 0;
  
  gameselect();
}
/* ------------------------------------------------- */
void levelcheck(int row, int col)
{
  // Check if all the bad guys are cleared from the board
  enemies = 0;
  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      if (board[row][col] == 'r' or board[row][col] == 'o')
      {
        enemies = enemies + 1;
      }
    }
  }
  if (enemies == 0)
  {
    // Reset board if all the bad guys are gone
    delay(500);
    for (int row = 1; row > -1; row--)
    {
      for (int col = 0; col < 3; col++)
      {
        board[row][col] = 'r';
      }
    }
    level += 1;
    if (level > 3)
    {
      board[0][0] = 'o';
      board[1][1] = 'o';
      board[0][2] = 'o';
    }
    if (level > 6)
    {
      board[1][0] = 'o';
      board[0][1] = 'o';
      board[1][2] = 'o';
    }
    else
    {
      for (int levels = 0; levels < level; levels++)
      {
        int randNumber = random(0,7);
        if (randNumber < 3)
        {
          board[0][randNumber] = 'o';
        }
        if (randNumber > 3)
        {
          board[1][randNumber - 4] = 'o';
        }
      }
    }
  }
}
/* ------------------------------------------------- */
void PI_enemyfire()
{
  // Enemies can fire after the intro level
  if (level > 0)
  {
    int randNumber = random(0,1000);
    if (randNumber < 150) // Thinking of making this a var so the firing gets more as the levels go up.
    {
      int randNumber = random(0,3);
      if (board[1][randNumber] == 'r' or board[1][randNumber] == 'o')
      {
        board[2][randNumber] = 'g';
      }
      else if (board[0][randNumber] == 'r' or board[0][randNumber] == 'o')
      {
        board[1][randNumber] = 'g';
      }
    }
  }
}
/* ------------------------------------------------- */
void updatepixels()
// Reads through the matrix and updates the pixel color
{
  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      setpixel(row, col, board[row][col]);
      levelcheck(row, col);
    }
  }
  strip.show();
}
/* ------------------------------------------------- */
void setpixel (int row, int col, byte color)
{
// r = red/bad guy
// o = orange/bad guy with armor
// c = purple/space ship
// n = none/blank
// l = blue/your laser
// g = green/bad guy laser
// w = white/you got hit
  
  int pixelnum = 5*col + (4-row);

  if (color == 'c')
  {
    strip.setPixelColor(pixelnum, 200, 0, 200);
  }
  else if (color == 'r')
  {
    strip.setPixelColor(pixelnum, 80, 0, 0);
  }
  else if (color == 'o')
  {
    strip.setPixelColor(pixelnum, 255, 70, 0);
  }
  else if (color == 'w')
  {
    strip.setPixelColor(pixelnum, 255, 255, 255);
  }
  else if (color == 'l')
  {
    strip.setPixelColor(pixelnum, 0, 0, 80);
  }
  else if (color == 'g')
  {
    strip.setPixelColor(pixelnum, 0, 90, 0);
  }
  else
  {
    strip.setPixelColor(pixelnum, 0, 0, 0);
  }
}
/* ------------------------------------------------- */
void PI_firewepon()
{
  // Find out what column the ship is in and fire in that column
  if (board[4][0] == 'c')
  {
    board[3][0] = 'l';
  }
  else if (board[4][1] == 'c')
  {
    board[3][1] = 'l';
  }
  else if (board[4][2] == 'c')
  {
    board[3][2] = 'l';
  } 
}
/* ------------------------------------------------- */
// All button functions
byte newleftclick() // call to check if the button was clicked and clears the click;
{
  if (newleftclickholder)
  {
    newleftclickholder = 0;
    return(1);
  }
  return (0);
}

byte newrightclick() // checks if the button was clicked and clears the click;
{
  if (newrightclickholder)
  {
    newrightclickholder = 0;
    return(1);
  }
  return (0);
}

byte newbothclick() // checks if the button was clicked and clears the click;
{
  if (newbothclickholder)
  {
    newbothclickholder = 0;
    return(1);
  }
  return (0);
}

byte checkbuttons() // checks the current status of buttons - returns 00, 01, 10 or 11 to represent which buttons are down.
{  
  //  buttonreadstate 00 is both off, 01 is right button, 10 is left button, 11 is both buttons, 1111 is long press/reset
  byte buttonreadstate = (2*(!(digitalRead(LBUTPIN)))) + (!(digitalRead(RBUTPIN)));

  if (buttonreadstate == buttonstate) // Nothing changed since last check
  {
    if (buttonstate == B00000011) // both staying pressed
    {
      longpresscounter++;
      if (longpresscounter > resetdelay)
      {
        longpresscounter = 0;
        reset = 1;
        return 0;
      }
    }
    else
    {
      longpresscounter = 0;
    }
  }
  else if ( !(buttonstate & B00000001) && (buttonreadstate & B00000001)) // Right button wasn't pressed but is now
  {
    delay(35); // wait 35ms to give some wiggle room for a simultaneous both button press.
    buttonreadstate = (2*(!(digitalRead(LBUTPIN)))) + (!(digitalRead(RBUTPIN))); // read buttons again after the delay
    if ( (!(buttonstate & B00000010)) && ((buttonreadstate & B00000010))) // Left button wasn't pressed but is now
    {
      newbothclickholder = 1;    
    }
    else
    {
      newrightclickholder = 1;
    }
  }
  else if ( !(buttonstate & B00000010) && (buttonreadstate & B00000010)) // Left button now pressed
  {
    delay(35); // wait 35ms to give some wiggle room for a simultaneous both button press.
    buttonreadstate = (2*(!(digitalRead(LBUTPIN)))) + (!(digitalRead(RBUTPIN))); // read buttons again after the delay
    if ( !(buttonstate & B00000001) && (buttonreadstate & B00000001)) // Right button now pressed
    {
      newbothclickholder = 1;    
    }
    else
    {
      newleftclickholder = 1;
    }
  }
  buttonstate = buttonreadstate;
  return buttonstate;
}
/* ------------------------------------------------- */
void PI_boardrefresh()
{
  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      if (board[0][col] == 'l') // Removes the laser if it ends up on the top row
      {
        board[0][col] = 'n';
      }
      if (board[row][col] == 'l' and row > 0) // Move the ship laser fire up the screen
      {
        if (board[row - 1][col] == 'r' or board[row - 1][col] == 'o')
        {
          if (board[row - 1][col] == 'o') // Reduces the enemy armor if it gets hit
          {
            board[row - 1][col] = 'r';
            board[row][col] = 'n';
          }
          else if (board[row - 1][col] == 'r') // Kills the enemy without armor
          {
            board[row - 1][col] = 'n';
            board[row][col] = 'n';
          }
        }
        else if (board[row - 1][col] == 'g') // Shoot the bad guys laser fire
        {
          board[row - 1][col] = 'n';
          board[row][col] = 'n';
        }
        else // Moves your laser fire up the screen
        {
          board[row - 1][col] = 'l';
          board[row][col] = 'n';
        }
      }
    }
  }
  delay(50); // Delay that controls how fast the refresh happens. (i.e. the laser fire)
}
/* ------------------------------------------------- */
void PI_enemyfirerefresh()
{
  if (level > 4 and level < 10)
  {
    pause = 10;
  }
  if (level > 10)
  {
    pause = 4;
  }
  // This makes the enemy laser refresh less often.
  counter += 1;
  if (counter >= pause)
  {
    for (int row = 4; row > 0; row--)
    {
      for (int col = 0; col < 3; col++)
      {
        if (board[row][col] == 'g') // Moves the enemy laser down the screen
        {
          if (board[row + 1][col] == 'c')
          {
            board[row][col] = 'n';
            PRcollision = col; // a collision happened in column col
            processcollision();
            //collision(row, col);
            counter = 0;
          }
          else
          {
            board[row + 1][col] = 'g';
            board[row][col] = 'n';
            counter = 0;
          }
        }
      }
    }
  }
}
/* ------------------------------------------------- */
void PR_newtoprow()
{
  int randNumber = random(0,7);
  if (randNumber  == 0)
  {
    board[0][0] = 'r';
    board[0][1] = 'n';
    board[0][2] = 'n'; 
  }
  else if (randNumber  == 1)
  {
    board[0][0] = 'r';
    board[0][1] = 'n';
    board[0][2] = 'n'; 
  }
  else if (randNumber  == 2)
  {
    board[0][0] = 'n';
    board[0][1] = 'r';
    board[0][2] = 'n'; 
  }
  else if (randNumber  == 3)
  {
    board[0][0] = 'n';
    board[0][1] = 'n';
    board[0][2] = 'r'; 
  }
  else if (randNumber  == 4)
  {
    board[0][0] = 'n';
    board[0][1] = 'r';
    board[0][2] = 'n'; 
  }
  else 
  {
    board[0][0] = 'n';
    board[0][1] = 'n';
    board[0][2] = 'r'; 
  }
}
/* ------------------------------------------------- */
void PR_pixelsdown()
{
  // moves all pixels down the grid, checks for collision
  for (int row = 4; row > 0; row--)
  {
    for (int col = 0; col < 3; col++)
    {
      if ( (board[row][col] == 'c')) // currently the car
      {
        if (board[row - 1][col] == 'r') // red is approaching
        {
          PRcollision = col; // a collision happened in column col
          score = score -  10;
          if (score < 10)
          {
            score = 1;
          }
        }
        //else if (board[row - 1][col] == 'g') // red is approaching
        //{
        //  score = score + 5;
        //}
        //if (score > 250)
        //{
        //  score = 254;
        //}
      }
      else
      {
        board[row][col] = board[row - 1][col];      
      }
    }
  }
}
/* ------------------------------------------------- */
void processcollision()
{
  setpixel(4, PRcollision, 'n');
  strip.show();
  delay(70);
  setpixel(4, PRcollision, 'c');
  strip.show();
  delay(60);
  setpixel(4, PRcollision, 'n');
  strip.show();
  delay(60);
  setpixel(4, PRcollision, 'c');
  strip.show();
  delay(60);
  setpixel(4, PRcollision, 'n');
  strip.show();
  delay(60);
  setpixel(4, PRcollision, 'c');
  //board[4][collision] = 'c';// put the car back after the collision.
  PRcollision = -1 ;

  lives -= 1; // You died. Subtract a life
  if (lives <= 0) // Go to gameover if you are out of lives
  {
    gameover();
  }
}
  /* ----------------------------------------------- */
  /*  Functions related to the PixPattern game (PP)  */
  /* ----------------------------------------------- */
/* ------------------------------------------------- */
void PP_movecursor()
{
  if (currow == 0)
  {
    if (curcol == 2)
    {
      currow++;
    }
    else
    {
      curcol++;
    }
  }
  else if (currow == 4)
  {
    if (curcol == 0)
    {
      currow--;
    }
    else
    {
      curcol--;
    }
  }
  else if (curcol == 0)
  {
    if (currow == 0)
    {
      curcol++;
    }
    else
    { 
      currow--;
    }
  }
  else if (curcol == 2)
  {
    if (currow == 4)
    {
      curcol--;
    }
    else
    {
      currow++;
    }
  }
}
/* ------------------------------------------------- */
void PP_showcursor()
{
  setpixel (currow, curcol, colorcarry);
}
/* ------------------------------------------------- */
void PP_newlevel()
{
  // Pick four random colors
  byte randcolorT;
  byte randcolorB;
  byte randcolorL;
  byte randcolorR;
  
  // Pick a random color for the top row
  randColor = colors[random(0,5)];
  for (int col = 0; col < 3; col++)
  {
    board[0][col] = randColor;
  }
  delay(100);
  // Pick a random color for the bottom row
  randColor = colors[random(0,5)];
  for (int col = 0; col < 3; col++)
  {
    board[4][col] = randColor;
  }
  delay(100);
  // Pick a random color for the left side
  randColor = colors[random(0,5)];
  for (int row = 1; row < 4; row++)
  {
    board[row][0] = randColor;
  }
  delay(100);
  // Pick a random color for the right side
  randColor = colors[random(0,5)];
  for (int row = 1; row < 4; row++)
  {
    board[row][2] = randColor;
  }

  delay(100);
  // Blank out the center column
  board[1][1] = 'n';
  board[2][1] = 'n';
  board[3][1] = 'n';

  updatepixels();
  //strip.show();
  
  delay(4000); // Pause for 4 seconds to show the player the board they will have to make
  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      pattern[row][col] = board[row][col]; // Save the board layout
    }
  }

  // Shift a given number of pixels based on the level
  for (int shift = 0; shift <= pixshift; shift++)
  {
    int randCol1 = 0;
    int randRow1 = 0;
    int randCol2 = 0;
    int randRow2 = 0;
    while (randCol1 == randCol2 or randRow1 == randRow2)
    {
      randCol1 = columns[random(0,2)];
      randRow1 = random(0,4);
      randCol2 = columns[random(0,2)];
      randRow2 = random(0,4);
    }
    byte temp1 = board[randRow1][randCol1];
    board[randRow1][randCol1] = board[randRow2][randCol2];
    board[randRow2][randCol2] = temp1;
  }

  // Show the player the shifted board
  updatepixels();
  strip.setBrightness(brightness + 30);
  strip.show();
  delay(1000);
  strip.setBrightness(brightness);
}
/* ------------------------------------------------- */
void PP_patterncheck()
{
  // Check to see if the player has matched the begining pattern (i.e. passed the level)
  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      if (board[row][col] == pattern[row][col])
      {
        colorcount += 1;
      }
    }
  }
  if (colorcount == 15) // You won the level!
  {
    delay(1000);
    loopspeed -= 200;
    colorcarrytemp = colorcarry;
    levelcleared();
    boardclear();
    PP_newlevel();
  }
  else
  {
    colorcount = 0;
  }
}
/* ------------------------------------------------- */
void PR_levelcheck()
{
  // Check to see if the player as passed 30 cars (+ the 5 intro pixels)
  if (PRpassed >= PRpasscount)
  {
    delay(1000);
    levelcleared();
    PRpassed = 0;
    masterspeeddelay -= 50;
    PRpasscount += 10;
    startpixracer();
    
  }
  else
  {
    PRpassed += 1;
  }
}
/* ------------------------------------------------- */

/* ------------------------------------------------- */
/*       Main loop for the games to play             */
/* ------------------------------------------------- */
void loop() {
  if (game == 99)
  {
    gameselect();
  }
  /* -------------------------------- */
  /*  This is the PixRacer game (PR)  */
  /* -------------------------------- */
  while (game == 0)
  {
    if (starter == 0)
    {
      startpixracer();
    }
    speeddelay --;
    if (speeddelay < 2)
    {
      speeddelay = masterspeeddelay;
      PR_levelcheck();
      PR_pixelsdown();
      PR_newtoprow();
      updatepixels();
      strip.show();
      
      if (PRcollision > -1)
      {
        processcollision();
      }
    }
    checkbuttons();

    if (newleftclick()) // Move car left
    {
      if (board[4][1] == 'c')
      {
        board[4][0] = 'c';
        board[4][1] = 'n';
      }
      else if (board[4][2] == 'c')
      {
        board[4][1] = 'c';
        board[4][2] = 'n';
      }           
    }
    if (newrightclick()) // Move car right
    {
      if (board[4][0] == 'c')
      {
        board[4][0] = 'n';
        board[4][1] = 'c';
      }
      else if (board[4][1] == 'c')
      {
        board[4][2] = 'c';
        board[4][1] = 'n';
      }
    }
    if (reset == 1)
    {
      gameover();
    }
    
    updatepixels();
    strip.show();
  }
  /* ----------------------------------- */
  /*  This is the PixInvaders game (PI)  */
  /* ----------------------------------- */
  while (game == 1)
  {
    if (starter == 0)
    {
      startpixinvaders();
    }
    checkbuttons();
    
    if (newleftclick()) // Move ship left
    {
      if (board[4][1] == 'c')
      {
        board[4][0] = 'c';
        board[4][1] = 'n';
      }
      else if (board[4][2] == 'c')
      {
        board[4][1] = 'c';
        board[4][2] = 'n';
      }           
    }
    if (newrightclick()) // Move ship right
    {
      if (board[4][0] == 'c')
      {
        board[4][0] = 'n';
        board[4][1] = 'c';
      }
      else if (board[4][1] == 'c')
      {
        board[4][2] = 'c';
        board[4][1] = 'n';
      }
    }
    if (newbothclick()) // Fire the laser
    {
      PI_firewepon();
    }
    if (reset == 1)
    {
      gameover();
    }
    
    updatepixels();
    PI_enemyfire();
    updatepixels();
    PI_enemyfirerefresh();
    PI_boardrefresh();
    strip.show();
  }
  /* ---------------------------------- */
  /*  This is the PixPattern game (PP)  */
  /* ---------------------------------- */
  while (game == 2)
  {
    if (starter == 0)
    {
      startpixpattern();
    }
    for (int loopcount = 0; loopcount < loopspeed; loopcount++)
    {
      if ((loopcount == 1)) // will do every 2000th time through the loop
      {
        updatepixels();
        PP_movecursor();
        PP_showcursor();
        strip.show();
      }
      checkbuttons();
      
      if (newrightclick() or newleftclick()) // Change colors out
      {
        int colorcarrytemp = board[currow][curcol];
        board[currow][curcol]= colorcarry;
        colorcarry = colorcarrytemp;
        PP_patterncheck();
      }
      if (reset == 1)
      {
        gameover();
      }
    }
  }
}
