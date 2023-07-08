/*
 
 This code is a basic implementation of a DTMF decoder for detecting the 16 character
 DTMF code from the analog pin A0 and gives the decoded output by checking for all the 
 Upper and lower tones in the DTMF matrix and gives us the corresponding number by turning
 on the corresponding digital bit for the numbers 0-9 and by Serially printing the rest of
 the characters.

  The Goertzel algorithm is long standing so see 
  http://en.wikipedia.org/wiki/Goertzel_algorithm for a full description.
  It is often used in DTMF tone detection as an alternative to the Fast 
  Fourier Transform because it is quick with low overheard because it
  is only searching for a single frequency rather than showing the 
  occurrence of all frequencies.

  Code based on a program made/modified by "Mian Mohammad Shoaib" and  Released into the public domain.
  Their work was in turn entirely based on the Kevin Banks code found at
  http://www.embedded.com/design/embedded/4024443/The-Goertzel-Algorithm
  so full credit to him for his generic implementation and breakdown. 
  
  This software was made/modified by Stuart de Haas, 2023-07-07

 */


#include <Goertzel.h>

const int sensorPin = A0; // Audio input
const int LED_PIN = 2;
const int N = 100;                       // Number of samples per measurement
const float sampling_freq = 8900;        //maximum detectable frequency is the sampling rate/2 and arduino uno with 16Mhz can support sampling up to 8900 Hz
const float threshhold = 2000;
float x_frequencies[4] = {1209, 1336, 1477, 1633}; // Defined by the DTMF standard 
float y_frequencies[4] = { 697,  770,  852,  941};
const int buffLen = 5;
int wordBuffer[buffLen] = {0,0,0,0,0};
int buffIndex = 0;
unsigned int lastBuffInsert = 0;
const unsigned int debounce = 1000; // ms

void setup(){
 
  Serial.begin(9600); 

}

// Takes in a frequency (Hz) and returns the detected magnitude
float measure_tone(float freq){

  Goertzel goertzel = Goertzel(freq, N, sampling_freq);        //initialize library function with the given sampling frequency no of samples and target freq
  goertzel.sample(sensorPin);                               //Will take n samples          
  float magnitude = goertzel.detect();                      //check them for target_freq

  return magnitude;
}


// Converts row and column info into a number from 0-15
int find_number(int row, int column){

  int number;

  if(row == -1 || column == -1) return -1; // No good read
  if (column == 3){
    number = 10+row;
  }
  if(row == 3){
    if(column == 0) number = 14;
    if(column == 1) number = 0;
    if(column == 2) number = 15;
  }else{
  number = row*3 + 1 + column;
  }
  return number;

}


// converts the raw number (from row, column) into the DTMF keypad symbols
// and prints to the serial monitor
void print_number(int number){

  if(number <10){
  Serial.print(number);
  }

  else if(number ==10) Serial.print('A');
  else if(number ==11) Serial.print('B');
  else if(number ==12) Serial.print('C');
  else if(number ==13) Serial.print('D');
  else if(number ==14) Serial.print('*');
  else if(number ==15) Serial.print('#');
  Serial.print("\n");
}


// Ranks the magnitude of the val items and stores the indexes in the ranks array in order
void rank_by_magnitude(float val[4], int ranks[4]){

  float localVal[4] = {val[0], val[1], val[2], val[3]};
  int max = 0;

  for(int j=0; j<4; j++){
    
    max = 0;
    for(int i=1; i<4; i++){
      if(localVal[i] > localVal[max]) max = i;
    }
    ranks[j] = max;
    localVal[max] = 0;
  }

  return;
}

// Finds the largest value from val[] and returns it's index
int find_max_from_spectrum(float val[4]){

  int max = 0;
  for(int i=1; i<4; i++){
    if(val[i] > val[max]) max = i;
  }

  return max;
}


// takes readings at all the desired DTMF frequencies and stores them in x,y arrays
void read_spectrum(float xval[4], float yval[4]){
  int i=0;

  for(i=0; i<4; i++){
    xval[i] = measure_tone(x_frequencies[i]);
    yval[i] = measure_tone(y_frequencies[i]);
  }
  return;
}

// Identifies the two most likely DTMF tones
void decode_tones(int *row, int *column){

  float xval[4], yval[4];

  read_spectrum(xval, yval);
  *column = find_max_from_spectrum(xval);
  *row    = find_max_from_spectrum(yval);

  if(xval[*column] < threshhold) *column = -1;
  if(yval[*row] < threshhold) *row = -1;

}


// Prints formated data to the serial port for calibration and debugging
void print_spectrum(float xval[4], float yval[4]){

  //int i = 0;
  char output[30] = "                           \n";
  int xranks[4];
  int yranks[4];

  rank_by_magnitude(xval, xranks);
  rank_by_magnitude(yval, yranks);

  //int column = find_max_from_spectrum(xval);
  output[8+(xranks[0]*5)] = '|';
  output[8+(xranks[0]*5)+1] = '|';
  Serial.print(output);

  sprintf(output, "xvals: %4d %4d %4d %4d\n", int(xval[0]), int(xval[1]), int(xval[2]), int(xval[3]));
  Serial.print(output);
  sprintf(output, "yvals: %4d %4d %4d %4d\n", int(yval[0]), int(yval[1]), int(yval[2]), int(yval[3]));
  Serial.print(output);

  sprintf(output, "                           \n");
  output[8+(yranks[0]*5)] = '|';
  output[8+(yranks[0]*5)+1] = '|';
  Serial.print(output);
  Serial.println();

  int minXDifference = xval[xranks[0]] - xval[xranks[1]];
  int minYDifference = yval[yranks[0]] - yval[yranks[1]];
  int number = find_number(yranks[0], xranks[0]);
  sprintf(output, "decoded number: %d\nminXDiff: %4d, minYDiff: %4d\n", number, minXDifference, minYDifference);
  Serial.print(output);
  Serial.println();
  Serial.println();

}

void calibrate(){
  float xval[4], yval[4];

  read_spectrum(xval, yval);
  print_spectrum(xval, yval);

}

int decode_DTMF(){
  int number = 0;
  int row = 0;
  int column = 0;

  decode_tones(&row, &column);
  number = find_number(row,column);

  return number;
}

int prevIndex(){
  return buffIndex <= 0 ? buffLen -1 : buffIndex - 1;
}

void read_buffer(){

}

void decode_word(){

 int number = decode_DTMF();
 if ( number == -1 || (wordBuffer[prevIndex()] == number && (lastBuffInsert + debounce) > millis()  )) return;
 print_number(number);
 //delay(500);

 wordBuffer[buffIndex] = number;
 lastBuffInsert = millis();
 
 if(wordBuffer[buffIndex] == 9 && wordBuffer[prevIndex()] == 6){
  Serial.println("Nice.");
 }

 buffIndex++;
 if(buffIndex >= buffLen) buffIndex = 0;

 return;
}

void loop(){

  // calibrate();
  // delay(1000);

  decode_word();

  // int number = decode_DTMF();
  // print_number(number);

  // if( number == 5){
  //   digitalWrite(LED_PIN, HIGH);
  // }else{
  //   digitalWrite(LED_PIN, LOW);
  // }
}