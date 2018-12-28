/*-----------------------------------------------------------------
|  Author: Luis Dominguez - LADBSoft.com                          |
|          makerSquirrel (META port)                              |
|  Date: 24/12/2018                                 Version: 1.0  |
|-----------------------------------------------------------------|
|  Name: FireBuino!                                               |
|  Description: Remake of the classic Game&Watch Fire, from 1980. |
|-----------------------------------------------------------------|
|   C H A N G E L O G                                             |
|  ===================                                            |
|  Classic version:                                               |
|  1.0b: Basic functionality.                                     |
|  1.1b: Bug corrections and speed tweaking.                      |
|  1.2b: Added new graphics by Erico Patricio Monteiro. Added     |
|        main menu and pause screen.                              |
|  1.3b: Added 5-slot highscore ranking. Survivor spawning        |
|        tweaked. Minor enhancements.                             |
|  1.4b: Finally solved the bug that made no more survivors to    |
         spawn! Remember, freeing memory is good :P               |
|  Meta Version:                                                  |
|  0.8:  Port of Meta version works with high scores.             |
/  1.0:  Added LED and sound fx                                   |
|----------------------------------------------------------------*/

#include <Gamebuino-Meta.h>
#include "graphics.h"
#include "highscore.h"
#include "soundfx.h"

#define LCDHEIGHT gb.display.height()
#define LCDWIDTH gb.display.width()

class Survivor {
  public:
    int16_t _floor;
    int16_t _step;
    int16_t _delay;
    bool _bounced;
    bool _dead;

    Survivor(int16_t floor, int16_t delay) {
      _floor = floor;
      if (floor == 0) {
        _step = 0;
      } else if (floor == 1) {
        _step = 1;
      } else if (floor == 2) {
        _step = 2;
      }
      _delay = delay;
      _bounced = false;
      _dead = false;
    }
};

#define STATE_MENU 0
#define STATE_PLAYING 1
#define STATE_GAMEOVER 2
#define STATE_PAUSED 3
#define STATE_ABOUT 4

#define HIGHSCORE_COUNT 5
#define NAME_LETTERS 10
extern const char g_classicHighScoreName[] = "Classic";
extern const char g_newHighScoreName[] = "New";

HighScore<uint32_t, 6, 13, g_classicHighScoreName> g_classicHighScore;
#define SAVE_CLASSIC 0
HighScore<uint32_t, 6, 13, g_newHighScoreName> g_newHighScore;
#define SAVE_NEW 1

const SaveDefault savefileDefaults[] = {
  { SAVE_CLASSIC, SAVETYPE_BLOB, sizeof(g_classicHighScore), 0 },
  { SAVE_NEW, SAVETYPE_BLOB, sizeof(g_newHighScore), 0 }
};

uint8_t survivorCount;

uint32_t score;
uint32_t minHighscore;
int16_t lives;
int16_t playerPosition;
int16_t moveTick;
int16_t spawnDelay;
int16_t noOfSurvivors;
bool isClassic;
uint8_t gameState;
Survivor *survivors[10] = {nullptr}; //max. 10 survivors at the same time
bool occupiedWindows[3];

// Graphics (declared in graphics.ino)
/// for animations to work Images are not allowed to be const (and therefore in flash), the Image data shall stay in flash, though...
Image backgroundBitmapMeta(backgroundBitmapMetaData);
Image BackgroundClassic(BackgroundClassicData);
Image* backgroundMetaArray[] = {&backgroundBitmapMeta, &BackgroundClassic}; /// TODO: add classic graphics
Image burningfire(burningfireData);
Image ambulanceBitmapMeta(newAmbulanceData);
Image AmbulanceLightClassic(AmbulanceLightClassicData); // only the blinking light, not the full ambulance
Image* ambulanceMetaArray[] = {&ambulanceBitmapMeta, &AmbulanceLightClassic};
Image playerBitmapMeta(playerBitmapMetaData);
Image playerBitmapClassic(playerBitmapClassicData);
Image* playerMetaArray[] = {&playerBitmapMeta, &playerBitmapClassic};
Image survivor0BitmapMeta(survivor0BitmapMetaData);
Image survivor0BitmapClassic(survivor0BitmapClassicData);
Image* survivor0MetaArray[] = {&survivor0BitmapMeta, &survivor0BitmapClassic};
Image survivor0FlipBitmapMeta(survivor0FlipBitmapMetaData);
Image* survivor0FlipMetaArray[] = {&survivor0FlipBitmapMeta, &survivor0BitmapClassic};
Image survivor1BitmapMeta(survivor1BitmapMetaData);
Image survivor1BitmapClassic(survivor1BitmapClassicData);
Image* survivor1MetaArray[] = {&survivor1BitmapMeta, &survivor1BitmapClassic};
Image survivor1FlipBitmapMeta(survivor1FlipBitmapMetaData);
Image* survivor1FlipMetaArray[] = {&survivor1FlipBitmapMeta, &survivor1BitmapClassic};
Image survivor2BitmapMeta(survivor2BitmapMetaData);
Image survivor2BitmapClassic(survivor2BitmapClassicData);
Image* survivor2MetaArray[] = {&survivor2BitmapMeta, &survivor2BitmapClassic};
Image survivor2FlipBitmapMeta(survivor2FlipBitmapMetaData);
Image* survivor2FlipMetaArray[] = {&survivor2FlipBitmapMeta, &survivor2BitmapClassic};
Image survivor3BitmapMeta(survivor3BitmapMetaData);
Image survivor3BitmapClassic(survivor3BitmapClassicData);
Image* survivor3MetaArray[] = {&survivor3BitmapMeta, &survivor3BitmapClassic};
Image survivor3FlipBitmapMeta(survivor3FlipBitmapMetaData);
Image* survivor3FlipMetaArray[] = {&survivor3FlipBitmapMeta, &survivor3BitmapClassic};
Image survivor4BitmapMeta(survivor4BitmapMetaData);
Image survivor4BitmapClassic(survivor4BitmapClassicData);
Image* survivor4MetaArray[] = {&survivor4BitmapMeta, &survivor4BitmapClassic};
Image survivor4FlipBitmapMeta(survivor4FlipBitmapMetaData);
Image* survivor4FlipMetaArray[] = {&survivor4FlipBitmapMeta, &survivor4BitmapClassic};

Image firebuinoMenu = Image(firebuinoMenuData);
// Image ledLights = Image(ledLightsData);
Image ledLights = Image(ledLightsv3Data);

extern const uint8_t livesPositions[][3][2];
extern const uint8_t ambulancePositions[][2];
extern const uint8_t playerPositions[][3][2];
extern const uint8_t survivorPositions[][survivorNumberOfSteps][3];
extern const uint8_t survivorIdlePositions[][3];
extern const uint8_t survivorKOPositions[][3][2];


void initGame() {
  gb.pickRandomSeed();
  gb.lights.fill(BLACK);

  gameState = STATE_PLAYING;
  lives = 3;
  score = 0;
  playerPosition = 1;
  spawnDelay = 2;
  for (int i = 0; i < 10; i++) {
    delete(survivors[i]);
    survivors[i] = NULL;
  }
  survivors[0] = new Survivor(0, 3);
  noOfSurvivors = 1;
  survivorCount = 1;
  for (int i = 0; i < 3; i++) {
    occupiedWindows[i] = false;
  }
  occupiedWindows[0] = true;
}


void drawBackground() {
  gb.display.drawImage(0, 0,*backgroundMetaArray[isClassic]);
  if (isClassic)
    return;
  gb.display.drawImage(0, 0,burningfire);
  gb.lights.drawImage(0,0, ledLights);
}


void drawScore() {
  if (!isClassic) {
    gb.display.fontSize = 2;
    gb.display.setColor(WHITE, BLACK);

    if (score <= 9) {
      gb.display.cursorX = 72;
    } else if (score <= 99) {
      gb.display.cursorX = 64;
    } else if (score <= 999) {
      gb.display.cursorX = 56;
    } else {
      gb.display.cursorX = 48;
    }

    gb.display.cursorY = 2;
    gb.display.print(score);

    gb.display.fontSize = 1;
    gb.display.setColor(BLACK, WHITE);
  } else {
    gb.display.fontSize = 1;
    gb.display.setColor(BLACK, WHITE);

    if (score <= 9) {
      gb.display.cursorX = 49;
    } else if (score <= 99) {
      gb.display.cursorX = 45;
    } else if (score <= 999) {
      gb.display.cursorX = 41;
    } else if (score <= 9999) {
      gb.display.cursorX = 37;
    } else if (score <= 99999) {
      gb.display.cursorX = 33;
    } else if (score <= 999999) {
      gb.display.cursorX = 29;
    } else if (score <= 9999999) {
      gb.display.cursorX = 25;
    } else {
      gb.display.cursorX = 25;
    }
    gb.display.cursorY = 1;
    gb.display.print(score);
  }
}


/// TODO: reverse order of live signs!
void drawLives() {
  gb.display.setColor(WHITE);
  switch (lives) {
    case 3: gb.display.fillRect(livesPositions[isClassic][2][0],
                                    livesPositions[isClassic][2][1], 6, 6);
    case 2: gb.display.fillRect(livesPositions[isClassic][1][0],
                                    livesPositions[isClassic][1][1], 6, 6);
    case 1: gb.display.fillRect(livesPositions[isClassic][0][0],
                                    livesPositions[isClassic][0][1], 6, 6);
  }

  gb.display.setColor(RED);
  switch (lives) {
    case 3:
      gb.display.fillRect(livesPositions[isClassic][2][0]+1,
                                    livesPositions[isClassic][2][1]+2, 4, 2);
      gb.display.fillRect(livesPositions[isClassic][2][0]+2,
                                    livesPositions[isClassic][2][1]+1, 2, 4);
    case 2:
      gb.display.fillRect(livesPositions[isClassic][1][0]+1,
                                    livesPositions[isClassic][1][1]+2, 4, 2);
      gb.display.fillRect(livesPositions[isClassic][1][0]+2,
                                    livesPositions[isClassic][1][1]+1, 2, 4);
    case 1:
      gb.display.fillRect(livesPositions[isClassic][0][0]+1,
                                    livesPositions[isClassic][0][1]+2, 4, 2);
      gb.display.fillRect(livesPositions[isClassic][0][0]+2,
                                    livesPositions[isClassic][0][1]+1, 2, 4);
  }
}


void drawAmbulance() {
  uint8_t posX = ambulancePositions[isClassic][0];
  uint8_t posY = ambulancePositions[isClassic][1];
  gb.display.drawImage(posX, posY,*ambulanceMetaArray[isClassic]);
}


void movePlayer() {
  //Move player to the left if BUTTON_LEFT or BUTTON_A pressed
  if (gb.buttons.pressed(BUTTON_LEFT) || gb.buttons.pressed(BUTTON_A)) {
    if (playerPosition > 0) {
      playerPosition--;
    }
  }

  //Move player to the right if BUTTON_RIGHT or BUTTON_B pressed
  if (gb.buttons.pressed(BUTTON_RIGHT) || gb.buttons.pressed(BUTTON_B)) {
    if (playerPosition < 2) {
      playerPosition++;
    }
  }
}


void spawnSurvivor() {
  uint8_t mustSpawn;
  uint8_t floorNo;
  uint8_t delayTicks;

  if (noOfSurvivors != 0) {
    //More probable with higher score
    if (score <= (5 * 150)) {
      mustSpawn = random(0, (5 + 5 - (score / 150)));
    } else {
      mustSpawn = random(0, 5);
    }
  } else {
    //More probable if no survivors on screen
    mustSpawn = random(0, 2);
  }

  //Spawn if zero
  if (mustSpawn != 0)
    return;

  //Search for a blank spot in the array
  for (int i = 0; i < 10; i++) {
    if (survivors[i] != NULL)
        continue; // already in use
    //found!
    if (score < 300) {
      //Only Third floor
      floorNo = 0;
      delayTicks = random(1, 6);
    } else if (score < 600) {
      //Third or second floor
      floorNo = random(0, 2);
      delayTicks = random(2, 6);
    } else {
      //Any floor
      floorNo = random(0, 3);
      delayTicks = random(3, 6);
    }

    //Avoid occupied windows
    while (occupiedWindows[floorNo]) {
      if (floorNo == 0)
        break;
      floorNo--;
    }

    //If all windows are occupied, try later
    if (floorNo < 0)
      break;

    //Else, add the new survivor to the array
    survivors[i] = new Survivor(floorNo, delayTicks);

    noOfSurvivors++;
    survivorCount++;
    spawnDelay = 2;
    break;
  }
}


void gameLogic() {
  //Decrement movement tick delay
  moveTick--;

  //Game tick
  if (moveTick > 0)
    return;
  moveSurvivors();

  gb.sound.playTick();

  //Decrement spawn delay
  if (spawnDelay > 0)
    spawnDelay--;

  //Get faster with higher score
  if (score <= (13 * 50))
    moveTick = 4 + (13 - (score / 50));
  else
    moveTick = 4;
  if (moveTick > 13) // hack for speeding up the beginning of the game only
    moveTick = 13;

  //Try to spawn a new survivor
  if (spawnDelay <= 0)
    spawnSurvivor();
}


void moveSurvivors() {
  for (int i = 0; i < 10; i++) {
    if (survivors[i] == NULL) continue;
    //If dead, remove
    if (survivors[i]->_dead) {
      delete(survivors[i]);
      survivors[i] = NULL;
      noOfSurvivors--;
      continue;
    }

    //Movement or delay decrementation
    if (survivors[i]->_delay > 0)
      survivors[i]->_delay--;
    else
      survivors[i]->_step++;

    //Jump logic
    if ((survivors[i]->_floor == 0 && survivors[i]->_step == 1) ||
        (survivors[i]->_floor == 1 && survivors[i]->_step == 2) ||
        (survivors[i]->_floor == 2 && survivors[i]->_step == 3))
      occupiedWindows[survivors[i]->_floor] = false;

    //After bounce logic
    if ((survivors[i]->_step == 4) ||
        (survivors[i]->_step == 10) ||
        (survivors[i]->_step == 16)) {
      //Bounced against player; proceed
      if (survivors[i]->_bounced) {
        survivors[i]->_bounced = false;
        score++;
        //Missed, mark as dead
      } else {
        survivors[i]->_dead = true;
        gb.sound.playCancel();
        lives--;
        if (lives <= 0)
          gameState = STATE_GAMEOVER;
        continue;
      }
    }

    //Got to the ambulance
    if (survivors[i]->_step >= survivorNumberOfSteps) {
      delete(survivors[i]);
      survivors[i] = NULL;
      noOfSurvivors--;
      score += 10;
    }
  }
}


void drawSurvivors() {
  uint8_t posX;
  uint8_t posY;
  uint8_t mult;

  for (int i = 0; i < 10; i++) {
    if (survivors[i] == NULL) continue;
    if (survivors[i]->_dead) {
      if (survivors[i]->_step > 10) {
        posX = survivorKOPositions[isClassic][2][0];
        posY = survivorKOPositions[isClassic][2][1];
      } else if (survivors[i]->_step > 4) {
        posX = survivorKOPositions[isClassic][1][0];
        posY = survivorKOPositions[isClassic][1][1];
      } else {
        posX = survivorKOPositions[isClassic][0][0];
        posY = survivorKOPositions[isClassic][0][1];
      }
      gb.display.drawImage(posX,posY,*survivor3MetaArray[isClassic]);
    } else {
      if (survivors[i]->_delay > 0) {
        posX = survivorIdlePositions[isClassic][0];
        posY = survivorIdlePositions[isClassic][1];
        mult = survivorIdlePositions[isClassic][2];

        gb.display.drawImage(posX,posY + (survivors[i]->_floor * mult),*survivor0MetaArray[isClassic]);
      } else {
        posX = survivorPositions[isClassic][survivors[i]->_step][0];
        posY = survivorPositions[isClassic][survivors[i]->_step][1];

        switch (survivorPositions[isClassic][survivors[i]->_step][2]) {
          case 0:
            gb.display.drawImage(posX,posY,*survivor0MetaArray[isClassic]);
            break;
          case 1:
            gb.display.drawImage(posX,posY,*survivor1MetaArray[isClassic]);
            break;
          case 2:
            gb.display.drawImage(posX,posY,*survivor2MetaArray[isClassic]);
            break;
          case 3:
            gb.display.drawImage(posX,posY,*survivor3MetaArray[isClassic]);
            break;
          case 4:
            gb.display.drawImage(posX,posY,*survivor4MetaArray[isClassic]);
            break;

          //Flipped horizontally
          case 5:
            gb.display.drawImage(posX,posY,*survivor0FlipMetaArray[isClassic]);
            break;
          case 6:
            gb.display.drawImage(posX,posY,*survivor1FlipMetaArray[isClassic]);
            break;
          case 7:
            gb.display.drawImage(posX,posY,*survivor2FlipMetaArray[isClassic]);
            break;
          case 8:
            gb.display.drawImage(posX,posY,*survivor3FlipMetaArray[isClassic]);
            break;
          case 9:
            gb.display.drawImage(posX,posY,*survivor4FlipMetaArray[isClassic]);
            break;
        }
      }
    }
  }
}


void drawPlayer() {
  uint8_t posX = playerPositions[isClassic][playerPosition][0];
  uint8_t posY = playerPositions[isClassic][playerPosition][1];
  gb.display.drawImage(posX,posY,*playerMetaArray[isClassic]);
}


void checkBounces() {
  for (int i = 0; i < 10; i++) {
    if (!survivors[i]->_bounced) {
      if ((survivors[i]->_step == 3 && playerPosition == 0) ||
          (survivors[i]->_step == 9 && playerPosition == 1) ||
          (survivors[i]->_step == 15 && playerPosition == 2)) {
        survivors[i]->_bounced = true;
        // gb.sound.playOK();
      }
      /// handle soundfx:
      if ((survivors[i]->_step == 3) && survivors[i]->_bounced)
        gb.sound.fx(bounce1);
      else if ((survivors[i]->_step == 9) && survivors[i]->_bounced)
        gb.sound.fx(bounce2);
      else if ((survivors[i]->_step == 15) && survivors[i]->_bounced)
        gb.sound.fx(bounce3);
    }
  }
}


//Draw GAME OVER screen
void drawGameOver() {
  bool newHighScore = isClassic ? g_classicHighScore.checkHighScore(score) : g_newHighScore.checkHighScore(score);
  if (newHighScore)
    gb.sound.fx(highscoreFX);
  else
    gb.sound.fx(gameoverFX);
  if (isClassic)
  {
    g_classicHighScore.showScore(score);
    if (newHighScore)
    {
      g_classicHighScore.updateHighscore(score);
      gb.save.set( SAVE_CLASSIC , g_classicHighScore );
    }
    g_classicHighScore.drawHighScores();
  }
  else
  {
    g_newHighScore.showScore(score);
    if (newHighScore)
    {
      g_newHighScore.updateHighscore(score);
      gb.save.set( SAVE_NEW , g_newHighScore );
    }
    g_newHighScore.drawHighScores();
  }
}


void drawHighScores()
{
  g_classicHighScore.drawHighScores();
  g_newHighScore.drawHighScores();
}


void drawPaused() {
  gb.display.setColor(WHITE);
  gb.display.fillRect(28, 19, (gb.display.getFontWidth() * 6), gb.display.getFontHeight());
  gb.display.setColor(BLACK, WHITE);
  gb.display.cursorX = 29;
  gb.display.cursorY = 20;
  gb.display.print("PAUSED");

  gb.display.setColor(WHITE);
  gb.display.fillRect(36, LCDHEIGHT - (gb.display.getFontHeight() * 2) - 1, (gb.display.getFontWidth() * 11), gb.display.getFontHeight());
  gb.display.setColor(BLACK, WHITE);
  gb.display.cursorX = 36;
  gb.display.cursorY = LCDHEIGHT - (gb.display.getFontHeight() * 2);
  gb.display.print("\x15: Continue");

  gb.display.setColor(WHITE);
  gb.display.fillRect(51, LCDHEIGHT - gb.display.getFontHeight() - 1, (gb.display.getFontWidth() * 7), gb.display.getFontHeight());
  gb.display.setColor(BLACK, WHITE);
  gb.display.cursorX = 52;
  gb.display.cursorY = LCDHEIGHT - gb.display.getFontHeight();
  gb.display.print("\x17: Quit");
}


void drawCredits() {
  while (true) {
    if (!gb.update())
        continue;
    gb.display.clear();
    gb.display.fill(DARKBLUE);
    gb.display.setColor(WHITE, DARKBLUE);
    gb.display.cursorX = 22;
    gb.display.cursorY = 2;
    gb.display.print("Developer:");
    gb.display.cursorX = 26;
    gb.display.cursorY = 10;
    gb.display.print("LADBSoft");
    gb.display.cursorX = 16;
    gb.display.cursorY = 20;
    gb.display.print("New graphics:");
    gb.display.cursorX = 32;
    gb.display.cursorY = 28;
    gb.display.print("erico");
    gb.display.cursorX = 2;
    gb.display.cursorY = 38;
    gb.display.print("METAport&Animations");
    gb.display.cursorX = 2;
    gb.display.cursorY = 46;
    gb.display.print("makerSquirrel et al");

    gb.display.cursorY = LCDHEIGHT - gb.display.getFontHeight();
    gb.display.cursorX = 2;
    gb.display.print("v1.0 Dec24-2018");
    gb.display.cursorX = 52;
    // gb.display.print("\x17: Back");
    if (gb.buttons.pressed(BUTTON_A) || gb.buttons.pressed(BUTTON_B) || gb.buttons.pressed(BUTTON_MENU)) {
        gb.sound.playOK();
        break;
    }
  }
}


void setup() {
  gb.begin();
  gb.display.init(80, 64, ColorMode::index);
  gb.display.drawImage(0,0,firebuinoMenu);
  initGame();
  gb.save.config(savefileDefaults);
  gb.save.get(SAVE_NEW, g_newHighScore);
  g_newHighScore.setTextColor(YELLOW);
  g_newHighScore.setBgColor(DARKBLUE);
  gb.save.get(SAVE_CLASSIC, g_classicHighScore);
  g_classicHighScore.setTextColor(YELLOW);
  g_classicHighScore.setTextColor(DARKBLUE);
}


void loop()
{
  menuLoop();
}


// basically a rect around current menu selection
void setMenuCursor(uint8_t xSelect, uint8_t ySelect)
{
  xSelect = xSelect == 0 ? 1 : 41;
  uint8_t height = 18;
  uint8_t width = 38;
  ySelect = ySelect == 0 ? 25 : 44;
  gb.display.setColor(RED);
  gb.display.drawRect(xSelect, ySelect,width, height);
}


void menuLoop()
{
  uint8_t menuXPos = 0;
  uint8_t menuYPos = 0;
  while(true)
  {
    if(!gb.update())
      continue;
    gb.display.clear();
    gb.display.drawImage(0, 0, firebuinoMenu);

    if (gb.buttons.repeat(BUTTON_UP, 4) && menuYPos > 0)
      menuYPos--;
    if (gb.buttons.repeat(BUTTON_DOWN, 4) && menuYPos < 1)
      menuYPos++;
    if (gb.buttons.repeat(BUTTON_LEFT, 4) && menuXPos > 0)
      menuXPos--;
    if (gb.buttons.repeat(BUTTON_RIGHT, 4) && menuXPos < 1)
      menuXPos++;

    setMenuCursor(menuXPos, menuYPos);

    if (!gb.buttons.pressed(BUTTON_A) && !gb.buttons.pressed(BUTTON_B))
      continue;
    if (menuXPos == 0 && menuYPos == 0) // Start Game
    {
      isClassic = false;
      gameState = STATE_PLAYING;
      initGame();
      gameLoop();
    }
    else if (menuXPos == 1 && menuYPos == 0) // Use the classic start
    {
      isClassic = true;
      gameState = STATE_PLAYING;
      initGame();
      gameLoop();
    }
    else if (menuXPos == 0 && menuYPos == 1) // Show Highscore
    {
      g_classicHighScore.drawHighScores();
      g_newHighScore.drawHighScores();
    }
    else if (menuXPos == 1 && menuYPos == 1) // Show the credits
      drawCredits();
  }
}


void gameLoop() {
  while (1) {
    if (!gb.update()) continue;

    gb.display.clear();
      drawBackground();

      //Draw score on the top right corner, before the number of lives
      drawScore();

      //Draw lives on the top right corner, after the score
      drawLives();

      if (gameState == STATE_PLAYING) {
        //Move the player acordingly to the pressed buttons
        movePlayer();

        //Do game stuff (delays, spawning...)
        gameLogic();

        //Check collisions with player
        checkBounces();
      }

      //Draw all current survivors
      drawSurvivors();

      //Draw the ambulance
      drawAmbulance();

      //Draw the player
      drawPlayer();

      if (gameState == STATE_PAUSED) {
        drawPaused();
        //Unpause
        if (gb.buttons.pressed(BUTTON_A) || gb.buttons.pressed(BUTTON_B))
          gameState = STATE_PLAYING;
        else if (gb.buttons.pressed(BUTTON_MENU))
            break;
      }

      if (gameState == STATE_GAMEOVER)
      {
          drawGameOver();
          break;
      }

      if (gb.buttons.pressed(BUTTON_MENU)) {
        if (gameState == STATE_PLAYING)
          gameState = STATE_PAUSED;
      }
  }
}
