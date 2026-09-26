# EcoCompact - Compactador Automatizado
Compactador automatizado e inteligente de resíduos, desenvolvido como projeto de TCC em Mecatrônica (SENAI). O EcoCompact combina **mecânica, eletrônica e programação embarcada** para reduzir o volume de lixo em Ecopontos, propondo uma alternativa aos problemas de superlotação, coleta ineficiente e impacto urbano do descarte irregular.

<img width="3039" height="2032" alt="Projeto" src="https://github.com/user-attachments/assets/667818de-e712-4c76-a54e-25c6fb9de36d" />
---

## Sumário

- [Visão geral](#visão-geral)
- [Programação (ESP32)](#programação-esp32)
  - [Arquitetura e organização do código](#arquitetura-e-organização-do-código)
  - [Máquina de estados e prioridades](#máquina-de-estados-e-prioridades)
  - [Controle do motor (soft start/stop)](#controle-do-motor-soft-startstop)
  - [Leitura dos sensores ultrassônicos](#leitura-dos-sensores-ultrassônicos)
  - [Painel local (LCD)](#painel-local-lcd)
  - [Conectividade: Wi-Fi, Firebase e app](#conectividade-wi-fi-firebase-e-app)
- [Mecânica](#mecânica)
- [Eletrônica](#eletrônica)
- [Estrutura do repositório](#estrutura-do-repositório)
- [Possíveis melhorias futuras](#possíveis-melhorias-futuras)

---

## Visão geral

O EcoCompact é uma prensa compactadora automatizada baseada em ESP32, controlada por fuso trapezoidal acionado por motor, com sensoriamento de nível/capacidade e posição, painel de controle local via LCD e monitoramento remoto por aplicativo mobile conectado em tempo real via Firebase.

Principais funcionalidades entregues:
- Compactação automatizada de resíduos
- Botão de emergência com trava total do sistema
- Modo de pausa para manutenção/retirada de material com segurança
- Monitoramento de capacidade e status via LCD e app
- Conectividade Wi-Fi para sincronização remota

## Programação (ESP32)

O firmware foi desenvolvido em **C/C++ para ESP32**, usando a **Arduino IDE Cloud**, em um **único arquivo (`EcoCompact.ino`)**. Essa escolha não foi por preferência de organização, mas por limitação da ferramenta: o ambiente utilizado não oferecia um compilador local eficiente para a placa nem suporte à divisão do projeto em múltiplos arquivos/abas com a mesma praticidade de uma IDE local, então o código foi mantido centralizado para simplificar o fluxo de desenvolvimento e upload.

### Arquitetura e organização do código

O código segue uma estrutura clássica de firmware:
- **Includes e definição de pinos**: motor, botões (emergência, pausa, acionamento) e três sensores ultrassônicos (trigger/echo).
- **Configuração de conectividade**: credenciais de Wi-Fi e Firebase, além dos objetos `FirebaseData`, `FirebaseAuth` e `FirebaseConfig`.
- **Variáveis de estado, PWM, botões, temporizadores (`millis()`) e leitura dos sensores**, todas centralizadas para fácil ajuste de parâmetros (velocidade do motor, tempos de debounce, limites dos sensores etc.).
- **`setup()`**: inicializa pinos, LCD (com caracteres customizados), sensores, conecta ao Wi-Fi (com timeout) e, se houver internet, autentica no Firebase, abre um *stream* do Realtime Database e busca configurações remotas (nome do equipamento, limites dos sensores, parâmetros de PWM).
- **`loop()`**: lê o stream do Firebase, atualiza o estado dos botões, decide o estado atual da prensa e executa a função correspondente a esse estado.

### Máquina de estados e prioridades

O comportamento da prensa é modelado como uma **máquina de estados finita**, com cinco estados principais:

| Estado | Descrição |
|---|---|
| `esperando` | Aguardando acionamento; monitora os sensores de capacidade em segundo plano |
| `descendo` | Prensa descendo para compactar o material |
| `subindo` | Prensa retornando à posição inicial após compactar |
| `emergencia` | Trava total da máquina; motores são desligados imediatamente |
| `pausando` | Estado de manutenção, para retirada de material ou limpeza com segurança |

A lógica de transição entre estados foi pensada em **ordem de prioridade das ações**:

1. **Emergência** — prioridade máxima. Pode ser acionada a qualquer momento (fisicamente ou pelo app) e interrompe qualquer ação em andamento, zerando o PWM do motor instantaneamente. Acionar o botão novamente restaura a prensa ao estado seguinte (subindo, por segurança).
2. **Pausa** — prioridade intermediária. Não interrompe uma compactação em andamento: se solicitada durante o movimento, fica sinalizada (`esperandoPausa`) e só é efetivada quando a prensa retorna ao estado de espera ou termina o ciclo de compactação. Serve para permitir manutenção, limpeza ou remoção de materiais com segurança.
3. **Acionamento/Compactação** — menor prioridade entre as ações manuais, só ocorre quando não há emergência nem pausa pendente.

Essa hierarquia é controlada principalmente pela função `verificarEstado()`, que também aplica **debounce** nos botões físicos e trata os comandos vindos remotamente (via Firebase) da mesma forma que os botões locais.

### Controle do motor (soft start/stop)

Em vez de ligar o motor diretamente em potência máxima, o firmware implementa uma rampa de PWM (`soft()`), incrementando ou decrementando o valor de PWM gradualmente (`pwmPasso`) até atingir o valor máximo configurado (`pwmMaximo`). Isso suaviza a partida e a parada do motor, reduzindo esforço mecânico sobre o fuso e a estrutura, e é usado tanto ao acelerar (descida/subida) quanto ao desacelerar antes de trocar de sentido ou parar.

### Leitura dos sensores ultrassônicos

O sistema usa três sensores HC-SR04:
- **Sensores 1 e 2**: alternam leituras em segundo plano durante o estado de espera para estimar o **percentual de capacidade** do compactador, que é enviado ao Firebase sempre que muda.
- **Sensor 3**: monitora a **posição da prensa** durante o movimento, definindo quando ela atingiu o limite inferior (fim da compactação) ou o limite superior (retorno completo), disparando a troca de estado.

### Painel local (LCD)

Um LCD I2C exibe o nome do equipamento e o estado atual da prensa, com **ícones customizados** (glifos definidos byte a byte) para cada estado — espera, emergência, descendo, subindo e pausa — permitindo identificação rápida por cor/símbolo sem depender de texto. Durante a pausa pendente, um ícone pisca no canto do display como indicador visual.

### Conectividade: Wi-Fi, Firebase e app

O painel de controle foi pensado em duas camadas: **local** (LCD + botões físicos) e **remota** (aplicativo desenvolvido no **MIT App Inventor**). A sincronia entre ESP32 e app ocorre via **Wi-Fi**, usando o **Firebase Realtime Database** como intermediário:

- O ESP32 abre um *stream* no RTDB para receber em tempo real os comandos de emergência, pausa e acionamento enviados pelo app.
- Estados, percentual de capacidade e outras informações são escritos de volta no banco, permitindo que o app reflita o status atual do equipamento.
- Parâmetros de configuração (nome do compactador, limites dos sensores, valores de PWM) também podem ser definidos remotamente e são lidos pelo ESP32 na inicialização.
- Há uma checagem de conectividade real com a internet (requisição HTTP a um endpoint do Google) antes de tentar autenticar no Firebase, evitando travar o `setup()` caso haja Wi-Fi mas sem acesso externo.

Tanto o app quanto o LCD usam **símbolos e cores distintas** para dar um feedback visual rápido do estado do equipamento ao usuário.

## Mecânica

- Estrutura em MDF
- Fuso trapezoidal + porca de bronze
- Motor de 3750 RPM
- Placa compactadora com movimento linear guiado por tubo/eixo

## Eletrônica

- ESP32 (controlador principal)
- Ponte H BTS7960 (acionamento do motor)
- 3x sensor ultrassônico HC-SR04
- Fonte 12V
- Display LCD I2C

## Estrutura do repositório

```
EcoCompact/
├── Testes_individuais/     # Testes isolados de cada componente
├── App.aia                 # Aplicativo Mobile
└── EcoCompact.ino          # Firmware principal (ESP32, C/C++)
```

A pasta `Testes_individuais` foi mantida no repositório mesmo sendo anterior ao início "oficial" do projeto, pois documenta o processo de aprendizado e pode ser útil como referência para quem quiser entender o uso isolado de cada componente (motor, sensor, LCD etc.).

## Possíveis melhorias futuras

- Implementação de esteiras e aumento de capacidade
- Melhorias na conectividade
- Ampliação para espaços públicos da cidade de São Paulo
- Parcerias público-privadas para viabilizar a instalação em Ecopontos
