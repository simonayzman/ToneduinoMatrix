/**********************************************/
/*** Original idea for the Toneduino Matrix ***/
/***    comes from the following source:    ***/
/***    http://tonematrix.audiotool.com/    ***/
/**********************************************/

/**************************/
/*** LIBRARIES INCLUDED ***/
/**************************/

#include <Adafruit_STMPE610.h>    // Source: https://github.com/adafruit/Adafruit_STMPE610
#include <Adafruit_GFX.h>         // Source: https://github.com/adafruit/Adafruit-GFX-Library
#include <RGBmatrixPanel.h>       // Source: https://github.com/adafruit/RGB-matrix-Panel
#include <Tone.h>
#include <Wire.h>
#include <SPI.h>

/****************************************/
/*** GLOBAL DEFINITIONS AND INSTANCES ***/
/****************************************/

/*** LED Matrix Definitions ***/

// Values required for initialization
#define CLK 11  // Use pin 8 by default, and pin 11 on the Mega
#define OE  9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

// Color identifiers; Must be initialized after matrix is initialized
uint16_t RED;
uint16_t GREEN;
uint16_t BLUE;
uint16_t ORANGE;
uint16_t PURPLE;
uint16_t WHITE;
uint16_t BLACK;   // no color, in actuality
  
/*** Logical boards Definitions ***/
#define ROWS      16
#define COLUMNS   16

int activeTonesBoard[ROWS][COLUMNS];                // The logical representation of the activated positions of the board
int activeTonesInColumns[COLUMNS];                  // Stores the active index positions of every column; INACTIVE_TONE by default
uint16_t currentColorsOfTonesInColumns[COLUMNS];    // Stores the color of the active notes of every column; BLACK by default

#define ACTIVE_TONE       1        // Only for activeTonesBoard
#define INACTIVE_TONE     -1       // Only for activeTonesBoard
#define NO_TONE_SELECTED  -1       // Only for activeTonesInColumns

/*** Frequency and tone definitions ***/
#define TONE_PIN          5
#define NOTE_LENGTH_PIN   A15      // Analog pin that will be used as input for the user's desired note length

#define SLOWEST_NOTE_LENGTH   300
#define FASTEST_NOTE_LENGTH   50

int currentNoteLength;            

const int tones[ROWS] = {NOTE_AS5,NOTE_A5,NOTE_GS5,NOTE_G5,NOTE_FS5,NOTE_F5,NOTE_E5,NOTE_DS5,NOTE_D5,NOTE_CS5,NOTE_C5,NOTE_B4,NOTE_AS4,NOTE_A4,NOTE_GS4,NOTE_G4};
//const int tones[ROWS] = {NOTE_C7,NOTE_A6,NOTE_G6,NOTE_F6,NOTE_D6,NOTE_C6,NOTE_A5,NOTE_G5,NOTE_F5,NOTE_D5,NOTE_C5,NOTE_A4,NOTE_G4,NOTE_F4,NOTE_D4,NOTE_C4,};

/*** Debounce values ***/
#define TOUCH_SCREEN_DELAY                      200 
#define PAUSE_PLAY_BUTTON_LAST_PRESSED_DELAY    300
#define RESET_BUTTON_LAST_PRESSED_DELAY         300

/*** Volatile data instances ***/
volatile unsigned long lastTouchedScreenTime = 0;
volatile unsigned long lastPausePlayTouchTime = 0;
volatile unsigned long lastResetTouchTime = 0;
volatile boolean isPlaying = true;

/*** Interrupt definitions ***/
#define PAUSE_PLAY_INTERRUPT_NUMBER   0     // Considered to be Pin 2 on the Mega
#define RESET_INTERRUPT_NUMBER        1     // Considered to be Pin 3 on the Mega

/*** Creation of major global instances ***/
Tone tonePlayer;
Adafruit_STMPE610 touch = Adafruit_STMPE610();
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

void setup()
{
  Serial.begin(9600);
  Serial.println("");
  Serial.print("*** Start initialization ***");
  
  /*** LEDMatrix Initialization, Definitions, and Test Sequence ***/
  matrix.begin();
  RED = matrix.Color888(255,0,0);
  GREEN = matrix.Color888(0,255,0);
  BLUE = matrix.Color888(0,0,255);
  ORANGE = matrix.Color888(255,128,0);
  PURPLE = matrix.Color888(127,0,255);
  WHITE = matrix.Color888(150,150,150);
  BLACK = matrix.Color888(0,0,0);
  for(int i = 0; i < 6; ++i)
  {
    if (i == 1)       matrix.fillScreen(WHITE); 
    else if (i == 1)  matrix.fillScreen(RED);
    else if (i == 2)  matrix.fillScreen(GREEN); 
    else if (i == 3)  matrix.fillScreen(BLUE); 
    else if (i == 4)  matrix.fillScreen(ORANGE); 
    else if (i == 5)  matrix.fillScreen(PURPLE); 
    delay(400);
  }
  matrix.fillScreen(BLACK);

  /*** Touch Screen Initialization ***/
  Serial.println("");
  if (!touch.begin()) Serial.print("STMPE not found!");
  else Serial.print("Waiting for touch sense");

  /*** Logical Tone Boards Initialization ***/
  for(int currCol = 0; currCol < COLUMNS; ++currCol) 
  { 
    activeTonesInColumns[currCol] = NO_TONE_SELECTED;
    currentColorsOfTonesInColumns[currCol] = BLACK;
    for(int currRow = 0; currRow < ROWS; ++currRow)
      activeTonesBoard[currRow][currCol] = INACTIVE_TONE; 
  }

  /*** Tone Initialization ***/
  tonePlayer.begin(TONE_PIN);
  
  /*** Interrupt Attachment ***/
  attachInterrupt(PAUSE_PLAY_INTERRUPT_NUMBER, resolvePaused, FALLING);
  attachInterrupt(RESET_INTERRUPT_NUMBER, clearAllBoards, FALLING);
  
  Serial.println("");
  Serial.print("*** End initialization ***");
}

void loop()
{ 
  Serial.println("");
  Serial.print("*** Beginning of Board ***"); 
  for(int currCol = 0; currCol < COLUMNS; ++currCol)
  {
    Serial.println("");
    Serial.print("* Column "); 
    Serial.print(currCol); 
    Serial.print(" *"); 
    
    // Synchronization occurs once per column during normal playing conditions
    // Synchronizes action made on the touch screen with the LED matrix
    // Synchronizes any changes to the duration of a single note
    synchronizeLEDMatrixWithTouchScreenInput();
    resolveNoteLengthChanges();
    
    // This loops infinitely when the system is paused as per the user's request
    // Convenient because the program then implicitly knows where it stopped last
    while(!isPlaying) 
    {
      // As indicated above, synchronization is required, even when paused    
      synchronizeLEDMatrixWithTouchScreenInput();
      resolveNoteLengthChanges();
    }

    // Quick look at whether an active tone exists in the current column;
    // Avoids the extra step of looping through another dimension in the logical tone board
    int activeNoteInColumn = activeTonesInColumns[currCol];
    if (activeNoteInColumn !=  NO_TONE_SELECTED)
    {
      pulseLEDFromActiveTonesBoardPosition(activeNoteInColumn,currCol);
      playNote(tones[activeNoteInColumn], currentNoteLength);
    }

    // Invisible delay, for when no note is played for the current column
    else 
    {
      delay(currentNoteLength);
    }

    // This extra delay makes a satisfying pause between
    // each note, making it clear that each one is distinct
    delay(currentNoteLength/8);
  }
}

void synchronizeLEDMatrixWithTouchScreenInput()
{
  // Checks if the screen is being touched and if enough time has passed
  // since the last synchronization step in order to do this one
  // 
  // ****HOWEVER****
  //
  // In reality, these checks are somewhat moot when the user wants a slower rhythm because 
  // the touch will only be registered by the Arduino when the system is not being delay()-ed
  //
  // Therefore, there will sometimes seem to be a lag between a user's touch 
  // and the eventual synchronization with the LED matrix (if at all, in fact)
  //
  // What actually happens then is that when the user decided note length (x 1.125, because there 
  // is also the between note delay) is less than TOUCH_SCREEN_DELAY, the user's touch will be 
  // registered after TOUCH_SCREEN_DELAY milliseconds, but if the note length x 1.125 is bigger, 
  // then it will be registered in note length x 1.125 milliseconds
  if ((millis() - lastTouchedScreenTime) > TOUCH_SCREEN_DELAY && touch.touched())
  {
    uint16_t x, y;
    uint8_t z;
    while (!touch.bufferEmpty())
      touch.readData(&x, &y, &z);
    
    // Sometimes, the touch screen reads an incredibly high value outside of its
    // possible pixel range (something like ~58,000), so this is one last safeguard
    // that prevents an incorrect update to the logical and LED matrices
    if (x < 4100 && y < 4100)
    {
      // Constraints in place to fit average outer bounds of touch screen
      // Values were found by trial and error, rather than actual calculation
      // Touch screen data values are not consistently precise enough  
      int activateCurrentColumn = constrain(map(x,175,4000,0,16),0,15);
      int activateCurrentRow = constrain(map(y,325,3650,0,16),0,15);
      
      // Relevant information from logical boards is collected
      int isNoteInPositionActive = activeTonesBoard[activateCurrentRow][activateCurrentColumn];
      int activeRowInCurrentColumn = activeTonesInColumns[activateCurrentColumn];
      
      // If this position is already on (and must be turned off)
      if (isNoteInPositionActive == ACTIVE_TONE)
      {
        Serial.println("");
        Serial.print("Turning off coordinate (");
        Serial.print(activateCurrentRow);
        Serial.print(",");
        Serial.print(activateCurrentColumn);
        Serial.print(").");
        turnOffPositionInActiveTonesBoard(activateCurrentRow,activateCurrentColumn);
        turnOffLEDFromActiveTonesBoardPosition(activateCurrentRow,activateCurrentColumn);
      }
      // If this position is off, and there's another one active in this column
      else if(activeRowInCurrentColumn != NO_TONE_SELECTED)
      {
        Serial.println("");
        Serial.print("Changing active coordinate from (");
        Serial.print(activeRowInCurrentColumn);
        Serial.print(",");
        Serial.print(activateCurrentColumn);
        Serial.print(") to (");
        Serial.print(activateCurrentRow);
        Serial.print(",");
        Serial.print(activateCurrentColumn);
        Serial.print(").");
        turnOffPositionInActiveTonesBoard(activeRowInCurrentColumn,activateCurrentColumn);
        turnOffLEDFromActiveTonesBoardPosition(activeRowInCurrentColumn,activateCurrentColumn);
        turnOnPositionInActiveTonesBoard(activateCurrentRow,activateCurrentColumn);
        turnOnLEDFromActiveTonesBoardPosition(activateCurrentRow,activateCurrentColumn);
      }
      // If this position is off, and there are no other active positions in this column
      else
      {
        Serial.println("");
        Serial.print("Turning on coordinate (");
        Serial.print(activateCurrentRow);
        Serial.print(",");
        Serial.print(activateCurrentColumn);
        Serial.print(").");
        turnOnPositionInActiveTonesBoard(activateCurrentRow,activateCurrentColumn);
        turnOnLEDFromActiveTonesBoardPosition(activateCurrentRow,activateCurrentColumn);
      }
      lastTouchedScreenTime = millis();
    }
  }
}

/*** Music Function ***/

void playNote(int frequency, int duration)
{ 
  tonePlayer.play(frequency); 
  delay(duration); 
  tonePlayer.stop(); 
}

/*** Logical Tone Boards Functions ***/

void turnOnPositionInActiveTonesBoard(int row, int column) 
{ 
  activeTonesBoard[row][column] = ACTIVE_TONE;
  activeTonesInColumns[column] = row;
  currentColorsOfTonesInColumns[column] = WHITE;
}

void turnOffPositionInActiveTonesBoard(int row, int column) 
{ 
  activeTonesBoard[row][column] = INACTIVE_TONE; 
  activeTonesInColumns[column] = NO_TONE_SELECTED;
  currentColorsOfTonesInColumns[column] = BLACK;
}

void togglePositionInActiveTonesBoard(int row, int column) 
{ 
  if (activeTonesBoard[row][column] == ACTIVE_TONE)
    turnOffPositionInActiveTonesBoard(row,column);
  else 
    turnOnPositionInActiveTonesBoard(row,column);
}

void clearActiveTonesBoard()
{ 
  for(int currCol = 0; currCol < COLUMNS; ++currCol) 
  {
    activeTonesInColumns[currCol] = NO_TONE_SELECTED;
    currentColorsOfTonesInColumns[currCol] = BLACK;
    for(int currRow = 0; currRow < ROWS; ++currRow) 
      activeTonesBoard[currRow][currCol] = INACTIVE_TONE; 
  } 
}

void printActiveTonesBoard()
{ 
  Serial.println("");
  for(int currRow = 0; currRow < ROWS; ++currRow) 
  {
    for(int currCol = 0; currCol < COLUMNS; ++currCol) 
      Serial.print(activeTonesBoard[currRow][currCol] + " ");
    Serial.println("");
  } 
}

/*** LED Matrix Functions ***/

void drawTwoByTwoClusteredPixelsWithColor(int x, int y, uint16_t color)
{
  matrix.drawPixel(x,y,color);
  matrix.drawPixel(x,y+1,color);
  matrix.drawPixel(x+1,y,color); 
  matrix.drawPixel(x+1,y+1,color);
}

void turnOnLEDFromActiveTonesBoardPosition(int row, int column)
{
  // The multiplication by two adjusts for the 16x16 to 32x32 adaptation
  drawTwoByTwoClusteredPixelsWithColor(column*2,row*2,WHITE);
}

void turnOffLEDFromActiveTonesBoardPosition(int row, int column)
{
  // The multiplication by two adjusts for the 16x16 to 32x32 adaptation
  drawTwoByTwoClusteredPixelsWithColor(column*2,row*2,BLACK);
}

void pulseLEDFromActiveTonesBoardPosition(int row, int column)
{
  uint16_t currentColor = currentColorsOfTonesInColumns[column];
  
  uint16_t randomOtherColor;
  if (currentColor == RED)
  {
    uint16_t otherColors[5] = {GREEN, BLUE, ORANGE, PURPLE, WHITE};
    randomOtherColor = otherColors[random(0,5)];
  }
  else if (currentColor == GREEN)
  {
    uint16_t otherColors[5] = {RED, BLUE, ORANGE, PURPLE, WHITE};
    randomOtherColor = otherColors[random(0,5)];
  }
  else if (currentColor == BLUE)
  {
    uint16_t otherColors[5] = {RED, GREEN, ORANGE, PURPLE, WHITE};
    randomOtherColor = otherColors[random(0,5)];
  }
  else if (currentColor == ORANGE)
  {
    uint16_t otherColors[5] = {RED, GREEN, BLUE, PURPLE, WHITE};
    randomOtherColor = otherColors[random(0,5)];
  }
  else if (currentColor == PURPLE)
  {
    uint16_t otherColors[5] = {RED, GREEN, BLUE, ORANGE, WHITE};
    randomOtherColor = otherColors[random(0,5)];
  }
  else if (currentColor == WHITE)
  {
    uint16_t otherColors[5] = {RED, GREEN, BLUE, ORANGE, PURPLE};
    randomOtherColor = otherColors[random(0,5)];
  }
  else
  {
    Serial.println("");
    Serial.print("Unexpected behavior.");
    uint16_t otherColors[6] = {RED, GREEN, BLUE, ORANGE, PURPLE, WHITE};
    randomOtherColor = otherColors[random(0,6)];
  }
  currentColorsOfTonesInColumns[column] = randomOtherColor;
  
  // The multiplication by two adjusts for the 16x16 to 32x32 adaptation
  drawTwoByTwoClusteredPixelsWithColor(column*2,row*2,randomOtherColor);
}

void clearLEDMatrix()
{
   matrix.fillScreen(BLACK);  
}

/*** Note Length Function ***/

void resolveNoteLengthChanges() 
{
  currentNoteLength = map(analogRead(NOTE_LENGTH_PIN), 0, 1023, FASTEST_NOTE_LENGTH, SLOWEST_NOTE_LENGTH);
}

/*** Interrupt Functions ***/

void clearAllBoards() 
{ 
  if ((millis() - lastResetTouchTime) > RESET_BUTTON_LAST_PRESSED_DELAY)
  {
    clearActiveTonesBoard(); 
    clearLEDMatrix();
    Serial.println("");
    Serial.print("ToneMatrix has been cleared."); 
    lastResetTouchTime = millis();
  }
}

void resolvePaused() 
{
  if ((millis() - lastPausePlayTouchTime) > PAUSE_PLAY_BUTTON_LAST_PRESSED_DELAY)
  {
    isPlaying = (isPlaying ? false : true); 
    Serial.println("");
    Serial.print("The ToneMatrix is now ");
    Serial.print((isPlaying ? "playing." : "paused."));
    lastPausePlayTouchTime = millis();
  }
}

