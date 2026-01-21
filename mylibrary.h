#ifndef myLibrary
#define myLibrary .h

// dichiaro le costanti del codice (wait di default)
#define DT_WAIT_DEBOUNCE 20
#define DT_M 10000
#define DT_LUCE 1000

// dichiaro classe madre (idealmente astratta, quindi utilizzabile solo da altre classi come stampo; nella pratica accessibile anche dall'esterno (solo public))
class Pin {
  protected: // accessibili solo da sottoclassi
    char set_up;
    unsigned short pin;

    Pin(unsigned short pin, char set_up) : pin(pin), set_up(set_up) {}  // metodo costruttore

  public:
    bool s;

    void init () {
      pinMode (pin, set_up);
    }
};

// dichiaro classe Pulsante come sottoclasse di Pin (assume come propri tutti i parametri e metodi di Pin)
class Pulsante : public Pin {
  private:    // variabili private della classe Pulsante (definizioni + variabili delle funzioni + costanti)
    #define ATTESA 0
    #define CONTROLLO 1
    #define PREMUTO 2
    
    bool lettura;
    char stato_digitalRead_debounced = ATTESA;

    bool letturaFronte;
    bool letturaFronte_old;

    bool POff;
    bool POn;

    int dt_wait_debounce;

    char logic;

    unsigned long tempo;

  public:
    bool f_UP, f_DOWN;

    Pulsante (unsigned short pin, char set_up, char logic = 'p', int dt_wait_debounce = DT_WAIT_DEBOUNCE) : Pin(pin, set_up), logic(logic), dt_wait_debounce(dt_wait_debounce) { 
      // metodo costruttore che richiama metodo costruttore di Pin

      if (logic == 'p') { // assegno valori di POn e POff
        POn = 1;
        POff = 0;
      } else {
        POn = 0;
        POff = 1;
      }
    }

    void digitalRead_debounced () { // digitalRead con debounce
    
      lettura = digitalRead(pin);
    
      switch (stato_digitalRead_debounced) {
        case ATTESA:
          if (lettura == POn){
            stato_digitalRead_debounced = CONTROLLO;
            tempo = millis();
          }
          break;
    
        case CONTROLLO:
          if ((millis() - tempo) > dt_wait_debounce) {
            if (lettura == POn)
              stato_digitalRead_debounced = PREMUTO;
            if (lettura == POff)
              stato_digitalRead_debounced = ATTESA;
          }
          break;
    
        case PREMUTO:
          if (lettura == POff)
            stato_digitalRead_debounced = ATTESA;
          break;
      }
    
      switch (stato_digitalRead_debounced) {
        case ATTESA:
        case CONTROLLO:
          s = false;
          break;
    
        case PREMUTO:
          s = true;
          break;
      }
    }


    void fronte () {    // lettura fronti (UP e DOWN assieme)
      letturaFronte_old = letturaFronte;
      digitalRead_debounced();
      letturaFronte = s;
      
      if (letturaFronte == 1 && letturaFronte_old == 0)
        f_UP = true;
      else
        f_UP = false;
      
      if (letturaFronte == 0 && letturaFronte_old == 1)
        f_DOWN = true;
      else
        f_DOWN = false;
    }
  
};

// dichiaro classe Motore come sottoclasse di Pin (assume come propri tutti i parametri e metodi di Pin)
class Motore : public Pin {
  private:
    unsigned long dt_M;
    unsigned long t;
    bool s_old;

  public:
    bool f_UP, f_DOWN;

    Motore (unsigned short pin, unsigned long dt_M = DT_M) : Pin(pin, OUTPUT), dt_M(dt_M) {
      // metodo costruttore che richiama metodo costruttore di Pin
    }

    void ON () {  // attivo
      s_old = s;
      s = 1;
      f_UP = ((s_old == 0 && s == 1) ? 1 : 0);
    }

    void OFF () { // spento
      s_old = s;
      s = 0;
      f_DOWN = ((s_old == 1 && s == 0) ? 1 : 0);
    }

    void scrivi () {  // scrittura (per scriviUscite)
      digitalWrite(pin, s);
    }

    void updateTime () {  // aggiornamento variabile tempo (privata)
      t = millis();
    }

    bool doneTime () {    // verifico se è passato il tempo
      return millis() - t >= dt_M;
    }
};


// dichiaro classe Led come sottoclasse di Pin (assume come propri tutti i parametri e metodi di Pin)
class Led : public Pin {
  private:
    bool enable;

    unsigned int dt_luce;
    unsigned long t_luce;
    
    void updateTime () {
      t_luce = millis();
    }


  public:
    Led (unsigned short pin, unsigned int dt_luce = DT_LUCE) : Pin(pin, OUTPUT), dt_luce(dt_luce) {
      // metodo costruttore che richiama metodo costruttore di Pin
    }


    void ON () {  // led on
      s = 1;
      enable = 1;
    }

    void OFF () { // led off
      s = 0;
      enable = 1;
    }

    void ENABLE (bool enable = 1) { // serve per segnalazione (caso favorevole o meno)
      this->enable = enable;
    }

    void scrivi () {    // scrittura (per scriviUscite)
      digitalWrite(pin, s && enable);
    }

    void segnalazione () {  // segnalazione (lampeggiante)
      if (millis() - t_luce >= dt_luce) {
        s = !s;
        updateTime();
      }
    }
};


#endif
