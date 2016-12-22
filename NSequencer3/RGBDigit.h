#ifndef RGB_DIGIT_H
#define RGB_DIGIT_H

int8_t Rrand = 10, Grand = 16, Brand = 22;
int8_t Rincr = 2, Gincr = 1, Bincr = 1; 

typedef enum {
  ZERO = 0,
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  SEVEN,
  EIGHT,
  NINE,
  LOWERC,
  UPPERC,
  UPPERA,
  LOWERB,
  LOWERD,
  LOWERE,
  UPPERE,
  UPPERF,
  UPPERG,
  UPPERH,
  LOWERH,
  UPPERI,
  UPPERL,
  UPPERM,
  LOWERN,
  LOWERO,
  UPPERP,
  LOWERR,
  UPPERS,
  LOWERT,
  UPPERU,
  UNDERSCORE,
  EQUAL,
  MINUS,
  OFF,
  RGB_CHAR_NUM,
} RGBCHARS;

//-------------Color array  8 RGBleds = 1 RGBSegments----------------------------------------------------------------------------------------------------------------------------------------------------
                 // a  b  c  d  e  f  g  h   
uint8_t Rarray[] = {0, 0, 0, 0, 0, 0, 0, 0, };
uint8_t Garray[] = {0, 0, 0, 0, 0, 0, 0, 0, };
uint8_t Barray[] = {0, 0, 0, 0, 0, 0, 0, 0, };

//------character table------------------------------------------------------------------------------

//            2-dimensional array of segment a,b,c,d,e,f,g,h data:   each character has its own colum
//                            0 1 2 3 4 5 6 7 8 9 c C A b d e E F G H h I L M n o P r S t U _ = - off
uint8_t  segmtarray [8][35] = { {1,0,1,1,0,1,0,1,1,1,1,1,1,0,0,1,1,1,1,0,0,0,0,1,0,0,1,0,1,0,0,0,0,0,0},    //a             a
                                {1,1,1,1,1,0,0,1,1,1,0,0,1,0,1,1,0,1,0,1,0,1,0,1,0,0,1,0,0,0,1,0,0,0,0},    //b          f     b
                                {1,1,0,1,1,1,1,1,1,1,0,0,1,1,1,0,0,0,1,1,1,1,0,1,1,1,0,0,1,0,1,0,0,0,0},    //c             g
                                {1,0,1,1,0,1,1,0,1,1,0,1,0,1,1,1,1,0,1,0,0,0,1,0,0,1,0,0,1,1,1,1,1,0,0},    //d          e     c
                                {1,0,1,0,0,0,1,0,1,0,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,0,0,0,0},    //e             d
                                {1,0,0,0,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,0,0,1,0,1,1,1,0,0,0,0},    //f
                                {0,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,0},    //g  
                                {0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,0,0,1,1,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0},};  //h   


inline uint8_t RANGE(int8_t x, uint8_t mi, uint8_t ma) {
  x = min(x, ma);
  x = max(x, mi);
  return x;
}

void updateRandCol(uint8_t segm) {

  switch(segm % 3) {
    case 0:
      if ( Grand > 24 || Grand < 4 ) Gincr = -Gincr;
      Grand = RANGE(Grand + Gincr, 0, 28);
      break;
    case 1:
      if ( Rrand > 24 || Rrand < 4 ) Rincr = -Rincr;
      Rrand = RANGE(Rrand + Rincr, 0, 28); 
      break;
    case 2:
      if ( Brand > 24 || Brand < 4 ) Bincr = -Bincr;
      Brand = RANGE(Brand + Bincr, 0, 28);
      break;

  }
 

  
  
/*
  Grand = RANGE((int8_t) Grand + random(-4,4), 0, 56);
  Rrand = RANGE((int8_t) Rrand + random(-4,4), 0, 56);
  Brand = RANGE((int8_t) Brand + random(-4,4), 0, 56);*/
}

void digitToRGBDigit(RGBCHARS d, uint8_t r, uint8_t g, uint8_t b){
 
  uint8_t segmnum = 0;
  uint8_t segmtloop = 0;

      while (segmtloop < 8) {
        if (segmtarray [segmtloop][d] == 1) {
	  updateRandCol(segmtloop);
          Garray[segmnum] = g + Grand;
          Rarray[segmnum] = r + Rrand;
          Barray[segmnum] = b + Brand;
/*    Serial.print("R: "); Serial.println(Rarray[segmnum]);
    Serial.print("G: "); Serial.println(Garray[segmnum]);
    Serial.print("B: "); Serial.println(Barray[segmnum]);
*/
          segmnum++;
        } else { 
          Garray[segmnum] = 0;
          Rarray[segmnum] = 0;
          Barray[segmnum] = 0;
          segmnum++;
        }
        segmtloop++ ; 
      }
} 

//-----------------------------------------------------------------------------
void programRGB(Adafruit_NeoPixel * RGB){
  uint8_t id = 0;
  while (id < 8) {
    RGB->setPixelColor(id, Rarray[id], Garray[id],  Barray[id]);
    id++; 
  } 
  RGB->show();
  delay(10);
}

#endif // RGB_DIGIT_H
