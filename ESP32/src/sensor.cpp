/**************** Definição de Pinos***************/
#define PinoTriggerSensor1 11
#define PinoTriggerSensor2 12
#define PinoTriggerSensor3 13

#define PinoEchoSensor1 8
#define PinoEchoSensor2 9
#define PinoEchoSensor3 10


/*********************Variáveis********************/
unsigned long tempoUltimaLeitura = 0;

float valorSensor1 = 0;
float valorSensor2 = 0;
float valorSensor3 = 0;

int delaySensor = 0,06


/**************************SETUP**************************/
void setup() {
  Serial.begin(9600);
  pinMode(PinoTriggerSensor1, OUTPUT);
  pinMode(PinoTriggerSensor2, OUTPUT);
  pinMode(PinoTriggerSensor3, OUTPUT);
  
  pinMode(PinoEchoSensor1, INPUT);
  pinMode(PinoEchoSensor2, INPUT);
  pinMode(PinoEchoSensor3, INPUT);
}

void loop() {
	valorSensor1 = ativarSensor(PinoTriggerSensor1, PinoEchoSensor1);
	Serial.print("Sensor 1: "); Serial.println(valorSensor1); 
  	delay(60);
  	valorSensor2 = ativarSensor(PinoTriggerSensor2, PinoEchoSensor2);
	Serial.print("Sensor 2: "); Serial.println(valorSensor2); 
  	delay(60);
  	valorSensor3 = ativarSensor(PinoTriggerSensor3, PinoEchoSensor3);
	Serial.print("Sensor 2: "); Serial.println(valorSensor3); 	
  	
  	delay(5000);
  
}


/****************Funções**********************/

float ativarSensor(int PinoTrigger, int PinoEcho){
	digitalWrite(PinoTrigger, HIGH);
  	delayMicroseconds(10);
	digitalWrite(PinoTrigger, LOW);
  
	float valor = pulseIn(PinoEcho, HIGH, 30000);
  	
  	return valor/58; // divisão para transformar valor em centímetros
}
