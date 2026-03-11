/**************** Definição de Pinos***************/
#define PinoMotorSobe 6
#define PinoMotorDesce 3
#define PinoBtnEmergencia 8


/*********************Variáveis********************/
enum EstadosPrensa{
  	parado = 0,
    emergencia,
	descendo,
  	subindo,
    espera //desabilita outras ações
};

EstadosPrensa estadoPrensa = parado;

int estadoPwm = 0;


/**************************SETUP**************************/
void setup()
{
  Serial.begin(9600);
  pinMode(PinoMotorDesce, OUTPUT);
  pinMode(PinoMotorSobe, OUTPUT);
  pinMode(PinoBtnEmergencia, INPUT);
}

void loop()
{
    switch(estadoPrensa){
        case parado:
      		esperar();
            break;
        
        case emergencia:
            break;
        
        case descendo:
      		descer();
            break;
        
        case subindo:
            break;
        
        case espera:
            break;
    }
}


/****************Funções**********************/
void esperar(){
    estadoPrensa = descendo;
}

void descer(){
  verificarEstado();
  if(estadoPwm == 0) {
  	estadoPwm = pwm();
  } else {
  	
  }
  
    //Função verificar corrente atual que motor está puxando

}

void subir(){
    digitalWrite(PinoMotorDesce, LOW);
    digitalWrite(PinoMotorSobe, HIGH);
}

void verificarEstado() {
	int estadoBtnEmergencia = digitalRead(PinoBtnEmergencia);
    if(estadoBtnEmergencia){
        estadoPrensa = emergencia;
        Serial.println("Emergencia");
	}
}

int pwm(){
	for(int pwm = 0; pwm <= 255; pwm += 4){
     	if (estadoPrensa == descendo){
          verificarEstado();
          digitalWrite(PinoMotorDesce, LOW);
          analogWrite(PinoMotorSobe, pwm);
          Serial.println(pwm);
          delay(20);
    	}
  	}
  
  	return 255;
}
