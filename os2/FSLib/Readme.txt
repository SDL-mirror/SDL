
FSLib Release 4
---------------

The FSLib is a library to be able to use fullscreen-switchable windows
on OS/2. The library uses the Scitech SNAP GA API, so it needs Scitech SNAP
installed to be able to use it.

The library itself can be compiled into a DLL. This has at least two advantages:
- If it needs updating and bugfixing, there is no need to recompile the 
  application which uses it.
- If the application crashes, the exception handler isntalled by the DLL will
  take care of restoring the desktop video mode.
  

Things to do before compiling
-----------------------------

To compile this, you'll need the followings installed:
- The OS/2 Developer's Toolkit
- The OpenWatcom compiler
- The Scitech SNAP SDK

The makefile assumes that you have the WATCOM and the OS2TK environmental
variables set to point to the OpenWatcom folder and the OS/2 Developer's
Toolkit folder, and you've also run the 'start-sdk.cmd' file of the Scitech
SNAP SDK, so those folders are also reachable.

To make your life easier, you can edit the second and third line of setvars.cmd
file to set the folders where the toolkit and the OW compiler is. If it's done,
all you have to do is to start the setvars.cmd file and then the start-sdk.cmd
file. If you do so, your environment should be set up to be able to compile the
library.


Compiling the library
---------------------

If everything is set up correctly, all you need is to run wmake.
This should create the FSLib.DLL and the corresponding FSLib.lib files.


Compiling the test app
----------------------

There is a small test application included in the 'test' folder. To compile
that one, please go to the test folder, and run wmake there!


Contacting the author
---------------------

If you have any questions, notes, recommendations, address them to
Doodle <doodle@scenergy.dfmk.hu>

