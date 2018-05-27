#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

/* change:
 *    - firePin
 *    - update rate for bullets and enemies
 *    - collision to be more accurate
 */

const int rightPin = 8, leftPin = 2, firePin = 10;     // the number of the pushbutton pin
const uint8_t player = 1, bullet = 8, enemy = 27;  // codul pt player, bullet si inamici
uint8_t bulletUpdateRate = 0, enemyUpdateRate = 0;  // la cate executii de loop sa updatam gloantele si inamicii pe ecran
uint8_t buttonStateRight = 0, buttonStateLeft = 0, buttonStateFire = 0; // checking if buttons are pressed or not
uint8_t xVal = 38, color = 0;  // pozitia( 38 - mijloc ) si culoarea player-ului
uint8_t gameState = 0; // verificam daca jucam sau e mesaju de end game
uint8_t score = 0, lives = 3; // scor adica cati omoram si vieti adica cate navete pot trece de noi pana pierdem


typedef struct NPCS
{
  // byte ca sa folosim mai putina memorie
  byte found; 
  byte posY;
  byte posX;
}npc;

npc bullets[3];
npc enemies[14]; // de schimbat in functie de cati incap pe un rand. Cred ca 2 randuri de inamici ajung  !!!!!

static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

void setup()
{
  Serial.begin(9600);
  pinMode(rightPin, INPUT);
  pinMode(leftPin, INPUT);
  pinMode(firePin, INPUT);
  
  display.begin();
  
  display.clearDisplay();
  display.setContrast(60);
  display.setTextSize(1);
  
  drawPlayer();

  // initializam structura de gloante . Nuj sincer daca trebe but just to be sure
  uint8_t i;
  for(i=0; i<3; i++)
  {
    bullets[i].found = 0;
    enemies[i].found = 1;
  }

 // de initializat pozitiile initiale ale inamicilor  !!!!!!!
}


void loop() 
{ 
  if(gameState == 0)
  {
    bulletUpdateRate++;
    enemyUpdateRate++;
    
    checkPlayer();
    if(bulletUpdateRate == 10)
     {
      bulletUpdateRate = 0;
      checkBullets();
     }
    if(enemyUpdateRate == 20)
    {
      enemyUpdateRate = 0;
      checkEnemies();
    }
    checkCollisions();
    checkEndGame();
  }
  else
    drawEndGameScreen();
}

void drawEndGameScreen()
{
  // de facut !!!!!
  // scriem pe ecran "GAME OVER! SCORE: val"
}

void checkEndGame()
{
  if(lives == 0)
    gameState = 1;
}

void checkCollisions()
{
  uint8_t enemyCount, bulletCount, enemyFound = 0;

  for(enemyCount = 0; enemyCount < 14; enemyCount++)
    if(enemies[enemyCount].found != 0)
    {
      enemyFound++;
      for(bulletCount = 0; bulletCount < 3; bulletCount++)
        if(bullets[bulletCount].found != 0)
          if(enemies[enemyCount].posX == bullets[bulletCount].posX && enemies[enemyCount].posY == bullets[bulletCount].posY)
          {
            // verificam pentru fiecare glont si naveta care exista daca pozitiile lor coincid si daca da le stergem desenand cu alb
            bullets[bulletCount].found = enemies[enemyCount].found = 0;
            color = 1;
            drawBullet(bullets[bulletCount].posY,bullets[bulletCount].posX);
            drawEnemy(enemies[enemyCount].posY,enemies[enemyCount].posX);
            color = 0;

            score += 2; // crestem scoru cu 2 puncte pentru fiecare kill pentru ca de aia :))
          }

      if(enemies[enemyCount].posX == xVal && enemies[enemyCount].posY == 40)
      {
        // verificam daca pozitia fiecare nave care exista coincide cu pozitia de la player
        // si daca da stergem nava si scadem o viata si redesenam playeru la aceleasi coordonate
        enemies[enemyCount].found = 0;
        color = 1;
        drawEnemy(enemies[enemyCount].posY,enemies[enemyCount].posX);
        lives--;

        color = 0;
        drawPlayer();
      }
    }

  // daca nu mai sunt inamici inseamna ca am castigat si schimbam starea jocului
  if(enemyFound == 0)
    gameState = 1;
}

void checkEnemies()
{
  uint8_t i = 0;

  // verificam daca inamicii au ajuns jos pe ecran si daca da ii stergem si scadem o viata. Daca nu le modificam pozitia

  for(i=0; i<14; i++)
    if(enemies[i].found != 0)
    {
      color = 1;
      drawEnemy(enemies[i].posY,enemies[i].posX);

      if(enemies[i].posY < 42)
      {
        enemies[i].posY++;
        drawEnemy(enemies[i].posY,enemies[i].posX);
      }
      else
      {
        enemies[i].found = 0;
        lives--;
      }
    }
}

void checkBullets()
{
  uint8_t foundBullet = 0, i;

  for(i=0; i<3; i++)
    if(bullets[i].found != 0)
    {
      // verificam cele 3 gloante si daca gasim unul tras retinem ca l-am gasit si il desenam alb
      foundBullet++;
      color = 1;
      drawBullet(bullets[i].posY, bullets[i].posX);

      // si apoi daca inca nu iese din ecran il redesenam la noua pozitie cu negru
      // daca iese nu mai desenam nimic si il punem ca glont netras inca
      if(bullets[i].posY > 0)
      {
        bullets[i].posY--;
        color = 0;
        drawBullet(bullets[i].posY, bullets[i].posX);
      }
      else bullets[i].found = 0;

      // de verificat coliziunea cu o naveta
    }

  if(buttonStateFire == HIGH && foundBullet < 3)
    // daca tragem si mai avem loc de gloante
    for(i=0; i<3; i++)
      if(bullets[i].found == 0)
      {
        // luam prima pozitie de glont disponibila si retinem ca l-am tras si ii punem coordonatele
        bullets[i].found = 1;
        bullets[i].posX = xVal;
        bullets[i].posY = 35; // de schimbat valoarea asta cu una care se potriveste  !!!!!!!!
        drawBullet(bullets[i].posY, bullets[i].posX);
        break;
      }
}

void checkPlayer()
{
  buttonStateRight = digitalRead(rightPin);
  buttonStateLeft = digitalRead(leftPin);
  
  if(buttonStateRight == HIGH)
  {
    color = 1;
    drawPlayer();
    
    if(xVal < 77)
      xVal++;
      
    color = 0;
    drawPlayer();  
  } 

  if(buttonStateLeft == HIGH)
  {
    color = 1;
    drawPlayer();
    
    if(xVal > 1)
      xVal--;

    color = 0;
    drawPlayer();     
  }
}

void drawEnemy(uint8_t posY, uint8_t posX)
{
  if(color == 0)
     display.setTextColor(BLACK);
  else 
     display.setTextColor(WHITE);
  
  display.setCursor(posY,posX);
  display.write(enemy);
  display.display();
}

void drawBullet(uint8_t posY, uint8_t posX)
{
  if(color == 0)
     display.setTextColor(BLACK);
  else 
     display.setTextColor(WHITE);
  
  display.setCursor(posY,posX);
  display.write(bullet);
  display.display();
}


void drawPlayer()
{
  if(color == 0)
     display.setTextColor(BLACK);
  else 
     display.setTextColor(WHITE);
  
  display.setCursor(xVal,40);
  display.write(player);
  display.display();
}