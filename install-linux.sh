#!/usr/bin/env bash
set -e

PROJECT_DIR="$(pwd)"
IMAGE="espressif/idf:release-v5.5"
SERVICE="idf"
SERIAL_PATH="${1:-/dev/ttyUSB0}"

echo "📦 Configurando ESP-IDF v5.5 no Docker (Linux)..."
echo "Diretório do projeto: $PROJECT_DIR"
echo "Serial: $SERIAL_PATH"

# Criar docker-compose.yml
cat > "${PROJECT_DIR}/docker-compose.yml" <<EOF
version: "3.9"

services:
  ${SERVICE}:
    image: ${IMAGE}
    container_name: esp-idf
    working_dir: /work
    tty: true
    stdin_open: true
    user: "\${UID:-1000}:\${GID:-1000}"
    devices:
      - "${SERIAL_PATH}:${SERIAL_PATH}"
    volumes:
      - ./:/work
      - \${HOME}/.espressif:/root/.espressif
      - \${HOME}/.cache/ccache:/root/.cache/ccache
    environment:
      - IDF_CCACHE_ENABLE=1
EOF

# Criar idf.sh
cat > "${PROJECT_DIR}/idf.sh" <<'EOSH'
#!/usr/bin/env bash
set -e
COMPOSE_FILE="$(dirname "$0")/docker-compose.yml"
SERVICE="idf"
if [ $# -eq 0 ]; then
  docker compose -f "$COMPOSE_FILE" run --rm $SERVICE bash
else
  docker compose -f "$COMPOSE_FILE" run --rm $SERVICE idf.py "$@"
fi
EOSH
chmod +x "${PROJECT_DIR}/idf.sh"

# Baixar imagem do IDF (garante que já estará disponível)
echo "⬇️ Baixando imagem ${IMAGE}..."
docker pull "${IMAGE}"

echo "✅ Ambiente configurado!"
echo "Arquivos criados:"
echo " - docker-compose.yml"
echo " - idf.sh"
echo
echo "Exemplo de uso:"
echo "  ./idf.sh --version"
echo "  ./idf.sh build"
echo "  ./idf.sh -p ${SERIAL_PATH} flash monitor"
