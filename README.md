# 🌊 Sistema Smart-Flow: Telemetria de Rios (Fase 1: Firmware & MQTT)

Este repositório contém a especificação técnica e o firmware de controle para o sistema **Smart-Flow** reestruturado. O projeto foi migrado para a plataforma **ESP32**, focando em telemetria pura de riscos ambientais (transbordamento de rios e deslizamento de encostas) e transmissão de dados assíncrona via protocolo **MQTT**.

---

## 📐 1. Arquitetura de Hardware e Pinagem (ESP32)

A nova estrutura elimina componentes de sinalização local (LEDs) para concentrar o fluxo de dados na rede. O display LCD e o Buzzer atuam como redundância e aviso sonoro de emergência local na bancada de testes.

### Tabela de Conexões (Pin Mapping)

| Componente | Pino do Componente | Pino no ESP32 | Função Técnico-Ambiental | Tipo de Sinal |
| :--- | :--- | :--- | :--- | :--- |
| **LCD 2004 I2C** | GND | GND | Aterramento do sistema | Energia |
| **LCD 2004 I2C** | VCC | 5V | Alimentação da lógica do display | Energia |
| **LCD 2004 I2C** | SDA | **GPIO 21** | Transmissão de Dados da Interface | I2C Nativo |
| **LCD 2004 I2C** | SCL | **GPIO 22** | Sincronismo de Clock da Interface | I2C Nativo |
| **HC-SR04** | TRIG | **GPIO 5** | Gatilho para disparo do pulso sônico | Saída Digital |
| **HC-SR04** | ECHO | **GPIO 19** | Retorno do pulso (Nível do rio) | Entrada Digital (3.3V)* |
| **Sensor de Chuva** | AO (Analog Out) | **GPIO 34** | Medição do índice de precipitação | Entrada Analógica (ADC1) |
| **Higrômetro (Solo)**| AO (Analog Out) | **GPIO 35** | Monitoramento de encharcamento da margem| Entrada Analógica (ADC1) |
| **Buzzer Ativo** | Positivo (+) | **GPIO 13** | Alarme sonoro de evacuação civil | Saída Digital / PWM |

> *Atenção:* O conversor analógico-digital (ADC) do ESP32 opera com resolução de 12 bits (valores de `0` a `4095`). Os pinos GPIO 34 e 35 são exclusivos de entrada, ideais para sensores analógicos estáveis.

---

## 🌐 2. Topologia da Rede MQTT

O ESP32 atua como um **Publisher** (publicador), fatiando os dados lidos em tópicos primitivos para evitar o overhead de processamento de strings JSON complexas no microcontrolador.

### Estrutura de Tópicos (Mapeamento)

```mermaid
graph TD
    ESP32[ESP32 / Smart-Flow] -->|Publica| T1[rio/telemetria/nivel]
    ESP32 -->|Publica| T2[rio/telemetria/chuva]
    ESP32 -->|Publica| T3[rio/telemetria/solo]
    ESP32 -->|Publica| T4[rio/telemetria/status]
    T1 & T2 & T3 & T4 --> Broker[Broker MQTT: HiveMQ Público]
    Broker -->|Assina| Python[Script Python / Consumidor]
