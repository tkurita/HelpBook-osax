HelpBook.osax
=============

HelpBook.osax is a scripting addition to register and display Help Books in any bundles in Help Viewer.

This project is deprecated, because scripting addtion don't work on macOS 10.14 or later.
For same pourpose, an AppleScript library [OpenHelpBook.scptd] has been developed.

[OpenHelpBook.scptd]: https://github.com/tkurita/OpenHelpBook.scptd

Applications for macOS have help files written by HTML in own bundle.
The help files is called Help Book and shown in Help Viewer.
This is smart way to provide application's helps because an application and its help files will never divorce.

HelpBook.osax allows followings through AppleScript.

Show a Help Book of an application without launching the application through AppleScript.
AppleScript's application bundles can have own help book and the help menu will be enabled.
AppleScript's script bundles can have own help book.

## Usage
English :
* https://www.script-factory.net/graveyard/osax/HelpBook-osax/en/index.html

Japanese :
* https://www.script-factory.net/graveyard/osax/HelpBook-osax/index.html

## Building
Reqirements :
* OS X 10.6 or later.
* Xcode

## Licence

Copyright &copy; 2006-2016 Tetsuro Kurita
Licensed under the [GPL license][GPL].
 
[GPL]: http://www.gnu.org/licenses/gpl.html

