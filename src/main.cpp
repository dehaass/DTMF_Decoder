/*
 
 This code is a basic implementation of a DTMF decoder for detecting the 16 character
 DTMF code from the analog pin A0 and gives the decoded output by checking for all the 
 Upper and lower tones in the DTMF matrix and gives us the corresponding number by turning
 on the corresponding digital bit for the numbers 0-9 and by Serially printing the rest of
 the characters.
  This work is entirely based on the Kevin Banks code found at
  http://www.embedded.com/design/embedded/4024443/The-Goertzel-Algorithm
  so full credit to him for his generic implementation and breakdown. 
  The Goertzel algorithm is long standing so see 
  http://en.wikipedia.org/wiki/Goertzel_algorithm for a full description.
  It is often used in DTMF tone detection as an alternative to the Fast 
  Fourier Transform because it is quick with low overheard because it
  is only searching for a single frequency rather than showing the 
  occurrence of all frequencies.
  * THIS CODE IS Made/modified by "Mian Mohammad Shoaib" and  Released into the public domain.
  * for any querries related to the code feel free to ask at 
  :w
  
  MMSHOAIB8452@GMAIL.COM
 */


#include <Goertzel.h>

const int DELAY = false;

const int sensorPin = A0; 
const int LED_PIN = 2;
const int N = 100;                       //it is the number of samples code will take y0u can change for sensitivity and can if large it can slow the arduino
const float threshold = 2000;            //minimum tone amplitude to be considered we can change it for more senstivity
const float sampling_freq = 8900;        //maximum detectable frequency is the sampling rate/2 and arduino uno with 16Mhz can support sampling upto 8900 Hz
float x_frequencies[4];                  // make two arrays for holding x and y axis frequencies to be detected
float y_frequencies[4];

void setup(){
 
  Serial.begin(9600); 

  x_frequencies[0]=1209;                //just initialize the arrays with the x and y axis tone frequencies along with their row and colon number
  x_frequencies[1]=1336;
  x_frequencies[2]=1477;
  x_frequencies[3]=1633;

  y_frequencies[0]=697;
  y_frequencies[1]=770;
  y_frequencies[2]=852;
  y_frequencies[3]=941;
}

float measure_tone(float freq){

  Goertzel goertzel = Goertzel(freq, N, sampling_freq);        //initialize library function with the given sampling frequency no of samples and target freq
  goertzel.sample(sensorPin);                               //Will take n samples          
  float magnitude = goertzel.detect();                      //check them for target_freq

  return magnitude;
}

bool detect_tone(float freq){

  float magnitude = measure_tone(freq);

  if(magnitude>threshold){                                   //if you're getting false hits or no hits adjust the threshold
    digitalWrite(13,HIGH);                                    //blink led on 13 if a pulse is detected
    if(DELAY) delay(250);
    digitalWrite(13,LOW);
    Serial.print(freq);
    Serial.print("\n");
    return true;
  }
  else
    return false;
}


int find_number(int row, int column){
  int number=0;
  if(row==0){                                             //find the number corresponding to the found row and column
    if(column== 0)
    number= 1;
    else if(column== 1)  
    number= 2;
    else if(column== 2)
    number= 3;  
    else if(column== 3)
    number= 10;  
  }
  else if(row==1){
    if(column== 0)
    number= 4;  
    else if(column== 1)
    number= 5;    
    else if(column== 2)
    number= 6;    
    else if(column== 3)
    number= 11;    
  }
  else if(row==2){
    if(column== 0)
    number= 7;  
    else if(column== 1)
    number= 8;    
    else if(column== 2)
    number= 9;    
    else if(column== 3)  
    number= 12;  
  }
  else if(row==3){
    if(column== 0)
    number= 14;  
    else if(column== 1)
    number= 0;    
    else if(column== 2)
    number= 15;    
    else if(column== 3)
    number= 13;    
  }

  return number;
}


void print_number(int number){

  if(number <10){
  Serial.print(number);
  }

  else if(number ==10)
  Serial.print('A');
  else if(number ==11)
  Serial.print('B');
  else if(number ==12)
  Serial.print('C');
  else if(number ==13)
  Serial.print('D');
  else if(number ==14)
  Serial.print('*');
  else if(number ==15)
  Serial.print('#');
  Serial.print("\n");
  if(DELAY) delay(800);
}


void sort_by_magnitude(float val[4], int ranks[4]){

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

int find_max_from_spectrum(float val[4]){

  int max = 0;
  for(int i=1; i<4; i++){
    if(val[i] > val[max]) max = i;
  }

  return max;
}


void read_spectrum(float xval[4], float yval[4]){
  int i=0;

  for(i=0; i<4; i++){
    xval[i] = measure_tone(x_frequencies[i]);
    yval[i] = measure_tone(y_frequencies[i]);
  }
  return;
}

void decode_tones(int *row, int *column){

  float xval[4], yval[4];

  read_spectrum(xval, yval);
  *column = find_max_from_spectrum(xval);
  *row    = find_max_from_spectrum(yval);

}


void print_spectrum(float xval[4], float yval[4]){

  //int i = 0;
  char output[30] = "                           \n";

  int column = find_max_from_spectrum(xval);
  output[8+(column*5)] = '|';
  output[8+(column*5)+1] = '|';
  Serial.print(output);

  sprintf(output, "xvals: %4d %4d %4d %4d\n", int(xval[0]), int(xval[1]), int(xval[2]), int(xval[3]));
  Serial.print(output);
  sprintf(output, "yvals: %4d %4d %4d %4d\n", int(yval[0]), int(yval[1]), int(yval[2]), int(yval[3]));
  Serial.print(output);

  sprintf(output, "                           \n");
  int row = find_max_from_spectrum(yval);
  output[8+(row*5)] = '|';
  output[8+(row*5)+1] = '|';
  Serial.print(output);
  Serial.println();

  int ranks[4];
  sort_by_magnitude(xval, ranks);
  int minXDifference = xval[ranks[0]] - xval[ranks[1]];
  sort_by_magnitude(yval, ranks);
  int minYDifference = yval[ranks[0]] - yval[ranks[1]];
  int number = find_number(row, column);
  sprintf(output, "decoded number: %d\nminXDiff: %4d, minYDiff: %4d\n", number, minXDifference, minYDifference);
  Serial.print(output);
  Serial.println();
  Serial.println();

}

void calibrate(){
  float xval[4], yval[4];

  read_spectrum(xval, yval);

  print_spectrum(xval, yval);
  //Serial.println(xval[0]);

}

int decode_DTMF(){
  int number = 0;
  int row = 0;
  int column = 0;

  decode_tones(&row, &column);
  number = find_number(row,column);

  //int column=0,row=0;

  return number;
}

void loop(){

  //calibrate();
  //delay(3000);

  int number = decode_DTMF();
  print_number(number);

  if( number == 5){
    digitalWrite(LED_PIN, HIGH);
  }else{
    digitalWrite(LED_PIN, LOW);
  }
}