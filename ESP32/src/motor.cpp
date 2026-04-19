/**************** Definição de Pinos***************/
#define PinoMotorDesce 6  //ESP pino 25
#define PinoMotorSobe 5  //Esp pino 26

#define PinoBtnEmergencia 8  //Esp pino 32
#define PinoBtnPausa 9  //Esp pino 33
#define PinoBtnAcionamento 10  //Esp pino 34


/*********************Variáveis********************/
char* listaEstadosPrensa[] = {"esperando", "emergencia", "descendo", "subindo", "pausando"};

enum EstadosPrensa{
  	esperando = 0,
    emergencia,
	descendo,
  	subindo,
    pausando //termina a ação e entra em estado de pausa
};

EstadosPrensa estadoPrensa = esperando;
EstadosPrensa estadoAnteriorPrensa = esperando;

int passoPwm = 5; //define o salto da função softStart()
int passoPwmEmergencia = 254; //define o salto da função softStop() em estado de Emergência
int pwmMaximo = 255; // define o PWM máximo usado no softStart() e softStop()

int estadoPwm = 0;

bool esperandoPausa = 0;

bool estadoBtnAcionamento = 0;
bool estadoBtnEmergencia = 0;
bool estadoBtnPausa = 0;

bool estadoAnteriorEmergencia = 0;
bool estadoAnteriorPausa = 0;

unsigned long tempoUltimaEmergencia = millis();
unsigned long tempoUltimaPausa = millis();
int tempoDebounce = 200;



/**************************PROTÓTIPO**********************/
void estadoSerial(EstadosPrensa estadoAtual);


/**************************SETUP**************************/
void setup(){
  Serial.begin(9600);
  pinMode(PinoMotorDesce, OUTPUT);
  pinMode(PinoMotorSobe, OUTPUT);
  pinMode(PinoBtnEmergencia, INPUT_PULLUP);
  pinMode(PinoBtnPausa, INPUT_PULLUP);
  pinMode(PinoBtnAcionamento, INPUT_PULLUP);

}

void loop() {
    switch(estadoPrensa){
        case esperando:
      	  estadoSerial(estadoPrensa);
          esperar();
          break;
        
        case emergencia:
      	  estadoSerial(estadoPrensa);
          alertar();
          break;
        
        case descendo:
      	  estadoSerial(estadoPrensa);
      	  descer();
          break;
        
        case subindo:
      	  estadoSerial(estadoPrensa);
          subir();
          break;

        case pausando:
      	  estadoSerial(estadoPrensa);
          pausar();
          break;
    }
}


/****************Funções**********************/
void esperar(){

    verificarEstado();

    if(esperandoPausa){
        esperandoPausa = 0;
        estadoAnteriorPrensa = estadoPrensa;
        estadoPrensa = pausando;
        return;
    }

    if(estadoPrensa == esperando){
        //Caso Acionamento da Prensa
        estadoBtnAcionamento = !digitalRead(PinoBtnAcionamento);
        
        verificarEstado();
        if(estadoBtnAcionamento && !esperandoPausa){
            estadoAnteriorPrensa = estadoPrensa;
            estadoPrensa = descendo;
            return;
        } 
    }

    //lendo sensores
    //enviando dados
}

void descer(){
    verificarEstado();

    if(estadoPwm == 0){
        SoftStart();
    } else {
      	delay(4000);
    	SoftStop();
        estadoAnteriorPrensa = estadoPrensa;
        estadoPrensa = subindo;
    }

    /*  
    if(chegar no fundo || ou puxar corrente demais){ 
        estadoPrensa = subindo;
        return;
    }
    */
}

void subir(){
    if(estadoPwm == 0){
        SoftStart();
    } else {
        delay(4000);
        SoftStop();
        estadoAnteriorPrensa = estadoPrensa;
      	estadoPrensa = esperando;
      	return;
    }

    /*  
    if(chegar no fundo || ou puxar corrente demais){ 
        estadoPrensa = espera;
        return;
    }
    */
  
}

//emergencia
void alertar(){
    estadoPwm = 0;
    verificarEstado();
}

void pausar(){  
    verificarEstado();
}

void verificarEstado() {   
    estadoBtnEmergencia = !digitalRead(PinoBtnEmergencia);
    estadoBtnPausa = !digitalRead(PinoBtnPausa);

    
    if(estadoBtnEmergencia && estadoAnteriorEmergencia == 0  && millis() - tempoUltimaEmergencia >= tempoDebounce) {
        if(estadoPrensa == emergencia){
            //Caso esperando sair Emergência/Alerta
            tempoUltimaEmergencia = millis();
            estadoAnteriorPrensa = estadoPrensa;
            estadoPrensa = subindo;
            estadoAnteriorEmergencia = estadoBtnEmergencia;
            return;
        } else {
            //Caso Emergência/Alerta
            tempoUltimaEmergencia = millis();
            estadoAnteriorPrensa = estadoPrensa;
            estadoPrensa = emergencia;
            esperandoPausa = 0;
            estadoAnteriorEmergencia = estadoBtnEmergencia;
            return;    
        }
    } 

    if(estadoBtnEmergencia == 0 && estadoAnteriorEmergencia){
        tempoUltimaEmergencia = millis();
        estadoAnteriorEmergencia = estadoBtnEmergencia;
    }

    //Caso Pausa
    if(estadoBtnPausa && estadoPrensa != emergencia && estadoAnteriorPausa == 0 && millis() - tempoUltimaPausa >= tempoDebounce){
        if(esperandoPausa){
            //Caso esperando Pausa
            return;
        }
        
        if(estadoPrensa == pausando){
            //Caso esperando sair Pausa
            tempoUltimaPausa = millis();
            estadoAnteriorPrensa = estadoPrensa;
            estadoPrensa = esperando;
            estadoAnteriorPausa = estadoBtnPausa;
            return;
        } else {
            //Caso acione Pausa
            tempoUltimaPausa = millis();
            esperandoPausa = 1;
            estadoAnteriorPausa = estadoBtnPausa;
            Serial.println("Estado Prensa: Esperando Pausa");
            return;
        }
        
    }
    if(estadoBtnPausa == 0){
        tempoUltimaPausa = millis();
        estadoAnteriorPausa = estadoBtnPausa;
    }
}

void SoftStart(){
    if(estadoPwm != 0) return;

	for(int pwm = 0; pwm <= pwmMaximo; pwm += passoPwm){ 
      	verificarEstado();

        if(estadoPrensa == emergencia){
            analogWrite(PinoMotorDesce, 0);
            analogWrite(PinoMotorSobe, 0);
            return;
        }
        
      	if (estadoPrensa == subindo){
            analogWrite(PinoMotorDesce, 0);
            analogWrite(PinoMotorSobe, pwm);
            Serial.println("Subindo: " + String(map(pwm, 0, 255, 0, 100)) + "%");
            delay(200);

    	} else if(estadoPrensa == descendo) {
            analogWrite(PinoMotorDesce, pwm);
            analogWrite(PinoMotorSobe, 0);            
            Serial.println("Descendo: " + String(map(pwm, 0, 255, 0, 100)) + "%");
            delay(200);        
        } 
  	}
  	estadoPwm = 255;
}

void SoftStop(){
	for(int pwm = pwmMaximo; pwm >= 0; pwm -= passoPwm){ 
      	verificarEstado();

        if(estadoPrensa == emergencia){
            analogWrite(PinoMotorDesce, 0);
            analogWrite(PinoMotorSobe, 0);
            return;
        }

      	if (estadoPrensa == subindo){
            analogWrite(PinoMotorDesce, 0);
            analogWrite(PinoMotorSobe, pwm);
            
            Serial.println("Velocidade: " + String(map(pwm, 0, 255, 0, 100)) + "%");
            delay(200);

    	} else if(estadoPrensa == descendo) {
            analogWrite(PinoMotorDesce, pwm);
            analogWrite(PinoMotorSobe, 0);
            
            Serial.println("Velocidade: " + String(map(pwm, 0, 255, 0, 100)) + "%");
            delay(200);        
        } 
  	}
    estadoPwm = 0;
    delay(2000);
}

void estadoSerial(EstadosPrensa estadoAtual){
    if(estadoAnteriorPrensa != estadoAtual) {
        Serial.println("Estado Prensa: " + String(listaEstadosPrensa[estadoAtual]));
        estadoAnteriorPrensa = estadoPrensa;
    }
}
