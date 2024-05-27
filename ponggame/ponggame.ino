#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int player1 = A1; // analog pin connected to Y output of player 1 joystick
const int player2 = A0;
const int player1Button = 7; // digital pin connected to the button of player 1 joystick
const int player2Button = 8; // digital pin connected to the button of player 2 joystick
const int selectButton = 11; // digital pin connected to the select button
const int ledPlayer1 = 9; // digital pin connected to Player 1 LED
const int ledPlayer2 = 10; // digital pin connected to Player 2 LED
const int buzzer = 12; // digital pin connected to the buzzer

const unsigned long PADDLE_RATE = 200; // Slower paddle movement
const unsigned long BALL_RATE = 500; // Slower ball movement
const uint8_t PADDLE_HEIGHT = 1; // Adjusted for LCD
int player1Score = 0;
int player2Score = 0;
int player1RoundsWon = 0;
int player2RoundsWon = 0;
int lastPlayer1Score = -1; // Store last score to avoid unnecessary updates
int lastPlayer2Score = -1; // Store last score to avoid unnecessary updates
int lastPlayer1RoundsWon = -1;
int lastPlayer2RoundsWon = -1;
int lastMinutes = -1;
int lastSeconds = -1;
int maxScore = 3; // Game ends at 3 points per round
bool resetBall = false;
bool ballLaunched = false;
bool singlePlayerMode = false; // To store the selected mode
#define SCREEN_WIDTH 16 // LCD display width, in characters
#define SCREEN_HEIGHT 2 // LCD display height, in characters

LiquidCrystal_I2C lcd1(0x27, SCREEN_WIDTH, SCREEN_HEIGHT); // Game display
LiquidCrystal_I2C lcd2(0x3F, SCREEN_WIDTH, SCREEN_HEIGHT); // Score and time display

void drawScore();
void showBallPossession(uint8_t player);
void showStartMessage();
void ledShowAndPlayMelody();
void waitForRestart();
void resetGame();
void resetRound();
void gameOver();
void playWinningMelody();
void selectMode();
void displayModeMenu(int selectedMode);
void waitForModeSelection();
void movePlayer2Automatically();
void launchBallAutomatically(uint8_t player);

// Custom character for the ball
byte ballChar[8] = {
  0b00000,
  0b00110,
  0b01111,
  0b01111,
  0b01111,
  0b01111,
  0b00110,
  0b00000
};

uint8_t ball_x, ball_y; // Adjusted for LCD
int8_t ball_dir_x, ball_dir_y;
unsigned long ball_update;

unsigned long paddle_update;
const uint8_t PLAYER1_X = 1; // Adjusted for LCD
uint8_t player1_y = 1;

const uint8_t PLAYER2_X = 14; // Adjusted for LCD
uint8_t player2_y = 1;

unsigned long startTime;
unsigned long lastTimeUpdate = 0; // Last time the time display was updated
uint8_t currentPlayer = 1;
uint8_t serveCount = 0;

void setup() {
  lcd1.init();
  lcd1.backlight();
  lcd2.init();
  lcd2.backlight();

  pinMode(player1Button, INPUT_PULLUP);
  pinMode(player2Button, INPUT_PULLUP);
  pinMode(selectButton, INPUT_PULLUP);
  pinMode(ledPlayer1, OUTPUT);
  pinMode(ledPlayer2, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // Create custom ball character
  lcd1.createChar(0, ballChar);

  waitForModeSelection(); // Show mode selection menu

  waitForRestart(); // Show message to start the game

  startTime = millis();
  drawScore();
  showBallPossession(currentPlayer);
  ball_update = millis();
  paddle_update = ball_update;
}

void loop() {
  bool update = false;
  unsigned long time = millis();

  if (resetBall) {
    if (player1Score == maxScore || player2Score == maxScore) {
      if (player1Score == maxScore) {
        player1RoundsWon++;
      } else {
        player2RoundsWon++;
      }

      if (player1RoundsWon == 2 || player2RoundsWon == 2) {
        ledShowAndPlayMelody();
        gameOver();
        waitForModeSelection(); // Select mode after each game
        waitForRestart(); // Show message to start the game
      } else {
        player1Score = 0;
        player2Score = 0;
        waitForRestart(); // Show message to start the next round
      }
    } else {
      lcd1.clear();
      drawScore();
      ball_x = (currentPlayer == 1) ? PLAYER1_X + 1 : PLAYER2_X - 1; // Start near the current player's paddle
      ball_y = (currentPlayer == 1) ? player1_y : player2_y;
      ballLaunched = false;
      showBallPossession(currentPlayer);
      resetBall = false;
    }
  }

  if ((currentPlayer == 1 && digitalRead(player1Button) == LOW) ||
      (currentPlayer == 2 && digitalRead(player2Button) == LOW) || 
      (singlePlayerMode && currentPlayer == 2)) {
    if (!ballLaunched) {
      ball_dir_x = (currentPlayer == 1) ? 1 : -1; // Launch towards the opponent
      ball_dir_y = (random(0, 2) == 0) ? -1 : 1; // Launch diagonally either up or down
      if (ball_dir_y == 0) { // Ensure it's never 0
        ball_dir_y = 1;
      }
      ballLaunched = true;
      lcd1.clear();
    }
  }

  if (ballLaunched && time > ball_update) {
    uint8_t new_x = ball_x + ball_dir_x;
    uint8_t new_y = ball_y + ball_dir_y;

    // Ensure the ball always moves diagonally
    if (ball_dir_y == 0) {
      ball_dir_y = (random(0, 2) == 0) ? -1 : 1;
    }

    // Check if we hit the vertical walls
    if (new_x == 0 || new_x == 15) { // Adjusted for LCD
      if (new_x == 0) {
        player2Score += 1; // Player 2 gains a point
        currentPlayer = 1; // Player 1 serves next
        serveCount++;
        lcd1.clear();
        resetBall = true;
        digitalWrite(ledPlayer2, HIGH); // Light up Player 2 LED
        delay(1000);
        digitalWrite(ledPlayer2, LOW);
      } else if (new_x == 15) { // Adjusted for LCD
        player1Score += 1; // Player 1 gains a point
        currentPlayer = 2; // Player 2 serves next
        serveCount++;
        lcd1.clear();
        resetBall = true;
        digitalWrite(ledPlayer1, HIGH); // Light up Player 1 LED
        delay(1000);
        digitalWrite(ledPlayer1, LOW);
      }
      ball_dir_x = -ball_dir_x;
    }

    // Check if we hit the horizontal walls
    if (new_y == 0 || new_y == 1) { // Adjusted for LCD
      ball_dir_y = -ball_dir_y;
    }

    // Check if we hit the player 1 paddle
    if (new_x == PLAYER1_X + 1 && new_y == player1_y) { // Adjusted for LCD
      ball_dir_x = 1;
      if (ball_dir_y == 0) ball_dir_y = (random(0, 2) == 0) ? -1 : 1; // Ensure diagonal movement
    }

    // Check if we hit the player 2 paddle
    if (new_x == PLAYER2_X - 1 && new_y == player2_y) { // Adjusted for LCD
      ball_dir_x = -1;
      if (ball_dir_y == 0) ball_dir_y = (random(0, 2) == 0) ? -1 : 1; // Ensure diagonal movement
    }

    lcd1.setCursor(ball_x, ball_y);
    lcd1.print(' '); // Clear previous position
    lcd1.setCursor(new_x, new_y);
    lcd1.write(byte(0)); // Draw new position with custom ball character
    ball_x = new_x;
    ball_y = new_y;

    ball_update = time + BALL_RATE; // Set the next ball update time
    update = true;
  }

  if (time > paddle_update) {
    paddle_update = time + PADDLE_RATE;

    // Player 1 paddle
    lcd1.setCursor(PLAYER1_X, player1_y);
    lcd1.print(' '); // Clear previous position
    if (analogRead(player1) < 475) { // Reversed joystick controls
      player1_y = 1;
    }
    if (analogRead(player1) > 550) { // Reversed joystick controls
      player1_y = 0;
    }
    lcd1.setCursor(PLAYER1_X, player1_y);
    lcd1.print('|'); // Draw paddle

    if (!singlePlayerMode) {
      // Player 2 paddle
      lcd1.setCursor(PLAYER2_X, player2_y);
      lcd1.print(' '); // Clear previous position
      if (analogRead(player2) < 475) {
        player2_y = 0;
      }
      if (analogRead(player2) > 550) {
        player2_y = 1;
      }
      lcd1.setCursor(PLAYER2_X, player2_y);
      lcd1.print('|'); // Draw paddle
    } else {
      movePlayer2Automatically();
    }
  }
  update = true;

  if (update) {
    drawScore();
  }
}

void drawScore() {
  // Calculate elapsed time
  unsigned long elapsedTime = (millis() - startTime) / 1000;
  int minutes = elapsedTime / 60;
  int seconds = elapsedTime % 60;

  // Update the score display only if there is a change
  if (player1Score != lastPlayer1Score) {
    lcd2.setCursor(0, 0);
    lcd2.print("P1:");
    lcd2.print(player1Score);
    lcd2.print("  ");
    lastPlayer1Score = player1Score;
  }

  if (player2Score != lastPlayer2Score) {
    lcd2.setCursor(5, 0);
    lcd2.print("P2:");
    lcd2.print(player2Score);
    lcd2.print("  ");
    lastPlayer2Score = player2Score;
  }

  // Update the timer display only if there is a change
  if (minutes != lastMinutes || seconds != lastSeconds) {
    lcd2.setCursor(11, 0);
    if (minutes < 10) lcd2.print('0');
    lcd2.print(minutes);
    lcd2.print(':');
    if (seconds < 10) lcd2.print('0');
    lcd2.print(seconds);
    lcd2.print(" ");
    lastMinutes = minutes;
    lastSeconds = seconds;
  }

  // Update the rounds won display only if there is a change
  if (player1RoundsWon != lastPlayer1RoundsWon) {
    lcd2.setCursor(0, 1);
    lcd2.print("R1:");
    lcd2.print(player1RoundsWon);
    lcd2.print("  ");
    lastPlayer1RoundsWon = player1RoundsWon;
  }

  if (player2RoundsWon != lastPlayer2RoundsWon) {
    lcd2.setCursor(8, 1);
    lcd2.print("R2:");
    lcd2.print(player2RoundsWon);
    lcd2.print("  ");
    lastPlayer2RoundsWon = player2RoundsWon;
  }
}

void showStartMessage() {
  lcd1.clear();
  lcd1.setCursor(3, 0);
  lcd1.print("Pong Game");
  lcd1.setCursor(2, 1);
  lcd1.print("Ready to Play!");
  delay(3000); // Display message for 3 seconds
  lcd1.clear();
}

void showBallPossession(uint8_t player) {
  lcd1.clear();
  if (player == 1) {
    lcd1.setCursor(2, 0);
    lcd1.print("Player 1");
    lcd1.setCursor(2, 1);
    lcd1.print("has the ball");
  } else {
    lcd1.setCursor(2, 0);
    lcd1.print("Player 2");
    lcd1.setCursor(2, 1);
    lcd1.print("has the ball");
  }
  delay(2000); // Display message for 2 seconds
  lcd1.clear();
}

void ledShowAndPlayMelody() {
  int melody[] = {262, 294, 330, 349, 392, 440, 494, 523, 587, 659, 698, 784}; // C4 to G5 notes
  int noteDurations[] = {4, 8, 8, 4, 4, 8, 8, 4, 4, 8, 8, 4}; // Different note durations

  for (int thisNote = 0; thisNote < 12; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(buzzer, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;

    digitalWrite(ledPlayer1, HIGH);
    digitalWrite(ledPlayer2, HIGH);
    delay(noteDuration);
    digitalWrite(ledPlayer1, LOW);
    digitalWrite(ledPlayer2, LOW);

    delay(pauseBetweenNotes - noteDuration);
  }
}

void waitForRestart() {
  lcd1.clear();
  lcd1.setCursor(0, 0);
  lcd1.print("Press button to");
  lcd1.setCursor(2, 1);
  lcd1.print("play again");
  while (digitalRead(selectButton) == HIGH) {
    // Wait for the restart button to be pressed
  }
  resetRound();
}

void resetRound() {
  player2Score = player1Score = 0;
  startTime = millis();
  ballLaunched = false;
  serveCount = 0;
  currentPlayer = 1;
  ball_update = millis();
  paddle_update = ball_update;
  resetBall = true;
  showStartMessage();
}

void resetGame() {
  player2Score = player1Score = 0;
  player2RoundsWon = player1RoundsWon = 0;
  startTime = millis();
  ballLaunched = false;
  serveCount = 0;
  currentPlayer = 1;
  ball_update = millis();
  paddle_update = ball_update;
  resetBall = true;
  showStartMessage();
}

void gameOver() {
  lcd1.clear();
  if (player1RoundsWon > player2RoundsWon) {
    lcd1.setCursor(4, 0);
    lcd1.print("Player 1");
    lcd1.setCursor(5, 1);
    lcd1.print("won");
  } else {
    lcd1.setCursor(4, 0);
    lcd1.print("Player 2");
    lcd1.setCursor(5, 1);
    lcd1.print("won");
  }
  delay(2000);
}

void selectMode() {
  int selectedMode = 0; // 0 for Single Player, 1 for Multiplayer
  displayModeMenu(selectedMode);

  while (true) {
    if (analogRead(player1) < 475) { // Reversed joystick controls
      selectedMode = 0; // Single Player
      displayModeMenu(selectedMode);
      delay(300); // Debounce delay
    }
    if (analogRead(player1) > 550) { // Reversed joystick controls
      selectedMode = 1; // Multiplayer
      displayModeMenu(selectedMode);
      delay(300); // Debounce delay
    }
    if (digitalRead(selectButton) == LOW) {
      singlePlayerMode = (selectedMode == 0);
      break;
    }
  }
}

void displayModeMenu(int selectedMode) {
  lcd2.clear();
  lcd2.setCursor(0, 0);
  if (selectedMode == 0) {
    lcd2.print("> Single Player");
  } else {
    lcd2.print("  Single Player");
  }

  lcd2.setCursor(0, 1);
  if (selectedMode == 1) {
    lcd2.print("> Multiplayer");
  } else {
    lcd2.print("  Multiplayer");
  }
}

void waitForModeSelection() {
  lcd1.clear();
  lcd1.setCursor(0, 0);
  lcd1.print("Check the other");
  lcd1.setCursor(0, 1);
  lcd1.print("display");
  selectMode();
  lcd2.clear();
}

void movePlayer2Automatically() {
  lcd1.setCursor(PLAYER2_X, player2_y);
  lcd1.print(' '); // Clear previous position

  // Move player 2 paddle to the position before the ball reaches it
  if (ball_x == PLAYER2_X - 2) { // One position before the ball reaches the paddle
    if (ball_y < player2_y) {
      player2_y = 0;
    } else if (ball_y > player2_y) {
      player2_y = 1;
    }
  }

  lcd1.setCursor(PLAYER2_X, player2_y);
  lcd1.print('|'); // Draw paddle
}
