@ECHO OFF
REM FastQuake benchmark (timedemo demo1). Results go to FQBENCH.TXT.
REM Usage: BENCH [exe] [extra options]   e.g.  BENCH FQUAKE -nosound
REM Your CONFIG.CFG is not changed by these runs.
SET FQEXE=%1
IF "%FQEXE%"=="" SET FQEXE=FQUAKE
%FQEXE% -fqbench %2 %3 %4 +exec FQB1.CFG
%FQEXE% -fqbench %2 %3 %4 +exec FQB2.CFG
%FQEXE% -fqbench %2 %3 %4 +exec FQB3.CFG
%FQEXE% -fqbench %2 %3 %4 +exec FQB4.CFG
%FQEXE% -fqbench %2 %3 %4 +exec FQB5.CFG
SET FQEXE=
TYPE FQBENCH.TXT
