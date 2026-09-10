#!/usr/bin/env python3
"""Normalize common European-Portuguese MT forms to conversational PT-BR."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]

WORDS = {
    "tu": "você", "és": "é", "estás": "está", "tens": "tem",
    "queres": "quer", "podes": "pode", "vais": "vai", "sabes": "sabe",
    "deves": "deve", "viste": "viu", "fizeste": "fez", "encontraste": "encontrou",
    "consegues": "consegue", "achas": "acha", "pensas": "pensa", "acreditas": "acredita",
    "dizes": "diz", "falas": "fala", "pareces": "parece", "precisas": "precisa",
    "tentaste": "tentou", "chegaste": "chegou", "vieste": "veio", "foste": "foi",
    "teu": "seu", "tua": "sua", "teus": "seus", "tuas": "suas",
    "contigo": "com você", "rapariga": "garota", "raparigas": "garotas",
    "ficheiro": "arquivo", "ficheiros": "arquivos", "ecrã": "tela", "ecrãs": "telas",
}

PHRASES = {
    "a sério": "sério",
    "tenho a certeza": "tenho certeza",
    "tens a certeza": "tem certeza",
    "está bem": "tudo bem",
    "cão de colo": "cachorrinho de estimação",
    "telemóvel": "celular",
    "casa de banho": "banheiro",
    "comboio": "trem",
    "autocarro": "ônibus",
    "equipa": "equipe",
    "facto": "fato",
    "contacto": "contato",
}


def same_case(source: str, replacement: str) -> str:
    if source.isupper():
        return replacement.upper()
    if source[:1].isupper():
        return replacement[:1].upper() + replacement[1:]
    return replacement


def gerund(verb: str) -> str:
    low = verb.lower()
    if low == "pôr":
        result = "pondo"
    elif low.endswith("ar"):
        result = low[:-2] + "ando"
    elif low.endswith("er"):
        result = low[:-2] + "endo"
    elif low.endswith("ir"):
        result = low[:-2] + "indo"
    else:
        return verb
    return same_case(verb, result)


def normalize(text: str) -> str:
    # European continuous form: "está a tentar" -> "está tentando".
    def continuous(match: re.Match[str]) -> str:
        auxiliary = {"estou": "estou", "estás": "está", "está": "está",
                     "estamos": "estamos", "estão": "estão",
                     "estava": "estava", "estavam": "estavam"}[match.group(1).lower()]
        auxiliary = same_case(match.group(1), auxiliary)
        return f"{auxiliary} {gerund(match.group(2))}"

    text = re.sub(
        r"\b(estou|estás|está|estamos|estão|estava|estavam) a ([A-Za-zÀ-ÿ]+(?:ar|er|ir|ôr))\b",
        continuous, text, flags=re.IGNORECASE,
    )
    for source, replacement in PHRASES.items():
        text = re.sub(
            rf"\b{re.escape(source)}\b",
            lambda m, r=replacement: same_case(m.group(0), r),
            text, flags=re.IGNORECASE,
        )
    for source, replacement in WORDS.items():
        text = re.sub(
            rf"\b{re.escape(source)}\b",
            lambda m, r=replacement: same_case(m.group(0), r),
            text, flags=re.IGNORECASE,
        )
    return text


for filename in ("included.txt", "extracted.txt"):
    path = ROOT / "mods" / "Languages" / "PTBR" / filename
    original = path.read_text(encoding="utf-8")
    path.write_text(normalize(original), encoding="utf-8", newline="\n")
    print(f"normalized {path}")
