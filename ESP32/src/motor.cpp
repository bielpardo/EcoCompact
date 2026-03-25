/**************** Definição de Pinos***************/
#define PinoMotorSobe 6
#define PinoMotorDesce 3

#define PinoBtnEmergencia 8
#define PinoBtnPausa 0
#define PinoBtnAcionamento 0


/*********************Variáveis********************/
enum EstadosPrensa{
  	espera = 0,
    emergencia,
	descendo,
  	subindo,
    pausa //termina a ação e entra em estado de pausa
};

EstadosPrensa estadoPrensa = descendo;

int estadoPwm = 0;
bool esperandoPausa = 0;

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
        case espera:
      		descer();
            break;
        
        case emergencia:
            alertar();
            break;
        
        case descendo:
      		descer();
            break;
        
        case subindo:
            subir();
            break;

        
        case pausa:
            pausar();
            break;
    }
}


/****************Funções**********************/
void esperar(){
    verificarEstado();

    if(esperandoPausa == 1){
        estadoPrensa = pausa;
        return;
    } else {
        if(estadoPrensa == espera){
            //Caso Acionamento da Prensa
            int estadoBtnAcionamento = digitalRead(PinoBtnAcionamento);
            if(estadoBtnAcionamento){
                estadoPrensa = descendo;
                Serial.println("Estado Prensa: Descendo");
                return;
            } 
        }
    }

    //lendo sensores
    //enviando dados
}

void descer(){
  verificarEstado();
  if(estadoPwm == 0) {
  	estadoPwm = pwm();
  } else {
  	
  }

    //Função verificar corrente atual que motor está puxando

}

void subir(){}

void alertar(){
    int estadoBtnEmergencia = digitalRead(PinoBtnEmergencia);

    digitalWrite(PinoMotorDesce, LOW);
    digitalWrite(PinoMotorSobe, LOW);

    if(estadoBtnEmergencia) {
        subir();
        return;
    }
}

void pausar(){

}

void verificarEstado() {
    if(esperandoPausa == 1) return;
    
    //Caso Pausa
    int estadoBtnPausa = digitalRead(PinoBtnPausa);
    if(estadoBtnPausa){
        esperandoPausa = 1;
        return;
    }

    //Caso Emergência/Alerta
    int estadoBtnEmergencia = digitalRead(PinoBtnEmergencia);
    if(estadoBtnEmergencia){
        estadoPrensa = emergencia;
        Serial.println("Estado Prensa: Emergencia/Alerta");
        return;
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