$ErrorActionPreference = "Stop"
$ProjectDir = (Get-Location).Path
$Image  = "espressif/idf:release-v5.5"
$Service = "idf"

Write-Host "📦 Configurando ESP-IDF v5.5 no Docker (Windows)..."
Write-Host "Diretório: $ProjectDir"

# Criar docker-compose.yml
$composeContent = @"
version: "3.9"

services:
  $Service:
    image: $Image
    container_name: esp-idf
    working_dir: /work
    tty: true
    stdin_open: true
    volumes:
      - ./:/work
      - \${USERPROFILE}/.espressif:/root/.espressif
      - \${USERPROFILE}/.cache/ccache:/root/.cache/ccache
    environment:
      - IDF_CCACHE_ENABLE=1
"@
$composeContent | Out-File -Encoding UTF8 "$ProjectDir\docker-compose.yml"

# Criar idf.ps1
$helperContent = @"
param([Parameter(ValueFromRemainingArguments=\$true)] \$Args)
\$compose = Join-Path (Split-Path -Parent \$MyInvocation.MyCommand.Path) "docker-compose.yml"
if (\$Args.Count -eq 0) {
  docker compose -f "\$compose" run --rm $Service powershell
} else {
  docker compose -f "\$compose" run --rm $Service idf.py \$Args
}
"@
$helperContent | Out-File -Encoding UTF8 "$ProjectDir\idf.ps1"

# Baixar imagem do IDF
Write-Host "⬇️ Baixando imagem $Image ..."
docker pull $Image | Out-Null

Write-Host ""
Write-Host "✅ Ambiente configurado!"
Write-Host "Arquivos criados:"
Write-Host " - docker-compose.yml"
Write-Host " - idf.ps1"
Write-Host ""
Write-Host "Exemplos (PowerShell):"
Write-Host "  .\idf.ps1 --% --version"
Write-Host "  .\idf.ps1 --% build"
