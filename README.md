# Cosmic Feedback Killer (VST3, JUCE 8)

Одна ручка AMOUNT 0.0-10.0 (автоматизируется в FL Studio).

## Сборка (Windows)
Нужны: Visual Studio 2022 (Desktop C++), CMake 3.22+, Git, интернет (JUCE скачается сам).

    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release

Готовый файл: build/CosmicFeedbackKiller_artefacts/Release/VST3/Cosmic Feedback Killer.vst3
(при сборке от администратора копируется в C:\Program Files\Common Files\VST3 автоматически).

## FL Studio
Options -> Manage plugins -> Find installed plugins. Поставить на микрофонный канал в Mixer.
Автоматизация: ПКМ по ручке -> Create automation clip.
