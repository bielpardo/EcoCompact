/**************** Definição de Pinos***************/
#define PinoTriggerSensor1 11
#define PinoTriggerSensor2 12
#define PinoTriggerSensor3 13

#define PinoEchoSensor1 8
#define PinoEchoSensor2 9
#define PinoEchoSensor3 10


/*********************Variáveis********************/
unsigned long tempoUltimaLeitura = 0;
unsigned long tempoUltimaLeituraGeral = 0;
int DelayEntreLeitura = 2000;
int DelayEntreLeituraGeral = 5000;
int sensorAtual = 1; // sensor que vai ser lido 

float valorSensor1 = 0;
float valorSensor2 = 0;
float valorSensor3 = 0;


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
    while(millis() - tempoUltimaLeituraGeral >= DelayEntreLeituraGeral){
        if(millis() - tempoUltimaLeitura >= DelayEntreLeitura && sensorAtual == 1){
            lerSensor(PinoTriggerSensor1, PinoEchoSensor1, valorSensor1);
        }
         
        if(millis() - tempoUltimaLeitura >= DelayEntreLeitura && sensorAtual == 2){
            lerSensor(PinoTriggerSensor2, PinoEchoSensor2, valorSensor2);
        }
        
        if(millis() - tempoUltimaLeitura >= DelayEntreLeitura && sensorAtual == 3){
            lerSensor(PinoTriggerSensor3, PinoEchoSensor3, valorSensor3);
            tempoUltimaLeituraGeral = millis();
        }
    }
}


/****************Funções**********************/

float ativarSensor(int PinoTrigger, int PinoEcho){
	digitalWrite(PinoTrigger, HIGH);
  	delayMicroseconds(10);
	digitalWrite(PinoTrigger, LOW);
  
	float valor = pulseIn(PinoEcho, HIGH, 30000);
  	
  	return valor/58; // divisão para transformar valor em centímetros
}

void lerSensor(int PinoTrigger, int PinoEcho, float valorSensor){
    valorSensor = ativarSensor(PinoTrigger, PinoEcho);
    Serial.println("Sensor " + String(sensorAtual) + ": " + String(valorSensor)); 
    sensorAtual = (sensorAtual == 3) ? 1 : sensorAtual + 1;
    tempoUltimaLeitura = millis();
}