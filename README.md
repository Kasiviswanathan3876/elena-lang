Windows : [![Build status](https://ci.appveyor.com/api/projects/status/qc5dvs5lueubivuo?svg=true)](https://ci.appveyor.com/project/arakov/elena-lang)
# ELENA Programming Language

ELENA is a general-purpose, object-oriented language with late binding. It features message dispatching, mixins and a script engine.
This is the GitHub repository of ELENA source code including compiler, IDE, ELENA libraries and samples.

## Resources
- **Wiki:** <https://github.com/ELENA-LANG/elena-lang/wiki/>
- **ELENA reddit:** <https://www.reddit.com/r/elena_lang/>
- **Source code:** <https://github.com/ELENA-LANG/elena-lang>
- **Git clone URL:** <git://github.com/ELENA-LANG/elena-lang.git>
- **Twitter:** <https://twitter.com/elena_language>
- **Rosetta code:** <http://rosettacode.org/wiki/Category:Elena>
- **Binaries mirror:** <https://sourceforge.net/projects/elenalang/files/>

## Currently Supported Platforms

- **Windows** (v. 4.0)

For time being only x86 (32-bit) architecture is supported. Work on x86/64 (64-bit) goes on.

## Source Download and Compilation

To acquire the source code clone the git repository:

    git clone git://github.com//ELENA-LANG/elena-lang.git

### Windows:

The compiler code is implemented in C++ and does not require external dependencies. You just need Visual Studio 2015 or 2017.

You have to add a path to _BIN_ folder to the system environment *PATH* or copy elenavm.dll and elenart.dll to _Windows\System32_ folder.

To build the compiler under VS2015 you have to go to the root folder and type:

    recompile.bat

If you use VS2017 type the following:

    recompile15.bat

To build the ELENA libraries type the following command:

    rebuild_lib.bat
    
To build ELENA samples type the following command:

    rebuild_examples.bat 
    
To build ELENA Rosetta-code samples type the following command:

    examples\rosetta\build.bat 
    
To run unit-tests type the following command:

    lib_tests.bat     

## Contribute!

ELENA Project looks for programmers interested in learning the language and providing feedback.

### 1. Bugs, questions, suggestions?

If you've noticed a bug or have a question go ahead and [make one](https://github.com/ELENA-LANG/elena-lang/issues/new)!

[Join](https://github.com/ELENA-LANG/elena-lang/issues?q=is%3Aissue+is%3Aopen+label%3A%22Design+Idea%22) design discussions or [take part](https://github.com/ELENA-LANG/elena-lang/issues?q=is%3Aissue+is%3Aopen+label%3ADiscussion) in general talks.

### 2. Implement "up for grab" issues

[Good first issue](https://github.com/ELENA-LANG/elena-lang/labels/good%20first%20issue) is good starting point for a first-time contributors.

### 3. Rosetta code

You may try to implement some of [Rosetta code tasks](http://rosettacode.org/wiki/Category:Programming_Tasks) 
which are not yet implemented

## Source Code Organization

### Windows:

The ELENA source code is organized as follows:

    bin                 binaries and shared libraries
    bin\scripts         scripts used by the script engine and VM console
    bin\templates       ELENA project templates
    asm                 source for core routines implemented in assembly
    dat\sg              language grammar file            
    dat\og              language optimization rules
    doc                 some documentations
    elenasrc2\elc       source for the compiler
    elenasrc2\elenart   source for the run-time shared library
    elenasrc2\elenasm   source for the script engine
    elenasrc2\elenavm   source for the virtual machine
    elenasrc2\gui       source for IDE
    elenasrc2\tools     source for ELENA utilities
    examples            ELENA examples
    src40               source for ELENA libraries

## License

The compiler and executables distributed in this package fall under MIT License, 
for more information read the file LICENSE.

## Questions?

Feel free to ask any question :  elenaprolang@gmail.com.
