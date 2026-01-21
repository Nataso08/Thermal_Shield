#include "mylibrary.h"

/*
PWM Arduino UNO
Pin   | Frequenza   | Timer   | Modalità
3       488 Hz        2         Fast PWM / Phase-correct
5       976 Hz        0         Fast PWM
6       976 Hz        0         Fast PWM
9       488 Hz        1         Fast PWM
10      488 Hz        1         Fast PWM  
11      488 Hz        2         Fast PWM / Phase-correct

Prescaler = 64 per tutti, di default

Definizioni di modalità:
- Fast PWM:
  0 -> TOP (salita) -> 0 (azzero) -> TOP ...
  f_PWM = (16 * 10^6) / (64 * 256) = 976 Hz
- Phase-correct
  0 -> TOP (salita) -> 0 (discesa) -> TOP ...
  f_PWM = (16 * 10^6) / (64 * 2 * 256) = 488 Hz
*/

float T, T_last, T_med, T_max = 35, T_min = 30;      // range di valori nel quale deve essere mantenuta la temperatura

short T_value;      // valore temperatura analogica (0-1023)
short PWM_resistenza, PWM_ventola;    // valore da applicare a pin PWM

// pulsanti START e STOP basati sulla libreria dello scorso anno (debounce e fronti con OOP)
Pulsante Start (4, INPUT_PULLUP, 'n');
Pulsante Stop (5, INPUT_PULLUP, 'n');

#define pinResistenza 10
#define pinVentola 11
#define pinTemp A0

// variabili di stato e definizione stati
char stato, stato_old;
#define ATTESA 0
#define SCALDA 1
#define RAFFREDDA 2

// dichiarazioni funzioni per macchina a stati (Moore)
void FTS ();
void FTU ();
void leggiIngressi ();
void scriviUscite ();

char lettura = '\n';

void setup() {
  // inizializzazione dei pulsanti
  Start.init();
  Stop.init();

  DDRB |= ((1<<PB2) | (1<<PB3));       // setup pin 10 e 11 in OUTPUT tramite registri
  DDRC &= ~(1<<PC0);                   // setup pin A0 in INPUT tramite registri

  // reset variabili di stato
  stato = ATTESA;
  stato_old = 3;

  // analogReference(DEFAULT);           // riferimento default 5 V
  analogReference(INTERNAL);          // riferimento interno 1.1 V
  // analogReference(EXTERNAL);       // riferimento esterno AREF (es. 3.3 V)

  Serial.begin(9600);     // inizializzazione seriale
}

void loop() {
  leggiIngressi ();
  FTS ();
  FTU ();
  scriviUscite ();
}


void FTS () {
  // aggiornamento stato_old
  stato_old = stato;

  // stato = ATTESA se STOP
  if (Stop.f_UP || lettura == '0')
    stato = ATTESA;

  // FTS con controllo temperature (mantenimento tra T_min e T_max)
  switch (stato) {
    case SCALDA:
      if (T_med >= T_max) 
        stato = RAFFREDDA;
      break;
    case RAFFREDDA:
      if (T_med <= T_min) 
        stato = SCALDA;
      break;
    case ATTESA:
    default:
      if (Start.f_UP || lettura == '1')
        stato = SCALDA;
      break;
  }
}

void FTU () {
  // assegnazione valori PWM in base a stato attuale
  switch (stato) {
    case SCALDA:
      PWM_resistenza = 200;
      PWM_ventola = 0;
      break;
    case RAFFREDDA:
      PWM_resistenza = 0;
      // PWM_ventola proporzionale alla differenza di temperatura (più veloce se errore grande)
      double kp = 230, q = 10;
      PWM_ventola = (byte) kp /(T_max - T_min) *(T_med - T_min) + q;
      break;
    case ATTESA:
    default:
      PWM_resistenza = 0;
      PWM_ventola = 0;
      break;
  }
}

void leggiDigital () {
  // lettura pin digitali (START e STOP)
  Start.fronte();
  Stop.fronte();
}
void leggiAnalog () {
  // lettura pin analogici (A0 -> temperatura) ogni 100 ms
  static unsigned long analogTime = 0;    // variabile statica (solo nella funzione, ma mantiene valore)

  if (millis() - analogTime >= 50) {
    analogTime = millis();                

    T_last = T;

    T_value = analogRead (pinTemp);            // lettura analogica di A0
    // calcolo Temperatura con proporzione  
    // lettura : 1023 = T : T_max

    // T = (float) T_value * 500.0 / 1023.0;     // proporzione con REF = 5 V (DEFAULT)
    T = (float) T_value * 110.0 / 1023.0;     // proporzione con REF = 1.1 V (INTERNAL)
    // T = (float) T_value * 330.0 / 1023.0;     // proporzione con es. REF = 3.3 V (EXTERNAL)

    T_med = (T + T_last) /2.0;                // temperatura media ultimi 2 valori -> aiuta a filtraggio software
  } 
}
void leggiSerial () {
  // lettura seriale (char) -> solo "fronte" (se c'è qualcosa salvo quel valore, altrimenti '\n')
  if (Serial.available()) {
    lettura = Serial.read();
  } else lettura = '\n';
}
void leggiIngressi() {
  leggiDigital ();
  leggiAnalog ();
  leggiSerial ();
}


void scriviDigital () {
  // assegnazione valori pin digitali
}
void scriviAnalog () {
  // assegnazione valori pin analogici
  analogWrite(pinResistenza, PWM_resistenza);   // scrittura analogica PWM su pin 10 (resistenza)
  analogWrite(pinVentola, PWM_ventola);
}
void scriviSerial () {
  // scrittura seriale ogni 100 ms (in formato Serial Plotter)
  static unsigned long serialTime = 0;

  if (millis() - serialTime >= 100) {
    serialTime = millis();

    Serial.print ("Tmin:");
    Serial.print (T_min);

    Serial.print (" Tmax:");
    Serial.print (T_max);

    Serial.print (" T:");
    Serial.print (T_med);

    Serial.print ('\n');
  }
}
void scriviUscite () {
  scriviDigital ();
  scriviAnalog ();
  scriviSerial ();
}
