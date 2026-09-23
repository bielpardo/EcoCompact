#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

/************************Variáveis*************************/
byte SimboloEsperando[8] = { B11111, B10001, B01010, B00100, B01010, B10001, B11111, B00000 };
byte SimboloEmergencia[8] = { B00100, B00100, B00100, B00100, B00100, B00000, B00100, B00000 };
byte SimboloDescendo[8] = { B00100, B00100, B00100, B00100, B10101, B01110, B00100, B00000 };
byte SimboloSubindo[8] = { B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00000 };
byte SimboloPausa[8] = { B00000, B01010, B01010, B01010, B01010, B01010, B00000, B00000 };

enum EstadosPrensa{
  	esperando = 0,
    emergencia,
	descendo,
  	subindo,
    pausando //termina a ação e entra em estado de pausa
};

EstadosPrensa estadoPrensa = esperando;
char* listaEstadosPrensa[] = {"Esperando", "emergencia", "descendo", "subindo", "pausando"};


/**************************PROTÓTIPO***********************/
void atualizarLcd(EstadosPrensa estadoAtual);


/**************************SETUP**************************/
void setup(){
  lcd.init();
  lcd.backlight();
  
  lcd.createChar(0, SimboloEsperando);
  lcd.createChar(1, SimboloEmergencia);
  lcd.createChar(2, SimboloDescendo);
  lcd.createChar(3, SimboloSubindo);
  lcd.createChar(4, SimboloPausa);
  
  atualizarLcd(estadoPrensa);
}

void loop(){
    
}

/***********************Funções**************************/
void atualizarLcd(EstadosPrensa estadoAtual){
  lcd.setCursor(0,0);
  lcd.write(estadoAtual);
  lcd.print(listaEstadosPrensa[estadoAtual]);
}