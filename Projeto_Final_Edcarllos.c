#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "inc/ssd1306_i2c.h"
#include "pico/stdlib.h"

// ==========================================================================
// Projeto: Medidor de Nível de Ruído para Ambientes Hospitalares
// Descrição: Este projeto monitora o nível de ruído em um ambiente hospitalar,
//            exibindo o nível de som em decibéis (dB) em um display OLED e
//            acionando um alarme sonoro quando o ruído excede um limite
//            configurável. No modo Estatísticas, o display mostra a quantidade
//            de vezes que o alerta máximo de som foi emitido.
//
// Componentes do sistema:
// - Microfone (sensor de som): capta o som ambiente e gera um sinal elétrico
//   analógico proporcional.
// - Conversor AD (ADC): converte o sinal analógico do microfone em valores
//   digitais para o microcontrolador.
// - Display OLED: exibe os valores medidos (nível de ruído em dB), gráficos e
//   mensagens de alerta.
// - Buzzer (avisador sonoro): emite um som de alerta quando o nível de ruído
//   ultrapassa o limite definido.
// - Botões (A e B): permitem ajustar a sensibilidade/limite de ruído para o
//   alerta (mais alto ou mais baixo).
// - Joystick (eixo X): alterna entre modos de operação (monitoramento em tempo
//   real ou visualização de estatísticas).
//
// Conceitos implementados no código:
// - Conversão do sinal do microfone (tensão analógica) para nível de pressão
//   sonora em decibéis (dB SPL).
// - Remoção do offset DC do sinal do microfone para obter apenas a componente
//   AC (variação do som efetiva).
// - Cálculo do valor RMS (raiz média quadrática) do sinal AC para estimar o
//   nível sonoro eficaz.
// - Conversão do valor RMS para decibéis utilizando uma calibração baseada em
//   20 µPa como referência (0 dB SPL).
// - Uso de uma média móvel para calcular o offset DC (filtro passa-baixa) do
//   microfone.
// - Ajuste da sensibilidade (limite de alerta) em tempo real via botões.
// - Uso de PWM (modulação por largura de pulso) para gerar o som do buzzer de
//   alerta.
// - Uso de um buffer circular para armazenar histórico dos níveis de ruído e
//   gerar um gráfico no display.
// - Exibição de estatísticas: quantidade de vezes que o alerta máximo foi
// acionado.
// ==========================================================================

// Configurações de Hardware
#define MICROFONE_ADC_PIN 28
#define BUZZER_PIN 10
#define BOTAO_A 5
#define BOTAO_B 6
#define JOYSTICK_VRX 27
#define I2C_SDA 14
#define I2C_SCL 15

// Parâmetros do Sistema
#define LIMITE_DB 100.0
#define CALIBRACAO 0.00002f  // 20µPa (referência 0 dB SPL)
#define AMOSTRAS 64
#define TAMANHO_HISTORICO 128
#define UPDATE_INTERVAL_MS 30

// Variáveis Globais
volatile float limite_atual_db =
    LIMITE_DB;  // Limite de ruído para disparo do alerta (dB)
enum Modo { MONITORAMENTO, ALERTA, ESTATISTICAS } modo_atual = MONITORAMENTO;
float historico[TAMANHO_HISTORICO];  // Histórico de níveis (dB)
uint8_t indice_historico = 0;        // Índice do histórico

// Variável para contar quantas vezes o alerta máximo foi acionado
unsigned int contador_alertas = 0;

// Protótipos de Funções
void inicializar_hardware();
float ler_decibeis();
void atualizar_display(float db, uint8_t *display_buffer);
void verificar_botoes();
void acionar_buzzer(bool estado);
void atualizar_historico(float db);
void desenhar_grafico(uint8_t *buffer);
void verificar_joystick();
void exibir_texto(uint8_t *buffer, struct render_area *area, char *lines[],
                  int num_lines);
float obter_valor_maximo(float *array, int size);

int main() {
  stdio_init_all();
  inicializar_hardware();

  struct render_area area_total = {.start_column = 0,
                                   .end_column = ssd1306_width - 1,
                                   .start_page = 0,
                                   .end_page = ssd1306_n_pages - 1};
  calculate_render_area_buffer_length(&area_total);

  uint8_t display_buffer[ssd1306_buffer_length];
  memset(display_buffer, 0, ssd1306_buffer_length);

  // Mensagem inicial
  char *inicio[] = {"Sistema Ativo", "v5.0"};
  exibir_texto(display_buffer, &area_total, inicio, 2);
  render_on_display(display_buffer, &area_total);
  sleep_ms(500);

  // Loop principal: leitura dos sensores e atualização do display
  while (true) {
    float db = ler_decibeis();  // Lê o nível de ruído (dB)
    atualizar_historico(db);    // Atualiza histórico de leituras

    verificar_botoes();    // Verifica botões para ajuste de sensibilidade
    verificar_joystick();  // Verifica joystick para mudança de modo

    memset(display_buffer, 0, ssd1306_buffer_length);

    // Verifica se o nível de ruído excede o limite e se não está no modo
    // ESTATISTICAS
    bool alerta_cond = (db > limite_atual_db) && (modo_atual != ESTATISTICAS);
    acionar_buzzer(alerta_cond);  // Aciona/desativa o buzzer conforme condição

    if (modo_atual == ESTATISTICAS) {
      // No modo Estatísticas, exibe o número de alertas emitidos
      char texto_estatisticas[30];
      snprintf(texto_estatisticas, sizeof(texto_estatisticas), "Alertas: %u",
               contador_alertas);
      // Exibe a mensagem no display
      char *estatisticas[] = {"Modo Estatisticas", texto_estatisticas};
      exibir_texto(display_buffer, &area_total, estatisticas, 2);
    } else {
      // Em outros modos, exibe normalmente o nível de ruído e gráfico
      if (alerta_cond) {
        char *alerta[] = {"  ALERTA!  ", "Nivel maximo", " excedido!  "};
        exibir_texto(display_buffer, &area_total, alerta, 3);
      } else {
        atualizar_display(db, display_buffer);
      }
    }

    render_on_display(display_buffer, &area_total);
    sleep_ms(UPDATE_INTERVAL_MS);
  }

  return 0;
}

float obter_valor_maximo(float *array, int size) {
  float max = array[0];
  for (int i = 1; i < size; i++) {
    if (array[i] > max) max = array[i];
  }
  return max;
}

// --------------------------------------------------------------------------
// Função: ler_decibeis
// Descrição: Lê o sinal do microfone via ADC e converte para dB SPL.
// --------------------------------------------------------------------------
float ler_decibeis() {
  static float offset_dc = 0.0f;
  float soma_quadrados = 0.0f;

  for (int i = 0; i < AMOSTRAS; i++) {
    adc_select_input(2);
    uint16_t leitura_adc = adc_read();
    float tensao = (leitura_adc * 3.3f) / 4096.0f;

    // Remove o offset DC usando um filtro passa-baixa
    offset_dc = 0.95f * offset_dc + 0.05f * tensao;
    float tensao_ac = (tensao - offset_dc) * 10.0f;  // Aplica ganho de 10x
    soma_quadrados += tensao_ac * tensao_ac;
    sleep_us(10);
  }

  float rms = sqrtf(soma_quadrados / AMOSTRAS);
  return fmaxf(30.0f, 20.0f * log10f(rms / CALIBRACAO + 1e-12f));
}

// --------------------------------------------------------------------------
// Função: atualizar_display
// Descrição: Atualiza o conteúdo do display OLED com o nível de ruído.
// --------------------------------------------------------------------------
void atualizar_display(float db, uint8_t *display_buffer) {
  char texto_db[20], texto_limite[25];
  snprintf(texto_db, sizeof(texto_db), "%.1f dB", db);
  snprintf(texto_limite, sizeof(texto_limite), "Limite: %.1f dB",
           limite_atual_db);

  const char *texto_modo = (modo_atual == MONITORAMENTO) ? "Monitoramento"
                           : (modo_atual == ALERTA)      ? "Modo Alerta"
                                                         : "Estatisticas";

  uint8_t buffer[ssd1306_buffer_length];
  memset(buffer, 0, ssd1306_buffer_length);

  ssd1306_draw_string(buffer, 5, 0, texto_db);
  ssd1306_draw_string(buffer, 5, 45, texto_modo);
  ssd1306_draw_string(buffer, 5, 55, texto_limite);

  if (modo_atual == MONITORAMENTO) {
    float nivel_maximo_db = obter_valor_maximo(historico, TAMANHO_HISTORICO);
    float escala = (nivel_maximo_db > 40.0f) ? (40.0f / nivel_maximo_db) : 1.0f;
    for (int i = 0; i < TAMANHO_HISTORICO; i++) {
      int y = 40 - (int)(historico[(indice_historico + i) % TAMANHO_HISTORICO] *
                         escala);
      ssd1306_set_pixel(buffer, i, y, true);
    }
    ssd1306_draw_line(buffer, 0, 15, ssd1306_width - 1, 15, true);
    ssd1306_draw_line(buffer, 0, 43, ssd1306_width - 1, 43, true);
  }

  memcpy(display_buffer, buffer, ssd1306_buffer_length);
}

// --------------------------------------------------------------------------
// Função: inicializar_hardware
// Descrição: Configura I2C, ADC, PWM e GPIO para os componentes.
// --------------------------------------------------------------------------
void inicializar_hardware() {
  i2c_init(i2c1, 400 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  uint8_t init_commands[] = {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
                             0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
                             0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB,
                             0x40, 0xA4, 0xA6, 0xAF};
  ssd1306_send_command_list(init_commands, sizeof(init_commands));

  adc_init();
  adc_gpio_init(MICROFONE_ADC_PIN);
  adc_select_input(2);

  gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
  pwm_config config = pwm_get_default_config();
  pwm_init(slice_num, &config, true);

  gpio_init(BOTAO_A);
  gpio_set_dir(BOTAO_A, GPIO_IN);
  gpio_pull_up(BOTAO_A);
  gpio_init(BOTAO_B);
  gpio_set_dir(BOTAO_B, GPIO_IN);
  gpio_pull_up(BOTAO_B);
}

// --------------------------------------------------------------------------
// Função: verificar_botoes
// Descrição: Verifica os botões para ajustar o limite de ruído.
// --------------------------------------------------------------------------
void verificar_botoes() {
  static uint32_t last_press = 0;
  const uint32_t debounce_delay = 200000;  // 200 ms

  if (time_us_32() - last_press < debounce_delay) return;

  if (!gpio_get(BOTAO_A))
    limite_atual_db = fminf(120.0f, limite_atual_db + 1.0f);
  if (!gpio_get(BOTAO_B))
    limite_atual_db = fmaxf(30.0f, limite_atual_db - 1.0f);

  last_press = time_us_32();
}

// --------------------------------------------------------------------------
// Função: verificar_joystick
// Descrição: Lê o joystick e define o modo de operação.
// --------------------------------------------------------------------------
void verificar_joystick() {
  adc_gpio_init(JOYSTICK_VRX);
  adc_select_input(1);
  float posicao_x = adc_read() / 4096.0f;

  if (posicao_x < 0.3f)
    modo_atual = MONITORAMENTO;
  else if (posicao_x > 0.7f)
    modo_atual = ESTATISTICAS;

  adc_select_input(2);
  adc_read();
}

// --------------------------------------------------------------------------
// Função: acionar_buzzer
// Descrição: Aciona ou desativa o buzzer com base na condição de alerta.
//          Se o alerta for acionado (transição de desligado para ligado),
//          incrementa o contador de alertas.
// --------------------------------------------------------------------------
void acionar_buzzer(bool estado) {
  static bool estado_anterior = false;
  if (estado == estado_anterior) return;

  uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
  uint canal = pwm_gpio_to_channel(BUZZER_PIN);
  if (estado) {
    uint32_t wrap_val = 125000000 / 2500 / 16;  // ~3125 para ~2.5 kHz
    pwm_set_wrap(slice_num, wrap_val);
    pwm_set_chan_level(slice_num, canal, wrap_val / 2);  // 50% duty cycle
    pwm_set_enabled(slice_num, true);
    // Incrementa o contador somente na transição para alerta (de desligado para
    // ligado)
    if (!estado_anterior) {
      contador_alertas++;
    }
  } else {
    pwm_set_enabled(slice_num, false);
  }
  estado_anterior = estado;
}

// --------------------------------------------------------------------------
// Função: exibir_texto
// Descrição: Exibe múltiplas linhas de texto no display OLED.
// --------------------------------------------------------------------------
void exibir_texto(uint8_t *buffer, struct render_area *area, char *lines[],
                  int num_lines) {
  memset(buffer, 0, ssd1306_buffer_length);
  int y_pos = 0;
  for (int i = 0; i < num_lines; i++) {
    ssd1306_draw_string(buffer, 5, y_pos, lines[i]);
    y_pos += 8;
  }
}

// --------------------------------------------------------------------------
// Função: atualizar_historico
// Descrição: Armazena o último valor de dB no histórico circular.
// --------------------------------------------------------------------------
void atualizar_historico(float db) {
  historico[indice_historico] = db;
  indice_historico = (indice_historico + 1) % TAMANHO_HISTORICO;
}

// --------------------------------------------------------------------------
// Função: desenhar_grafico
// Descrição: Desenha um gráfico simples dos valores do histórico.
// --------------------------------------------------------------------------
void desenhar_grafico(uint8_t *buffer) {
  for (int i = 0; i < TAMANHO_HISTORICO; i++) {
    int index = (indice_historico + i) % TAMANHO_HISTORICO;
    int y = 40 - (int)(historico[index] * 0.8f);
    ssd1306_set_pixel(buffer, i, y, true);
  }
}
