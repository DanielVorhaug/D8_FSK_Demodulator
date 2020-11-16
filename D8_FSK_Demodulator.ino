#include <TimerOne.h>

// Globale variaber

const int sampling_frekvens = 5250;                   // Frekvense målingene skal bli gjort på.
int counter = 0;                                    // Hjelpevariabel for å skrive ut verdier én gang i sekundet

  // pin-nummer på output
const int u_pin = 4;
const int b_pin = 5;
const int timer_pin = 3;

volatile int sample;                                  // Holder siste sample.
bool newSample;                                       // Støtte varibel for å sjekke om ny sample er tatt.

const int antall_samples = 50;                        // Hvor mange målinger som skal holdes.
int samples_hode = 0;                                 // Hvor i arrayen av samples, samples_inn, den nye målingen skal settes inn.
int samples_inn[antall_samples];


  // Hjelpevariabler for filter0 som slipper gjennom f_0 = 175 Hz
const int N_filter0 = 7;                              // Antall ordens filter
int filter0_hode = antall_samples - N_filter0 - 1;    // Viser hvilken verdi fra samples_inn som skal fjernes for å finne filtrert verdi.
int filter0_nyverdi_hode = 0;                         // Hvor i arrayen av filterverdier den nye verdien skal settes inn.
int filter0_middelverdi_hode = 1;                     // Viser hvilken verdi fra filter0 som skal fjernes for å finne middelverdi.
int filter0_verdi = 0;                                // Nye verdi fra filteret.
int filter0[N_filter0];                               // Holder de siste målte filtrerte verdiene.
long filter0_middelverdi = 0;                         // Holder middelverdien, men uten å være delt på N_filter0, siden deling tar mye prosesseringskraft.
long filter0_sammenligning = 0;                       // Holder en verdi for å sammenligne de to filtrene.


  // Hjelpevariabler for filter1 som slipper gjennom f_1 = 750 Hz
const int N_filter1 = 30;                             // Antall ordens filter.
int filter1_hode = antall_samples - N_filter1 - 1;    // Viser hvilken verdi fra samples_inn som skal fjernes for overå finne filtrert verdi.
int filter1_nyverdi_hode = 0;                         // Hvor i arrayen av filterverdier den nye verdien skal settes inn.
int filter1_middelverdi_hode = 1;                     // Viser hvilken verdi fra filter1 som skal fjernes for å finne middelverdi.
int filter1_verdi = 0;                                // Nye verdi fra filteret.
int filter1[N_filter1];                               // Holder de siste målte filtrerte verdiene.
long filter1_middelverdi = 0;                         // Holder middelverdien, men uten å være delt på N_filter1, siden deling tar mye prosesseringskraft.
long filter1_sammenligning = 0;                       // Holder en verdi for å sammenligne de to filtrene.


typedef enum {
  UBESTEMT,     // Ingen frekvenser er målt
  F0,           // f_0 er målt
  F1            // f_1 er målt
} state_t;

state_t state = UBESTEMT;

const int counter_limit = 500;  // Antall interupt i strekk at en måling må være lik for å bytte state. L = 500
int ubestemt_counter = 0;
int f0_counter = 0;
int f1_counter = 0;


  // Fyller arrayen med verdien 0
void nuller(int *array_inn, int *lengde) {
  for (int i = 0; i < lengde; i++) {
    *(array_inn + i) = 0;
  }
}


  // Forandrer output etter hvilken state som er målt.
void changed_state() {
  switch (state) {
    case UBESTEMT: 
      digitalWrite(u_pin, LOW);
      digitalWrite(b_pin, LOW);
      break;
    
    case F0: 
      digitalWrite(u_pin, HIGH);
      digitalWrite(b_pin, LOW);
      break;
    
    case F1: 
      digitalWrite(u_pin, HIGH);
      digitalWrite(b_pin, HIGH);
      break;
  }
}

  // Forander verdien til hodet til neste plass i arrayen.
void ny_hodeverdi(int *hode, int grense) {
  (*hode)++;
  if (*hode == grense) {
  *hode = 0;
  }
}
    
    

void setup() {
  pinMode(OUTPUT,u_pin);
  pinMode(OUTPUT,b_pin);
  pinMode(OUTPUT,timer_pin); // Brukes til å måle om arduinoen klarer å bli ferdig med signalprosesseringen før neste interrupt kommer.

  nuller(&samples_inn[0], antall_samples);
  nuller(&filter0[0], N_filter0);
  nuller(&filter1[0], N_filter1);
  
  Serial.begin(115200);

  changed_state(); // Passer på å begynne i UBESTEMT state.
  
  // Oppsett av timer interrupt
  Timer1.initialize(1000000/sampling_frekvens); // Periode mellom hver sample.
  // Argumentet i "attachInterrupt" bestemmer hvilken funskjon som er interrupt handler.
  Timer1.attachInterrupt(takeSample);
}


  

void loop() {
  
  if(newSample){

    //
    // Samples
    //
    
    samples_inn[samples_hode] = sample;
    ny_hodeverdi(&samples_hode, antall_samples);



    //
    // Filter 0
    //
    
    filter0_verdi = filter0_verdi + sample - samples_inn[filter0_hode]; 
    filter0[filter0_nyverdi_hode] = filter0_verdi;

    ny_hodeverdi(&filter0_hode, antall_samples);
    ny_hodeverdi(&filter0_nyverdi_hode, N_filter0);

    
    filter0_middelverdi =  filter0_middelverdi + abs(filter0_verdi) - abs(filter0[filter0_middelverdi_hode]); // Legger til nye verdi og trekker fra gamle. 

    ny_hodeverdi(&filter0_middelverdi_hode, N_filter0);
    

       
    //
    // Filter 1
    //

    filter1_verdi = filter1_verdi + sample - samples_inn[filter1_hode];
    filter1[filter1_nyverdi_hode] = filter1_verdi;

    ny_hodeverdi(&filter1_hode, antall_samples);
    ny_hodeverdi(&filter1_nyverdi_hode, N_filter1);

    
    filter1_middelverdi =  filter1_middelverdi + abs(filter1_verdi) - abs(filter1[filter1_middelverdi_hode]); // Legger til nye verdi og trekker fra gamle. 


    ny_hodeverdi(&filter1_middelverdi_hode, N_filter1);
    

    



    //
    //  State
    //

    filter0_sammenligning = N_filter1 * filter0_middelverdi;
    filter1_sammenligning = 15* N_filter0 * filter1_middelverdi;


      // Skriver ut verdier én gang i sekundet
    /*counter++;
    if (counter > sampling_frekvens) {
      Serial.print(filter0_sammenligning);
      Serial.print(" ");
      Serial.println(filter1_sammenligning);
      counter = 0;    
    }*/

        // Sammenligner middelverdier og legger til i tellere
    if (filter0_sammenligning > 100000 || filter1_sammenligning > 100000) { // K = 100000
      if (filter0_sammenligning > filter1_sammenligning){
        ubestemt_counter = 0;
        f0_counter++;
        f1_counter = 0;
      } else {
        ubestemt_counter = 0;
        f0_counter = 0;
        f1_counter++;
      }
    } else {
      ubestemt_counter++;
      f0_counter = 0;
      f1_counter = 0;
    }
    
        // Sjekker om middelverdiene har vært forandret lenge nok for
        // at en forandring i signalet har skjedd
    if (ubestemt_counter > counter_limit) {
      state = UBESTEMT;
      changed_state();
      ubestemt_counter = 0;
    } else if (f0_counter > counter_limit) {
      state = F0;
      changed_state();
      f0_counter = 0;
    } else if (f1_counter > counter_limit) {
      state = F1;
      changed_state();
      f1_counter = 0;
    }

    newSample = false;
    
    digitalWrite(timer_pin, LOW); // Setter pin lav når signalprosessering er ferdig
  }
  
}

// Interrupt-handler (kalles ved hvert interrupt)
void takeSample(void){
  digitalWrite(timer_pin, HIGH); // Setter pin høy når signalet kommer inn
  
  sample = analogRead(0) - 512; // Sampler på A0. En verdi tilsvarende 2.5V er trukket fra
  newSample = true;  
}
