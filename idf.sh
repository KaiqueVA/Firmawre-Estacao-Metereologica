set -e
SERVICE="idf"
if [ $# -eq 0 ]; then
  docker compose run --rm $SERVICE bash
else
  docker compose run --rm $SERVICE idf.py "$@"
fi
