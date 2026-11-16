/* A Shell gives 4 KB of stack; libnix's startup sets this much up instead:
 * wxWidgets' event handling and Aria's editors nest deeply.
 * GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team. */
unsigned long __stack = 1024 * 1024;
static const char version[] __attribute__((used)) = "$VER: AriaMaestosa 1.4.13 (16.11.2026) GFOS 68040";
