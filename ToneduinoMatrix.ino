#include <Adafruit_STMPE610.h>
#include <Adafruit_GFX.h> 
#include <RGBmatrixPanel.h>
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

#define SLOWEST_NOTE_LENGTH   400
#define FASTEST_NOTE_LENGTH   50

int currentNoteLength;            

const int tones[ROWS] = {NOTE_G4,NOTE_GS4,NOTE_A4,NOTE_AS4,NOTE_B4,NOTE_C5,NOTE_CS5,NOTE_D5,NOTE_DS5,NOTE_E5,NOTE_F5,NOTE_FS5,NOTE_G5,NOTE_GS5,NOTE_A5,NOTE_AS5};
//const int tones[ROWS] = {NOTE_C7,NOTE_A6,NOTE_G6,NOTE_F6,NOTE_D6,NOTE_C6,NOTE_A5,NOTE_G5,NOTE_F5,NOTE_D5,NOTE_C5,NOTE_A4,NOTE_G4,NOTE_F4,NOTE_D4,NOTE_C4,};

/*** Debounce values ***/
#define TOUCH_SCREEN_DELAY                      650 
#define PAUSE_PLAY_BUTTON_LAST_PRESSED_DELAY    500
#define RESET_BUTTON_LAST_PRESSED_DELAY         500

int lastTouchedScreenTime = -TOUCH_SCREEN_DELAY;
int lastPausePlayTouchTime = -PAUSE_PLAY_BUTTON_LAST_PRESSED_DELAY;
int lastResetTouchTime = -RESET_BUTTON_LAST_PRESSED_DELAY;

/*** Volatile data instance ***/
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
  touch.writeRegister8(STMPE_INT_STA, 0xFF);     // initial reset of touch screen values

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
    Serial.print(currCol+1); 
    Serial.print(" *"); 

    // Synchronization occurs once per column during normal playing conditions
    // Synchronizes action made on the touch screen with the LED matrix
    // Synchronizes any changes to the duration of a single note
    synchronizeLEDMatrixWithTouchScreenInput();
    resolveNoteLengthChanges();
    
    // This loops infinitely when the system is paused as per the user's request
    // Convenient because it implicitly knows where it stopped last
    while(!isPlaying) 
    {
      // As above, synchronization is required, even when paused    
      synchronizeLEDMatrixWithTouchScreenInput();
      resolveNoteLengthChanges();
    }

    // Quick look at whether an active tone exists in the current column;
    // Avoids the extra step of looping through another dimension in the logical tone board
    int activeNoteInColumn = activeTonesInColumns[currCol];
    if (activeNoteInColumn !=  NO_TONE_SELECTED)
    {
      pulseLEDFromactiveTonesBoardPosition(activeNoteInColumn,currCol);
      playNote(tones[activeNoteInColumn], currentNoteLength);
    }

    // Invisible delay, for when no note is played for the current column
    else 
    {
      delay(currentNoteLength);
    }

    // This extra delay makes a satisfying pause between 
    // each note, making it clear that each is distinct
    delay(currentNoteLength/8);
  }
}

void synchronizeLEDMatrixWithTouchScreenInput()
{
  int activateCurrentRow = -1;
  int activateCurrentColumn = -1;

  // Checks if the screen is being touched and if enough time has passed
  // since the last synchronization step in order to do this one
  if (((millis() - lastTouchedScreenTime) > TOUCH_SCREEN_DELAY) && checkIfTouchScreenNoteHasBeenSelected(activateCurrentRow,activateCurrentColumn))
  {
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
      turnOffPositionInactiveTonesBoard(activateCurrentRow,activateCurrentColumn);
      turnOffLEDFromactiveTonesBoardPosition(activateCurrentRow,activateCurrentColumn);
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
      turnOffPositionInactiveTonesBoard(activeRowInCurrentColumn,activateCurrentColumn);
      turnOffLEDFromactiveTonesBoardPosition(activeRowInCurrentColumn,activateCurrentColumn);
      turnOnPositionInactiveTonesBoard(activateCurrentRow,activateCurrentColumn);
      turnOnLEDFromactiveTonesBoardPosition(activateCurrentRow,activateCurrentColumn);
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
      turnOnPositionInactiveTonesBoard(activateCurrentRow,activateCurrentColumn);
      turnOnLEDFromactiveTonesBoardPosition(activateCurrentRow,activateCurrentColumn);
    }
    lastTouchedScreenTime = millis();
  }
}

/*** Touch Screen Functions ***/

bool checkIfTouchScreenNoteHasBeenSelected(int &activateCurrentRow, int &activateCurrentColumn)
{
  bool isTouched = false;

  if (touch.touched()) 
  {
    uint16_t x, y;
    uint8_t z;

    // Reads in data until it is consistent
    while (!touch.bufferEmpty()) 
      touch.readData(&x, &y, &z);

    // Constraints in place to fit average
    // outer bounds of touch screen 
    constrain(x,200,4000);
    constrain(y,400,3700);
    activateCurrentColumn = map(x,200,4000,0,15);
    activateCurrentRow = map(y,400,3700,0,15);

    isTouched = true;
  }
  touch.writeRegister8(STMPE_INT_STA, 0xFF); // reset all ints

  return isTouched;
}

/*** Music Function ***/

void playNote(int frequency, int duration)
{ 
  tonePlayer.play(frequency); 
  delay(duration); 
  tonePlayer.stop(); 
}

/*** Logical Tone Boards Functions ***/

void turnOnPositionInactiveTonesBoard(int row, int column) 
{ 
  activeTonesBoard[row][column] = ACTIVE_TONE;
  activeTonesInColumns[column] = row;
  currentColorsOfTonesInColumns[column] = WHITE;
}

void turnOffPositionInactiveTonesBoard(int row, int column) 
{ 
  activeTonesBoard[row][column] = INACTIVE_TONE; 
  activeTonesInColumns[column] = NO_TONE_SELECTED;
  currentColorsOfTonesInColumns[column] = BLACK;
}

void togglePositionInactiveTonesBoard(int row, int column) 
{ 
  if (activeTonesBoard[row][column] == ACTIVE_TONE)
    turnOffPositionInactiveTonesBoard(row,column);
  else 
    turnOnPositionInactiveTonesBoard(row,column);
}

void clearactiveTonesBoard()
{ 
  for(int currCol = 0; currCol < COLUMNS; ++currCol) 
  {
    activeTonesInColumns[currCol] = NO_TONE_SELECTED;
    currentColorsOfTonesInColumns[currCol] = BLACK;
    for(int currRow = 0; currRow < ROWS; ++currRow) 
      activeTonesBoard[currRow][currCol] = INACTIVE_TONE; 
  } 
}

void printactiveTonesBoard()
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

void turnOnLEDFromactiveTonesBoardPosition(int row, int column)
{
  // The multiplication by two adjusts for the 16x16 to 32x32 adaptation
  drawTwoByTwoClusteredPixelsWithColor(column*2,row*2,WHITE);
}

void turnOffLEDFromactiveTonesBoardPosition(int row, int column)
{
  // The multiplication by two adjusts for the 16x16 to 32x32 adaptation
  drawTwoByTwoClusteredPixelsWithColor(column*2,row*2,BLACK);
}

void pulseLEDFromactiveTonesBoardPosition(int row, int column)
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
    clearactiveTonesBoard(); 
    clearLEDMatrix();
    Serial.println("");
    Serial.print("ToneMatrix has been cleared."); 
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

