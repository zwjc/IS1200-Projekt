#include <stdint.h>  /* Declarations of uint_32 and the like */
#include <pic32mx.h> /* Declarations of system-specific addresses etc */
#include "mipslab.h" /* Declatations for these labs */
#include <stdio.h>
#include <stdlib.h>

// skarm = 32 pixlar höjd (y) 128 pixlar in bred (x)
uint8_t skarm[32][128];
// oled_skarm = 1D matris som hårdaran förstår
uint8_t oled_skarm[512];

int iHuvudmeny = 1;
int iSpel = 0;
int iSpellagen = 0;
int iToplista = 0;
int huvudmenyPekare = 0;
int spellagenPekare = 0;
int enMotEn = 1;
int spellageAI = 0;
int oandligt = 0;
int startaHuvudmeny = 1;
int startaSpellagen = 0;
int startaToplista = 0;
int startaSpel = 1;
int avslutaSpel = 0;

int oandligtPoang = 0;
int toplistaArray[3] = {0, 0, 0};
char topnamnArray[3][5] = {"    ","    ","    "};
char placering[3][13] = {"1:", "2:", "3:"};
char resultat[3][(sizeof(int) * 3) + 2];
char poangAppend[16];
char avstandIPoang[7] = "       ";

float spelareHojd = 10;
float spelareBredd = 4;
float spelareFart = 1.5;
int leds = 0xf;
float spelareAiFart = 0.60;
float spelareEttX = 0;
float spelareEttY = 32 / 2 - 5;
int spelareEttPoang = 0;
float spelareTvaX = 127 - 3;
float spelareTvaY = 32 / 2 - 5;
int spelareTvaPoang = 0;

float bollStorlek = 3;
float bollFartX = 1;
float bollFartY = 0;
float bollX = 128 / 2 - 2;
float bollY = 16;
float bollMaxFartX = 4;

float randomNummer = 0;

int timeoutcount = 0;

/* Interrupt Service Routine */
void user_isr(void)
{
  if(IFS(0) & 0x100)
  {
    timeoutcount++;
    IFSCLR(0) = 0x100;
    if(timeoutcount == 10)
    {
      timeoutcount = 0;
    }
  }
}

// Set the necessary settings before the program starts
void labinit(void)
{
  volatile int *trisE = (volatile int*)0xbf886100;
  TRISECLR = 0xff; // Leds set as output
  TRISDSET = 0xfe0; // bits 11 through 5 of port D are set as inputs
  TRISFSET = 0x2; // Set button 1 as input
  PORTE = 0x0; // LEDS
}

// We use the TIMER to get randomness
void faRandom()
{
  randomNummer = rand() % 5;
  randomNummer = randomNummer / 10;
  randomNummer = randomNummer * faRandomTecken();
}

int faRandomTecken()
{
  if(rand() % 2)
  {
    return -1;
  }
  return 1;
}

void arrayLasare(int input, int posX, int posY, int lanX, int lanY)
{
  int rad, kolumn = 0;
  for (rad = 0; rad <= 31; rad++)
  {
    for (kolumn = 0; kolumn <= 127; kolumn++)
    {
      switch (input)
      {
        case 1:
        if (kolumn >= posX && kolumn <= (posX + lanX))
        {
          if (rad >= posY && rad <= (posY + lanY))
          {
            skarm[rad][kolumn] = 1;
          }
        }
          break;
        case 2:
          skarm[rad][kolumn] = 0;
          break;
        default:
          break;
      }
    }
  }
}

//sätta pixel arrayen till det som matas in, detta görs med ettor och nollor
void startaPixelArray(int posX, int posY, int lanX, int lanY)
{
  arrayLasare(1, posX, posY, lanX, lanY);
}

// från vanliga pixel array till OLED skärmens array så att hårdvaran förstår
oledFixare()
{
  uint8_t binarVarde = 1;
  uint8_t oledNummer = 0;
  int sida = 0;
  int x = 0;
  int kolumn = 0;
  int rad = 0;
  while(sida <= 3)
  {
    for (kolumn = 0; kolumn <= 127; kolumn++)
    {
      binarVarde = 1;
      oledNummer = 0;
      for(rad = 0; rad <= 7; rad++)
      {
        if(skarm[8 * sida + rad][kolumn])
          oledNummer = oledNummer | binarVarde;
        binarVarde = binarVarde << 1;
      }
      if(oandligt && sida == 0)
      {
        if(kolumn % 8 == 0)
          x = textbuffer[sida][kolumn / 8];
        if(!(x & 0x80))
          oledNummer = oledNummer | font[x * 8 + kolumn % 8];
      }
      oled_skarm[kolumn + sida * 128] = oledNummer;
    }
    sida++;
  }
}

void startaOmSkarm()
{
  arrayLasare(2, 0, 0, 0, 0);
  int x = 0;
  while(x <= 511)
  {
    oled_skarm[x++] = 0;
  }
}

// uppdatera det som händer
uppdatera()
{
  int bollYKoll = (32 - bollStorlek);
  bollY = bollY + bollFartY;
  bollX = bollX + bollFartX;
  if(bollY <= 1 || bollY >= bollYKoll) //kolla om den nuddar taket och i sånna fall ändra vertikal riktning
    bollFartY *= -1;
}

// Omstart av spelet genom att resetta alla värden
startaOmSpel()
{
  spelareEttX = 0;
  spelareEttY = (32 / 2) - (spelareHojd / 2);

  spelareTvaX = 127 - spelareBredd;
  spelareTvaY = (32 / 2) - (spelareHojd / 2);

  bollX = (128 / 2) - 2;
  bollY = 16;

  startaOmSkarm();
  startaPixelArray(spelareEttX, spelareEttY, spelareBredd, spelareBredd);
  startaPixelArray(spelareTvaX, spelareTvaY, spelareBredd, spelareBredd);
  startaPixelArray(bollX, bollY, bollStorlek, bollStorlek);
  oledFixare();
  display_image(0, oled_skarm);
  startaSpel = 1;
}

//fina leds lyser :)
void ledAvslut()
{
  PORTE = 0xff;
  quicksleep(1 << 22);
}

char bokstav1;
char bokstav2;
char bokstav3;
char a[] = {'A', 0};
char b[] = {'A', 0};
char c[] = {'A', 0};
int bokstavPlats = 1;
int skrivNamnKlar = 0;

startaOmStringSkarm()
{
  int i = 0;
  for(i = 0; i <= 3; i++)
  {
    display_string(i, "\t");
  }
}

void skrivNamn(int btns) {
  quicksleep(1 << 19);
  switch(bokstavPlats)
  {
    case 1:
    display_string(0, a);
    display_string(1, b);
    display_string(2, c);

    if ((a[0] != 'Z') && (getbtns() & 0x2))
    {
      a[0]++;
    }
    if ((a[0] != 'A') && (getbtns() & 0x4))
    {
      a[0]--;
    }
    if (getbtns() & 0x1)
    {
      bokstav1 = a[0];
      quicksleep(1 << 19);
      startaOmStringSkarm();
      bokstavPlats = 2;
    }
      break;

    case 2:
    display_string(0, a);
    display_string(1, b);
    display_string(2, c);
     if ((getbtns() & 0x2) && (b[0] != 'Z'))
     {
       b[0]++;
     }
     if ((getbtns() & 0x4) && (b[0] != 'A'))
     {
       b[0]--;
     }

     if (getbtns() & 0x1)
     {
       bokstav2 = b[0];
       quicksleep(1 << 19);
       startaOmStringSkarm();
       bokstavPlats = 3;
      }
      break;

    case 3:
    display_string(0, a);
    display_string(1, b);
    display_string(2, c);
     if ((getbtns() & 0x2) && (c[0] != 'Z'))
     {
       c[0]++;
     }
    if ((getbtns() & 0x4) && (c[0] != 'A'))
    {
      c[0]--;
    }

    if (getbtns() & 0x1)
    {
      bokstav3 = c[0];
      bokstav1 = a[0];
      bokstav2 = b[0];
      quicksleep(1 << 19);
      startaOmStringSkarm();
      a[0] = 'A';
      b[0] = 'A';
      c[0] = 'A';
      spelareEttPoang = 0;
      bokstavPlats = 1;
      skrivNamnKlar = 1;
      }
        break;
  }
  display_update();
}

//loopas skriv namn och hoppa ut när de tre bokstäverna har skrivits in
anvandSkrivNamn()
{
  int u = 1;
  while(u = 1)
  {
    int btns = getbtns();
    quicksleep(1<<18);
    skrivNamn(btns);
    if(skrivNamnKlar == 1)
    {
      return;
    }
  }
}

void avsluta(void) //det som händer efter en match
{
  int i = 0;
  int x = 0;
  if(!leds)
  {
    for(i = 0; i <= 3; i++)
    {
      if(i == 1)
      {
        display_string(i, "Vanster vann :D ");
      }
      else
      display_string(i, "");
    }
    display_update();
    ledAvslut();
  }
  else if(leds == 0xff)
  {
    display_string(0, "");
    display_string(3, "");
    if(oandligt)
    {
      anvandSkrivNamn();
      skrivNamnKlar = 0;
      display_string(1, "   Poang:");
      if(oandligtPoang >= toplistaArray[0])
        strcat(poangAppend, " etta");  //strcat "concatinates" eller med andra ord "appendar"
      else if(oandligtPoang >= toplistaArray[1] && oandligtPoang < toplistaArray[0])
        strcat(poangAppend, " tvaa");
      else if(oandligtPoang >= toplistaArray[2] && oandligtPoang < toplistaArray[1])
        strcat(poangAppend, " trea");
      else //denna händer aldrig eftersom vi är så bra :)
        strcat(poangAppend, "  :)");
      display_string(2, poangAppend);
    }
    else
    {
      display_string(2, "");
      display_string(1, " Hoger vann :D  ");
    }
    display_update();
    ledAvslut();
  }
  if(oandligt)
  {
    if(toplistaArray[0] < oandligtPoang) //kolla om första plats
    {
      if(toplistaArray[0])
      {
        strcpy(placering[0], "1:");

        //här nedan flyttas allt ner en placering för att vi har en nya första plats
        strcpy(placering[1], "2:");
        snprintf(resultat[1], sizeof(resultat[1]), "%d", toplistaArray[0]); //skriver från toplistaArray[0] in i resultat[1]
        strcat(placering[1], resultat[1]);
        int l = 0;
        for(l = 0; l <= 3; l++)
        {
          snprintf(resultat[1], sizeof(resultat[1]), "%c", topnamnArray[0][l]);
          strcat(placering[1], resultat[1]);
        }

        strcpy(placering[2], "3:");
        snprintf(resultat[2], sizeof(resultat[2]), "%d", toplistaArray[1]);
        strcat(placering[2], resultat[2]);
        l = 0;
        for(l = 0; l <= 3; l++)
        {
          snprintf(resultat[2], sizeof(resultat[2]), "%c", topnamnArray[1][l]);
          strcat(placering[2], resultat[2]);
        }

        toplistaArray[2] = toplistaArray[1];
        strcpy(topnamnArray[2], topnamnArray[1]);
        toplistaArray[1] = toplistaArray[0];
        strcpy(topnamnArray[1], topnamnArray[0]);
      }
      //lägg till den nya ettan
      char array[5] = {' ', bokstav1, bokstav2, bokstav3, '\0'};
      snprintf(resultat[0], sizeof(resultat[0]), "%d", oandligtPoang);
      strcat(placering[0], resultat[0]);
      int l = 0;
      for(l = 0; l <= 3; l++)
      {
        snprintf(resultat[0], sizeof(resultat[0]), "%c", array[l]);
        strcat(placering[0], resultat[0]);
      }
      strcpy(topnamnArray[0], array);
      toplistaArray[0] = oandligtPoang;
    }
    //kolla andra plats
    else if(toplistaArray[1] < oandligtPoang)
    {
      if(toplistaArray[1])
      {
        //flytta ner placeringar
        strcpy(placering[1], "2:");

        strcpy(placering[2], "3:");
        snprintf(resultat[2], sizeof(resultat[2]), "%d", toplistaArray[1]);
        strcat(placering[2], resultat[2]);
        int d = 0;
        for(d = 0; d <= 3; d++)
        {
          snprintf(resultat[2], sizeof(resultat[2]), "%c", topnamnArray[1][d]);
          strcat(placering[2], resultat[2]);
        }
        toplistaArray[2] = toplistaArray[1];
        strcpy(topnamnArray[2], topnamnArray[1]);
      }
      //lägg till den nya tvåan
      char arrayTva[5] = {' ', bokstav1, bokstav2, bokstav3, '\0'};
      snprintf(resultat[1], sizeof(resultat[1]), "%d", oandligtPoang);
      strcat(placering[1], resultat[1]);
      int d = 0;
      for(d = 0; d <= 3; d++)
      {
        snprintf(resultat[1], sizeof(resultat[1]), "%c", arrayTva[d]);
        strcat(placering[1], resultat[1]);
      }
      strcpy(topnamnArray[1], arrayTva);
      toplistaArray[1] = oandligtPoang;
    }
    //kolla om tredje plats
    else if(toplistaArray[2] < oandligtPoang)
    {
      if(toplistaArray[2])
      {
        //sätt trean till 3: (så att den basically töms)
        strcpy(placering[2], "3:");
      }
      //lägg till den nya trean
      char arrayTre[5] = {' ' ,bokstav1, bokstav2, bokstav3, '\0'};
      snprintf(resultat[2], sizeof(resultat[2]), "%d", oandligtPoang);
      strcat(placering[2], resultat[2]);
      int r = 0;
      for(r = 0; r <= 3; r++)
      {
        snprintf(resultat[2], sizeof(resultat[2]), "%c", arrayTre[r]);
        strcat(placering[2], resultat[2]);
      }
      toplistaArray[2] = oandligtPoang;
    }
    oandligtPoang = 0;
  }
  startaHuvudmeny = 1;
  startaSpel = 1;
  iHuvudmeny = 1;
  iSpel = 0;
}

// Kolla om någon får poäng
void poang()
{
  if(bollX > (128 - bollStorlek))
  {
    leds = leds >> 1;
    spelareEttPoang = spelareEttPoang + 1;
  }
  if(bollX < 0)
  {
    leds = ((leds << 1) | 0x1);
    spelareTvaPoang = spelareTvaPoang + 1;
  }
  startaOmSpel();
  if(leds == 0x0 || leds == 0xff)
  {
    avsluta();
  }
}

int kollaOmNuddadSpelare1()
{
  if((spelareEttX < bollX) && (bollFartX < 0) && ((spelareEttY + spelareHojd) >= bollY) && (bollX <= (spelareEttX + spelareBredd)) && ((bollY + bollStorlek) > spelareEttY))
  {
    return 1;
  }
  return 0;
}

int kollaOmNuddadSpelare2()
{
  if((spelareTvaX < (bollX + bollStorlek)) && (bollFartX > 0) && ((spelareTvaY + spelareHojd) >= bollY) && ((bollX + bollStorlek) <= (spelareTvaX + spelareBredd)) && ((bollY + bollStorlek) > spelareTvaY))
  {
    return 1;
  }
  return 0;
}

//kolla om bollen nuddar någon spelare
void nuddaSpelare()
{
  //kolla om första spelaren nuddar bollen
  if(kollaOmNuddadSpelare1())
  {
    bollFartX = bollFartX * -1;

    if((bollFartY > -1.5) && ((bollY + (bollStorlek / 2)) < (spelareEttY + (spelareHojd / 2))))
    {
      if(bollFartX >= -2)
        bollFartY = bollFartY - 0.25;
      else
        bollFartY = bollFartY - 0.50;
    }
    else if((bollFartY < 1.5) && ((bollY + (bollStorlek / 2)) > (spelareEttY + (2 * (spelareHojd / 2)))))
    {
      if(bollFartX >= -2)
        bollFartY = bollFartY + 0.25;
      else
        bollFartY = bollFartY + 0.50;
    }

    if(bollFartX < bollMaxFartX)
    {
      bollFartX = bollFartX + 0.50;
    }

    if(oandligt)
    {
      oandligtPoang = oandligtPoang + 1;
    }
  }
  //kolla om andra spelaren nuddar bollen
  else if(kollaOmNuddadSpelare2())
  {
    bollFartX = bollFartX * -1;

    if((bollFartY > -1.5) && ((bollY + (bollStorlek / 2)) < (spelareTvaY + (spelareHojd / 2))))
    {
      if(bollFartX >= -2)
        bollFartY = bollFartY - 0.25;
      else
        bollFartY = bollFartY - 0.50;
    }
    else if((bollFartY < 1.5) && ((bollY + (bollStorlek / 2)) > (spelareTvaY + (2 * (spelareHojd / 2)))))
    {
      if(bollFartX >= -2)
        bollFartY = bollFartY + 0.25;
      else
        bollFartY = bollFartY + 0.50;
    }

    if(bollFartX > -bollMaxFartX)
    {
      bollFartX = bollFartX - 0.50;
    }
  }
  if((bollX + bollStorlek) < 0 || (bollX > 128))
  {
    quicksleep(1 << 15);
    poang();
  }
}

anvandSpelare() //kollar om man försöker röra på sin spelare med knapparna
{
  if((0 <= spelareEttY) && (getbtns() & 0x8))
  {
    spelareEttY = spelareEttY - spelareFart;
  }
  else if(((31 - spelareHojd) >= spelareEttY) && (getbtns() & 0x4))
  {
    spelareEttY = spelareEttY + spelareFart;
  }
  if(enMotEn)
  {
    if(spelareTvaY < (31 - spelareHojd) && (getbtns() & 0x1))
    {
      spelareTvaY = spelareTvaY + spelareFart;
    }
    else if((spelareTvaY > 0) && (getbtns() & 0x2))
    {
      spelareTvaY = spelareTvaY - spelareFart;
    }
  }
}

void anvandPekare(char* forsta, char* andra, char* tredje, char* fjarde, char* femte, char* sjatte, int tal)
{
  int pekare = 0; //den här behövs för att annars är C inte tillräckligt smart för att förstå att pekare kommer ha ett värde i switch(pekare)
  if(tal == 1)
  {
    pekare = huvudmenyPekare;
  }
  else
  {
    pekare = spellagenPekare;
  }
  switch(pekare)
  {
    case 0:
      display_string(1, forsta);
      display_string(2, andra);
      display_string(3, tredje);
      break;
    case 1:
      display_string(1, fjarde);
      display_string(2, femte);
      display_string(3, tredje);
      break;
    case 2:
      display_string(1, fjarde);
      display_string(2, andra);
      display_string(3, sjatte);
      break;
    default:
      break;
  }
}

void anvandPekareHuvudmeny(void)
{
  display_string(0, "   HUVUDMENY    ");
  anvandPekare(" --> Spela <--  ", "    Spellage    ", "    Toplista    ", "     Spela      ", "--> Spellage <--", "--> Toplista <--"  , 1);
  display_update();
}

// Användaren väljer i huvudmenyn vilken gamemode som ska väljas
void huvudmeny(void)
{
  quicksleep(1 << 20);
  if(getbtns() & 0x1)
  {
    switch(huvudmenyPekare)
    {
      case 0:
        iHuvudmeny = 0;
        huvudmenyPekare = 0;
        startaSpel = 1;
        iSpel = 1;
        leds = 0xf;
        break;
      case 1:
        iHuvudmeny = 0;
        huvudmenyPekare = 0;
        iSpellagen = 1;
        startaSpellagen = 1;
        break;
      case 2:
        iHuvudmeny = 0;
        huvudmenyPekare = 0;
        iToplista = 1;
        startaToplista = 1;
        break;
      default:
        break;
    }
  }
  else if((getbtns() & 0x2) && huvudmenyPekare != 2)
  {
    huvudmenyPekare += 1;
    anvandPekareHuvudmeny();
  }
  else if((getbtns() & 0x4) && huvudmenyPekare != 0)
  {
    huvudmenyPekare -= 1;
    anvandPekareHuvudmeny();
  }
}

//det man kan göra om man är i toplista
void toplista()
{
  quicksleep(1 << 20);
  if(getbtns() & 0x1)
  {
    iHuvudmeny = 1;
    iToplista = 0;
    startaHuvudmeny = 1;
  }
}

void anvandPekareSpellagen()
{
  display_string(0, "   SPELLAGEN    ");
  anvandPekare("  --> 1v1 <--   ", "       AI       ", "    Oandligt    ", "      1v1       ", "   --> AI <--   ", "--> Oandligt <--", 0);
  display_update();
}

// Spellagen bestämmer vilket spelläge som väljs beroende på vad spellagenPekare pekar på för instruktion och vad man klickar för knapp.
void spellagen()
{
  quicksleep(1 << 20);
  if(getbtns() & 0x1)
  {
    switch(spellagenPekare)
    {
      case 0:
        iHuvudmeny = 1;
        iSpellagen = 0;
        spellagenPekare = 0;
        enMotEn = 1;
        spellageAI = 0;
        oandligt = 0;
        startaHuvudmeny = 1;
        break;
      case 1:
        iHuvudmeny = 1;
        iSpellagen = 0;
        spellagenPekare = 0;
        enMotEn = 0;
        spellageAI = 1;
        oandligt = 0;
        startaHuvudmeny = 1;
        break;
      case 2:
        iHuvudmeny = 1;
        iSpellagen = 0;
        spellagenPekare = 0;
        enMotEn = 0;
        spellageAI = 0;
        oandligt = 1;
        startaHuvudmeny = 1;
        break;
      default:
        break;
    }
  }
  else if((getbtns() & 0x2) && spellagenPekare != 2)
  {
    spellagenPekare += 1;
    anvandPekareSpellagen();
  }
  else if((getbtns() & 0x4) && spellagenPekare != 0)
  {
    spellagenPekare -= 1;
    anvandPekareSpellagen();
  }
}

int kollaSwitch()
{
  if(getsw() & 0x1)
  {
    return 1;
  }
  else if(getsw() & 0x2)
  {
    return 2;
  }
  else if(getsw() & 0x4)
  {
    return 3;
  }
  else
  {
    return 4;
  }
}

// Välj hur svår AIn ska vara genom switches som ändrar på farten av AIn och hastigheten av bollen
void ai()
{
  if(spellageAI)
  {
    int x = kollaSwitch();
    switch(x)
    {
      case 1:
        bollMaxFartX = 4.2;
        spelareAiFart = 0.75;
      break;
      case 2:
        bollMaxFartX = 4.5;
        spelareAiFart = 1.0;
        break;
      case 3:
        bollMaxFartX = 5;
        spelareAiFart = 1.20;
        break;
      case 4:
        spelareAiFart = 0.75;
        break;
    }
  }
}

// Avsluta spelet med switch 4
void avslutaMedSwitch()
{
  if (iSpel && getsw() & 0x8)
  {
    startaOmSpel();
    avsluta();
  }
}

void labwork()
{
  quicksleep(1 << 15);
  if(iSpel)
  {
    startaOmStringSkarm();
    startaPixelArray(spelareEttX, spelareEttY, spelareBredd, spelareHojd);
    startaPixelArray(spelareTvaX, spelareTvaY, spelareBredd, spelareHojd);

    if(spellageAI)
    {
      ai();
      if(bollX <= (127 / 2))
      {
        if(14.8 > (spelareHojd / 2) + spelareTvaY)
        {
          spelareTvaY = spelareTvaY + 1;
        }
        if(15.2 < (spelareHojd / 2) + spelareTvaY)
        {
          spelareTvaY = spelareTvaY - 1;
        }
      }
      else
      {
        if((spelareTvaY < (31 - spelareHojd)) && ((spelareTvaY + (spelareHojd / 2)) < (bollY + (bollStorlek / 2))))
        {
          spelareTvaY = spelareTvaY + spelareAiFart;
        }
        else if((0 < spelareTvaY) && ((spelareTvaY + (spelareHojd / 2)) > (bollY + (bollStorlek / 2))))
        {
          spelareTvaY = spelareTvaY - spelareAiFart;
        }
      }
    }
    else if(oandligt)
    {
      spelareTvaY = bollY - (spelareHojd / 2);
      strcpy(poangAppend, avstandIPoang);
      strcat(poangAppend, itoaconv(oandligtPoang));
      display_string(0, poangAppend);
    }
    startaPixelArray(bollX, bollY, bollStorlek, bollStorlek);
    oledFixare();
    if(avslutaSpel == 0)
    {
      display_image(0, oled_skarm);
    }
    anvandSpelare();
    nuddaSpelare();
    uppdatera();
    PORTE = leds;
    avslutaMedSwitch();
    if(startaSpel != 0)
    {
      quicksleep(1 << 23);
      startaSpel = 0;
      faRandom();
      bollFartY = randomNummer;
      bollFartX = faRandomTecken();
    }
    startaOmSkarm();
  }
  else if(iSpellagen)
  {
    spellagen();
    if(startaSpellagen)
    {
      display_string(0, "   SPELLAGEN    ");
      display_string(1, "  --> 1v1 <--   ");
      display_string(2, "       AI       ");
      display_string(3, "    Oandligt    ");
      display_update();
      startaSpellagen = 0;
    }
  }
  else if(iToplista)
  {
    toplista();
    int i = 0;
    for(i = 0; i <= 3; i++)
    {
      if(i == 3)
        display_string(i, "-->Huvudmeny<-- ");
      else
        display_string(i, placering[i]);
    }
    display_update();
    startaToplista = 0;
  }
  else if(iHuvudmeny)
  {
    huvudmeny();
    if(startaHuvudmeny != 0)
    {
      PORTE = 0x0;
      display_string(0, "   HUVUDMENY    ");
      display_string(1, " --> Spela <--  ");
      display_string(2, "    Spellage    ");
      display_string(3, "    Toplista    ");
      display_update();
      startaHuvudmeny = 0;
    }
  }
}
