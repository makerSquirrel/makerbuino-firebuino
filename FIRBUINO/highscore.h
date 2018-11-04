#ifndef HIGHSCORE_H
#define HIGHSCORE_H
#include <Gamebuino-Meta.h>

template <typename HighscoreType, uint8_t nHighScoreEntries, uint8_t gamerNameLength, const char * const highScoreName> class HighScore
{
private:

static constexpr const char * const sc_name = highScoreName;
HighscoreType m_minHighScore;
HighscoreType m_highScores[nHighScoreEntries];
char m_highScoreNames[nHighScoreEntries][gamerNameLength];
Color m_textColor;
Color m_bgColor;
bool m_nameIsSet;

public:
  HighScore(Color textColor = WHITE, Color bgColor = BLACK) : m_minHighScore(0),
  m_textColor(textColor), m_bgColor(bgColor), m_nameIsSet(strlen(highScoreName) > 2 ? true : false) {}

  void setTextColor(const Color& newCol) { m_textColor = newCol; }
  void setBgColor(const Color& newCol) { m_bgColor = newCol; }

/// returns true if given value is a new highscore:
bool showScore(HighscoreType score);

/// simple check, returns true if new highscore
inline bool checkHighScore(HighscoreType score)
  { return score > m_minHighScore ? true : false; }

void updateHighscore(HighscoreType newHighScore); // writes highscore to SD Card, should not be called more often than needed (i.e. only if showScore returns true)

void drawHighScores(); // prints highscores to screen, but only valid entries if loadHighscores() was called before at least once.
};


template <typename HighscoreType, uint8_t nHighScoreEntries, uint8_t gamerNameLength, const char * const highScore>
bool HighScore<HighscoreType, nHighScoreEntries, gamerNameLength, highScore>::showScore(HighscoreType score)
{
  bool isHighScore = checkHighScore(score);
  int16_t maxWidth = 15*gb.display.getFontWidth() +2;
  int16_t leftPos = (gb.display.width() - maxWidth)/2;
    while (true) {
        if (!gb.update())
            continue;
        // gb.display.clear();
        gb.display.setColor(m_bgColor);
        gb.display.fillRect(leftPos, 2, maxWidth, 35);
        gb.display.setColor(m_textColor);
        gb.display.cursorY = 3;
        gb.display.cursorX = leftPos+1+2*gb.display.getFontWidth();
        gb.display.println(F("GAME OVER!"));
        gb.display.println(F(""));
        gb.display.cursorX = leftPos+1+2*gb.display.getFontWidth();
        gb.display.println(F("YOUR SCORE:"));
        gb.display.cursorX = leftPos + maxWidth/2 - 5;
        gb.display.println(score);
        gb.display.cursorX = leftPos+1;
        if (isHighScore)
            gb.display.println(F("NEW HIGHSCORE!"));
        if (gb.buttons.pressed(BUTTON_A) || gb.buttons.pressed(BUTTON_B) || gb.buttons.pressed(BUTTON_MENU))
            { break; }
    }
    return isHighScore;
}



template <typename HighscoreType, uint8_t nHighScoreEntries, uint8_t gamerNameLength, const char * const highScore>
void HighScore<HighscoreType, nHighScoreEntries, gamerNameLength, highScore>::updateHighscore(HighscoreType newHighScore)
{
  char playerName[gamerNameLength];
  gb.getDefaultName(playerName);
  gb.gui.keyboard("New Highscore!",playerName);
  strcpy(m_highScoreNames[nHighScoreEntries - 1],playerName);
    m_highScores[nHighScoreEntries - 1] = newHighScore;

    //Sort highscores
    for (byte i = nHighScoreEntries - 1; i > 0; i--) {
        if (m_highScores[i - 1] >= m_highScores[i])
            { break; }
        char tempName[gamerNameLength];
        strcpy(tempName, m_highScoreNames[i - 1]);
        strcpy(m_highScoreNames[i - 1], m_highScoreNames[i]);
        strcpy(m_highScoreNames[i], tempName);
        uint16_t tempScore;
        tempScore = m_highScores[i - 1];
        m_highScores[i - 1] = m_highScores[i];
        m_highScores[i] = tempScore;

    }
    //update minimum highscore
    m_minHighScore = m_highScores[nHighScoreEntries-1];
}


template <typename HighscoreType, uint8_t nHighScoreEntries, uint8_t gamerNameLength, const char * const highScore>
void HighScore<HighscoreType, nHighScoreEntries, gamerNameLength, highScore>::drawHighScores() {
  // gb.display.clear();
  while (true) {
    if (!gb.update())
        continue;
    gb.display.clear();
    gb.display.setColor(m_bgColor);
    gb.display.fillRect(0, 0, gb.display.width(), gb.display.height());
    gb.display.setColor(m_textColor);

    //Title
    gb.display.cursorX = 24;
    gb.display.cursorY = 3;
    // if (m_nameIsSet)
    gb.display.println(sc_name);
    gb.display.cursorX = 18;
    gb.display.println(F("HIGH SCORES"));

    gb.display.cursorY = gb.display.fontHeight * 3;
    int16_t minNameWidth =  3 + gb.display.getFontWidth() * gamerNameLength;
    for (byte i = 0; i < nHighScoreEntries; i++) {
        gb.display.cursorX = 3;

        //Name
        if (m_highScores[i] == 0)
            { gb.display.print('-'); }
        else
            { gb.display.print(m_highScoreNames[i]); }

        //Score
        HighscoreType currentScore = m_highScores[i];
        int16_t xPos = gb.display.width() - 3 - gb.display.getFontWidth();
        while ( currentScore > 10)
        {
            xPos -= gb.display.getFontWidth();
            currentScore /= 10;
        }
        gb.display.cursorX = xPos > minNameWidth ? xPos : minNameWidth;
        gb.display.cursorY = (gb.display.fontHeight * 3) + (gb.display.fontHeight * i);
        gb.display.println(m_highScores[i]);
    }

    if (gb.buttons.pressed(BUTTON_A) || gb.buttons.pressed(BUTTON_B) || gb.buttons.pressed(BUTTON_MENU)) {
        gb.sound.playOK();
        break;
    }
  }
}

#endif // HIGHSCORE_H
