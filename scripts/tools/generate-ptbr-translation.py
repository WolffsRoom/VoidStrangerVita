#!/usr/bin/env python3
"""Generate a structurally safe PT-BR draft for Void Stranger International."""

from __future__ import annotations

import json
import os
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
LANGUAGES = ROOT / "mods" / "Languages"
SOURCE = LANGUAGES / "EN"
TARGET = LANGUAGES / "PTBR"
CACHE_FILE = ROOT / ".tools" / "ptbr-translation-cache.json"

# UI and recurring lore terms get an explicit Brazilian localization.  The
# neural translator handles dialogue while this table keeps the terminology
# stable and the Vita-facing menus concise.
EXACT = {
    "ENGLISH": "PORTUGUÊS (BRASIL)",
    "English script": "Tradução PT-BR para PS Vita",
    "MENUS": "MENUS",
    "[Yes]": "[Sim]",
    "[No]": "[Não]",
    "BACK": "VOLTAR",
    "ON": "LIGADO",
    "OFF": "DESLIGADO",
    "FINNISH": "FINLANDÊS",
    "RESUME": "CONTINUAR",
    "SETTINGS": "CONFIGURAÇÕES",
    "QUIT GAME": "SAIR DO JOGO",
    "GRAPHICS": "GRÁFICOS",
    "AUDIO": "ÁUDIO",
    "CONTROLS": "CONTROLES",
    "MISC": "OUTROS",
    "MASTER": "GERAL",
    "MUSIC": "MÚSICA",
    "SOUND": "SOM",
    "RESOLUTION": "RESOLUÇÃO",
    "FULLSCREEN": "TELA CHEIA",
    "TIMER": "CRONÔMETRO",
    "COUNTER": "CONTADOR",
    "UP": "CIMA",
    "LEFT": "ESQUERDA",
    "RIGHT": "DIREITA",
    "DOWN": "BAIXO",
    "ACTION KEY": "BOTÃO DE AÇÃO",
    "LANGUAGE": "IDIOMA",
    "START": "INICIAR",
    "Skipping...": "Pulando...",
    "TEXT SPEED": "VELOCIDADE DO TEXTO",
    "MID": "MÉDIA",
    "SLOW": "LENTA",
    "FAST": "RÁPIDA",
    "PAUSE": "PAUSAR",
    "PRESS ACTION TO CONFIRM": "PRESSIONE X PARA CONFIRMAR",
    "PRESS ANY KEY/BUTTON TO CONTINUE": "PRESSIONE X PARA CONTINUAR",
    "YOU'VE FALLEN": "VOCÊ CAIU",
    "NO CONTROLLERS FOUND": "NENHUM CONTROLE ENCONTRADO",
    "CONTROLLER(S) FOUND": "CONTROLE(S) ENCONTRADO(S)",
    "MOVES": "MOVIMENTOS",
    "VOIDS": "VAZIOS",
    "TIME": "TEMPO",
    "Resetting...": "Reiniciando...",
    "FLASHING FX": "EFEITOS PISCANTES",
    "PALETTE": "PALETA",
    "Wake up": "Acordar",
    "End it all": "Acabar com tudo",
    "REST SHUTDOWN": "DESCANSO FINAL",
    "PRESENTED BY": "APRESENTADO POR",
    "STAFF": "EQUIPE",
    "SOUND LIBRARIES": "BIBLIOTECAS DE ÁUDIO",
    "PLAYTESTERS": "TESTADORES",
    "MEMORIES": "MEMÓRIAS",
    "BURDENS": "FARDOS",
    "VOID MEMORY": "MEMÓRIA DO VAZIO",
    "VOID SWORD": "ESPADA DO VAZIO",
    "VOID WINGS": "ASAS DO VAZIO",
    "MOVEMENT": "MOVIMENTO",
    "TAP": "TOCAR",
    "HOLD": "SEGURAR",
    "SCALING": "ESCALA",
    "INTEGER": "INTEIRA",
    "FIT": "AJUSTAR",
    "BORDER COLOR": "COR DA BORDA",
    "BLACK": "PRETO",
    "DARK": "ESCURO",
    "VSYNC": "VSYNC",
    "60 FPS": "60 FPS",
    "30 FPS": "30 FPS",
    "REDUCED": "REDUZIDOS",
}

DO_NOT_TRANSLATE = re.compile(
    r"^(?:\s*|[-+*/=<>_|.0-9]+|[A-Z0-9_]+\.(?:png|ogg|wav)|https?://\S+)$"
)
TOKEN = re.compile(r"(\\[nrt]|#[0-9A-Fa-f]{6}|\{[^{}]*\}|%[-+0-9.]*[a-zA-Z])")


def protect(text: str) -> tuple[str, list[str]]:
    tokens: list[str] = []

    def repl(match: re.Match[str]) -> str:
        tokens.append(match.group(0))
        return f"ZXQ{len(tokens) - 1}QXZ"

    return TOKEN.sub(repl, text), tokens


def restore(text: str, tokens: list[str]) -> str:
    for index, token in enumerate(tokens):
        text = text.replace(f"ZXQ{index}QXZ", token)
        text = text.replace(f"ZXQ {index} QXZ", token)
    return text


def main() -> int:
    tools = ROOT / ".tools" / "argostranslate"
    os.environ.setdefault("ARGOS_PACKAGES_DIR", str(ROOT / ".tools" / "argos-models"))
    sys.path.insert(0, str(tools))
    import argostranslate.translate  # type: ignore

    installed = argostranslate.translate.get_installed_languages()
    english = next(x for x in installed if x.code == "en")
    portuguese = next(x for x in installed if x.code == "pt")
    translator = english.get_translation(portuguese)
    if translator is None:
        raise RuntimeError("English to Portuguese Argos model is not installed")

    cache: dict[str, str] = {}
    if CACHE_FILE.exists():
        cache = json.loads(CACHE_FILE.read_text(encoding="utf-8"))

    TARGET.mkdir(parents=True, exist_ok=True)
    translated_count = 0
    for filename in ("included.txt", "extracted.txt"):
        source_text = (SOURCE / filename).read_text(encoding="utf-8-sig")
        trailing_newline = source_text.endswith(("\n", "\r"))
        source_lines = source_text.splitlines()
        output: list[str] = []
        for index, line in enumerate(source_lines):
            if filename == "extracted.txt" and index == 0:
                output.append("WolffsRoom")
                continue
            if line in EXACT:
                output.append(EXACT[line])
                continue
            if DO_NOT_TRANSLATE.fullmatch(line) or not re.search(r"[A-Za-z]", line):
                output.append(line)
                continue
            if line not in cache:
                safe, tokens = protect(line)
                cache[line] = restore(translator.translate(safe), tokens)
                translated_count += 1
                if translated_count % 50 == 0:
                    CACHE_FILE.parent.mkdir(parents=True, exist_ok=True)
                    CACHE_FILE.write_text(
                        json.dumps(cache, ensure_ascii=False, indent=2), encoding="utf-8"
                    )
                    print(f"translated={translated_count} cache={len(cache)}", flush=True)
            output.append(cache[line])
        result = "\n".join(output) + ("\n" if trailing_newline else "")
        (TARGET / filename).write_text(result, encoding="utf-8", newline="\n")

    CACHE_FILE.parent.mkdir(parents=True, exist_ok=True)
    CACHE_FILE.write_text(json.dumps(cache, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"done translated_now={translated_count} cached={len(cache)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
