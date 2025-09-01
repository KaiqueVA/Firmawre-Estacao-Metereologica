# 🌦️ Firmware - Estação Meteorológica

Projeto de firmware para ESP32 desenvolvido em **ESP-IDF v5.5**, rodando em ambiente **Docker** para garantir portabilidade e reprodutibilidade.  
O objetivo é capturar e processar dados meteorológicos, utilizando FreeRTOS e os recursos do ESP32.

---

## 📂 Estrutura do Projeto

```
Firmware-Estacao-Metereologica/
├── main/                # Código fonte principal (main.c, tasks, etc.)
├── build/               # Saídas de compilação (ignorado no git)
├── .devcontainer/       # Configuração para Dev Containers do VS Code
├── .vscode/             # Configurações locais do VS Code
├── docker-compose.yml   # Ambiente Docker com ESP-IDF
├── idf.sh               # Script auxiliar para rodar comandos idf.py no container
├── sdkconfig            # Configuração do projeto (gerada pelo menuconfig)
├── CMakeLists.txt       # Build system (ESP-IDF + CMake)
└── .gitignore           # Ignora build/ e artefatos de compilação
```

---

## 🐳 Ambiente de Desenvolvimento (Docker)

O projeto utiliza a imagem oficial da Espressif:

- **Imagem base**: `espressif/idf:release-v5.5`  
- **IDE**: Visual Studio Code com Dev Containers  
- **Toolchain**: `xtensa-esp-elf-gcc` fornecido pelo ESP-IDF  

### ▶️ Subindo o ambiente

Na raiz do projeto, rode:

```bash
docker compose run --rm idf idf.py --version
```

ou use o atalho:

```bash
./idf.sh --version
```

---

## ⚙️ Build do Projeto

Dentro do container (ou via script `idf.sh`):

```bash
./idf.sh set-target esp32
./idf.sh build
```

---

## 🔌 Flash e Monitor Serial

Conecte seu ESP32 e rode:

```bash
./idf.sh -p /dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0 flash monitor
```

> 🔎 A porta `/dev/serial/by-id/...` é usada para garantir identificação estável do dispositivo.

---

## 📦 Dependências

- [Docker](https://docs.docker.com/get-docker/) ≥ 20.x
- [Docker Compose](https://docs.docker.com/compose/)
- [Visual Studio Code](https://code.visualstudio.com/)
  - Extensão **Dev Containers**
  - Extensão **CMake Tools**
  - Extensão **C/C++**
  - (opcional) Extensão **Espressif IDF**

---

## 🛠️ Próximos Passos

- Implementar tarefas FreeRTOS para leitura de sensores (ex.: temperatura, umidade, pressão, sensores de vento).
- Configurar logging via UART.
- Adicionar suporte a Wi-Fi/MQTT para envio dos dados coletados.
- Adicionar suporte a LoRa
- Implementar configuração da estação via socket/http

