# Medidor de Nível de Ruído para Ambientes Hospitalares

Este repositório contém o código-fonte e a documentação do **Medidor de Nível de Ruído para Ambientes Hospitalares**, um projeto desenvolvido para a Residência Embarcatech. O sistema embarcado monitora continuamente o nível de ruído em ambientes críticos, como UTIs e enfermarias, e alerta a equipe caso o som ultrapasse um limite configurável.

## Descrição do Projeto

O projeto utiliza a plataforma **BitDogLab** com um microcontrolador RP2040 para captar o som ambiente via microfone, processar o sinal (remoção de offset DC, cálculo do valor RMS e conversão para decibéis - dB SPL) e exibir as informações em um display OLED. Quando o nível de ruído excede o limite definido, um buzzer é acionado por meio de PWM, emitindo um alerta sonoro. Além disso, o sistema permite que o usuário ajuste o limite de alerta utilizando dois botões e alterne entre modos de operação (monitoramento em tempo real e estatísticas) por meio de um joystick. No modo de estatísticas, o display exibe a quantidade de vezes que o alerta foi acionado, permitindo uma análise histórica simples.

## Funcionalidades

- **Monitoramento em Tempo Real:** Captura e processamento contínuo do áudio para exibição do nível sonoro em dB.
- **Alerta Sonoro:** Ativação de um buzzer quando o nível de ruído ultrapassa o limite configurado.
- **Interface Gráfica:** Exibição do valor de dB, gráfico do histórico de leituras e mensagens de alerta em um display OLED.
- **Ajuste de Limite:** Permite alterar o limiar de ruído via botões (aumentar/diminuir).
- **Modos de Operação:** Alternância entre modo de monitoramento e modo de estatísticas (contagem de alertas) utilizando o joystick.
- **Baixo Custo e Fácil Implementação:** Utilização de componentes acessíveis e integração completa na plataforma BitDogLab.

## Tecnologias Utilizadas

- **Linguagem de Programação:** C
- **Plataforma:** BitDogLab (RP2040)
- **Periféricos:** ADC para leitura do microfone e joystick, PWM para controle do buzzer, I²C para comunicação com o display OLED, GPIO para leitura dos botões.
- **Bibliotecas:** SDK do RP2040, biblioteca SSD1306 para o display OLED, funções de hardware padrão (adc, pwm, i2c, stdio).

## Estrutura do Projeto
├── README.md            # Este arquivo
├── Projeto_Final_Edcarllos.c    # Código-fonte do firmware
├── docs/                # Documentação técnica completa do projeto
│   ├── Relatorio_Tecnico.pdf  # Relatório completo (escopo, pesquisa, especificação)
│   └── ...              
└── LICENSE              # Licença do projeto


## Como Usar

1. **Configuração do Hardware:**  
   - Conecte o microfone ao pino ADC designado (GPIO28).  
   - Conecte o display OLED via I²C (SDA: GPIO14, SCL: GPIO15).  
   - Conecte o buzzer ao pino PWM (GPIO10) e os botões aos pinos GPIO5 e GPIO6.  
   - Conecte o joystick ao pino ADC correspondente (GPIO27).  

2. **Compilação:**  
   - Configure seu ambiente para compilar projetos com o SDK do RP2040.  
   - Compile o arquivo `Projeto_Final_Edcarllos.c` utilizando as ferramentas do Pico SDK.  

3. **Upload:**  
   - Após a compilação, faça o upload do firmware para a placa BitDogLab conforme as instruções da plataforma.  

4. **Operação:**  
   - Ao iniciar, o dispositivo exibirá “Sistema Ativo – v5.0” por alguns instantes.  
   - No modo de monitoramento, o display mostrará o nível atual de dB, um gráfico do histórico e o limite configurado.  
   - Quando o nível de ruído ultrapassar o limite, o buzzer será acionado e o display exibirá uma mensagem de alerta.  
   - Use os botões para ajustar o limite e o joystick para alternar para o modo estatísticas, onde será exibida a contagem de alertas.

## Exemplos de Uso

- **Ambiente Silencioso:** O display mostra valores de dB baixos e o gráfico indica uma variação suave, sem acionar o buzzer.
- **Ambiente Ruidoso:** Ao exceder o limite configurado, o sistema aciona o buzzer e exibe “ALERTA! Nível máximo excedido!”.
- **Modo Estatísticas:** Ao mover o joystick para a posição adequada, o display apresenta a contagem total de alertas acionados.

## Contribuições

Contribuições são bem-vindas! Se você deseja sugerir melhorias, reportar problemas ou propor novas funcionalidades, por favor abra uma _issue_ ou envie um _pull request_. Certifique-se de seguir as diretrizes de contribuição descritas no arquivo CONTRIBUTING.md.

## Licença

Este projeto está licenciado sob a [MIT License](LICENSE).

## Contato

Para dúvidas, sugestões ou suporte, entre em contato pelo e-mail: [edcarllossantos@gmail.com](mailto:edcarllossantos@gmail.com).

---

Este projeto foi desenvolvido com dedicação à inovação e à melhoria do ambiente hospitalar, alinhado aos objetivos da Residência Embarcatech. Esperamos que a solução inspire novas ideias e contribua para a criação de ambientes de saúde mais seguros e confortáveis.
