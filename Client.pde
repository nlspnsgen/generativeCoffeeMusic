import ddf.minim.*;
import ddf.minim.analysis.*;
import ddf.minim.effects.*;
import ddf.minim.signals.*;
import ddf.minim.spi.*;
import ddf.minim.ugens.*;

int timer;
int melodyTimer;
int harmonicsTimer;
int temperature;
Minim minim;
AudioOutput out;
Oscil wave;
Oscil wave2;
Delay myDelay;
Wavetable table;
int oddHarmonics;
Flanger flange;
int threshholdTemp = 50 ;


boolean coffeeIsDone = false;
boolean coffeeStopped = false;

//Melody will be played in just intonation
float[] pitches = {272.54, 313.96, 367.92, 418.60, 470.93};
float amplitude = 0.0;

void setup() {
    oddHarmonics = 1;
    table = Waves.randomNHarms(oddHarmonics);
    minim = new Minim(this);
    out = minim.getLineOut();
    wave = new Oscil(pitches[(int) random(4)], amplitude, table);
    myDelay = new Delay( 0.6, 0.5, true, true );
   flange = new Flanger( 1,     // delay length in milliseconds ( clamped to [0,100] )
                        0.2f,   // lfo rate in Hz ( clamped at low end to 0.001 )
                        1,     // delay depth in milliseconds ( minimum of 0 )
                        0.5f,   // amount of feedback ( clamped to [0,1] )
                        0.5f,   // amount of dry signal ( clamped to [0,1] )
                        0.5f    // amount of wet signal ( clamped to [0,1] )
                       );
    wave.patch(flange).patch(myDelay).patch(out);

}

void draw() {
    //Refresh only every 10 seconds
    if (millis() - timer >= 1000) {
      temperature = getTemperature();
      println(temperature);
      if(temperature > threshholdTemp) coffeeIsDone = true;
      if(temperature < threshholdTemp && coffeeIsDone) coffeeStopped = true;
      timer = millis();
  }
  if (coffeeIsDone && !coffeeStopped){
    playMelody();
  }
  
  if (coffeeStopped){
    stopMelody();
  }
}

//Loads html site provided by arduino and parses temperature from it
int getTemperature(){
   String[] htmlLines = loadStrings("http://192.168.1.20/pyroSensor");
   String[] temperatureStrings = match(htmlLines[2], "[0-9]*.[0-9][0-9]");
   return Integer.parseInt(temperatureStrings[0].trim());
}

void playMelody(){
   wave.setAmplitude(0.1);     
   if (millis() - melodyTimer >= ((int) random(2000)+ 1200)) {
     addOddHarmonics();
     wave.setFrequency(pitches[(int) random(4)]);
     melodyTimer = millis();
  }
}

void stopMelody(){
  wave.setAmplitude(0.0);
}

void addOddHarmonics(){
  if (oddHarmonics < 24) {
      if (millis() - harmonicsTimer >= 15000){
      oddHarmonics += 1;
      table = Waves.randomNHarms(oddHarmonics);
      wave.setWaveform(table);
      harmonicsTimer = millis();
    }
  }
}