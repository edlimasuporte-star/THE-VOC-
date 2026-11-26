# THE VOC FX

Plugin **VST3 + Standalone** (Windows e macOS) de efeitos vocais criativos para **performance ao vivo**,
com as mesmas funcoes do VocalSynth 2: cinco motores de sintese vocal, sete efeitos, modos Auto/MIDI/Sidechain,
correcao de afinacao e um capacete vintage no centro cuja viseira acende com a sua voz.

## Motores vocais
| THE VOC FX | Funcao | Controles |
|---|---|---|
| **TRACT** | sintese de trato vocal sobre o synth | Clarity, Shift, Nasal, Breath, Level, Pan, Synth |
| **VOCODER** | vocoder de banco de filtros | Mode (Smooth 20 bandas / Vintage 12 / Hard 32), Shift, Scale, Level, Pan, Synth |
| **ROBOT** | voz computadorizada LPC | Bits, Bytes, Bats, Mode (Spell/Read/Math), Level, Pan, Synth |
| **TALKBOX** | talkbox sem tubo | Drive, Speaker, Formant, Mode (Dark/Classic/Bright), Level, Pan, Synth |
| **HARMONY** | gerador de harmonias com a sua voz | Formant, Character, Humanize, Level, Width |

## Pitch & Voicing
- **AUTO**: detecta o pitch e toca o synth seguindo a voz + ate 3 vozes diatonicas (Off, -2 oit ... +2 oit)
- **MIDI**: Mono (com Glide) ou Poly (8 vozes); as harmonias seguem as notas tocadas
- **SIDECHAIN**: o instrumento no sidechain vira a portadora ("faz a guitarra falar")
- Register Low/Mid/High, Key, Scale (Major, Minor, Chromatic, pentatonicas, Dorian), correcao com Speed e Strength
- Synth: 10 timbres (Saw, Square, Super Saws, Sync, Triangle, Saw+Sub, Pulse, Noise, Organ, 80s Brass), oitava, attack, release, vibrato

## Efeitos (em serie)
Distort (Warm/Analog/Edgy/Fuzz) · Ring Mod (sync) · Filter (NY LP / NY HP / Scream / Combo) ·
Transform (8 caixas/alto-falantes) · Shred (stutter sincronizado com sequencia euclidiana A/B/C) · Chorus · Delay (sync, ping-pong)
Saida: Mix, Gate, Pan, Width, Gain e limitador de seguranca para palco.

## Ao vivo
- **Baixa latencia**: nenhuma latencia reportada ao host; a harmonia/correcao usa PSOLA em tempo real (~2 periodos da voz)
- Tela LIVE minimalista; tela EDIT com todos os parametros
- **Capacete = XY pad**: arraste para controlar dois parametros (X/Y escolhidos no topo); a viseira acende com a cor dos motores ativos e mostra a nota cantada
- **INSPIRE**: gera 6 presets criativos (com nivel ajustado automaticamente) para desbloqueio criativo
- **Presets**: 53 de fabrica (BASICO, POP, ALTERNATIVO, INDIE, ELETRONICO, WORSHIP, ANOS 80, ANOS 90, HIP HOP, EXPERIMENTAL) + os seus (Documentos/THE VOC FX/Presets)
- Program Change MIDI troca preset; tudo automatizavel na DAW

## Download
Cada push nesta branch compila no GitHub Actions (Windows + macOS universal).
Baixe em **Actions → Build THE VOC FX → (ultima execucao verde) → Artifacts**. Veja `INSTALAR.txt`.

Para gerar uma pagina de download permanente: **Releases → Draft a new release**, crie a tag `fx-v1.0.0` apontando para a branch `the-voc-fx` e clique **Publish** — o GitHub Actions compila e anexa os `.zip` de Windows e macOS automaticamente.

## Compilar localmente
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Requer CMake 3.22+, Visual Studio 2022 (Windows) ou Xcode (macOS). O JUCE e baixado automaticamente.

## Testes do motor (sem JUCE)
`c++ -O2 -std=c++17 -I Source -o fx_test tests/fx_test.cpp && ./fx_test`
Confere afinacao das harmonias e da correcao, niveis de cada motor, os 53 presets, o INSPIRE e o uso de CPU.

## Licenca
O JUCE e AGPLv3 ou licenca comercial. Para distribuir o THE VOC FX, publique o codigo sob AGPLv3 ou adquira uma licenca JUCE.
VocalSynth e marca da iZotope; este projeto e independente e nao usa codigo nem recursos dela.
