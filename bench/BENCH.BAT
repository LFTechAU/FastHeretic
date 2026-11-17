@echo off
rem FastHeretic: run a benchmark file with every FastHeretic program here.
rem   BENCH            uses QUICK.BNC
rem   BENCH CPU.BNC    uses CPU.BNC
rem Results build up in FHBENCH.CSV (one line per run, program name first).
set FHB=%1
if "%FHB%"=="" set FHB=QUICK.BNC
for %%e in (FHTIC*.EXE FHM*.EXE) do %%e -nosound -bench %FHB%
set FHB=
