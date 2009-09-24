SDL Mac OS X Developer Notes:
	This is an optional developer package to provide extras that an 
	SDL developer might benefit from.
	
	Make sure you have already installed the SDL.framework 
	from the SDL.dmg. 
	
	For more complete documentation, please see READMEs included 
	with the  SDL source code. Also, don't forget about the API 
	documentation (also included with this package).


This package contains:
- SDL API Documentation
- A variety of SDLMain and .Nib files to choose from
- Xcode/Project Builder project templates


SDL API Documentation:
	We include both the HTML documentation and the man files. For 
	the HTML documentation, we have previously been installing to 
	/Develoepr/Documentation, but Apple explicitly says this is not 
	intended for 3rd party documentation. Xcode installs/updates 
	reserve the right and have been known to completely purge the 
	/Developer directory before installation, so if you place stuff 
	there, be aware that it could be deleted.
	(Xcode 3.0 introduces support for 3rd-party documentation, but
	we are still investigating the details.)


SDLMain:
	We include several different variations of SDLMain and the 
	.Nibs. (Each of these are demonstrated by the different PB/Xcode 
	project templates.) You get to pick which one you want to use, 
	or you can write your own to meet your own specific needs. We do 
	not currently provide a libSDLMain.a. You can build it yourself
	once you decide which one you want to use though it is easier and 
	recommended in the SDL FAQ that you just copy the SDLMain.h and 
	SDLMain.m directly into your project. If you are puzzled by this, 
	we strongly recommend you look at the different PB/Xcode project 
	templates to understand their uses and differences. (See Project 
	Template info below.) Note that the "Nibless" version is the same 
	version of SDLMain we include the the devel-lite section of the 
	main SDL.dmg.
	
	
Xocde Project Templates:
	For convenience, we provide Project Templates for Xcode. 
	Using Xcode is *not* a requirement for using 
	the SDL.framework. However, for newbies, we do recommend trying 
	out the Xcode templates first (and work your way back to raw gcc 
	if you desire), as the Xcode templates try to setup everything
	for you in a working state. This avoids the need to ask those 
	many reoccuring questions that appear on the mailing list 
	or the SDL FAQ.


	We have provided 3 different kinds of SDL templates for Xcode and have 
	a different set of templates for each version of Xcode (which generally 
	correspond with a particular Mac OS X version). 
	The installion directory depends on which version of Xcode you have.
	(Note: These directories may not already exist on your system so you must create them yourself.)

	For Leopard and Snow Leopard (Xcode 2.5, 3+), we recommend you install to:
	/Library/Application Support/Developer/Shared/Xcode/Project Templates/Application

	For Xcode 1.0 to 2.4,
	/Library/Application Support/Apple/Developer Tools/Project Templates/Appllcation 


	Also note you may place it in per-user locations, e.g.
	~/Library/Application Support/Developer/Shared/Xcode/Project Templates/Application

	
	And for advanced users who have multiple versions of Xcode installed on a single system,
	you may put each set in a directory with the Xcode version number instead of using "Shared", e.g.
	/Library/Application Support/Developer/2.5/Xcode/Project Templates/Application
	/Library/Application Support/Developer/3.1/Xcode/Project Templates/Application
	/Library/Application Support/Developer/3.2/Xcode/Project Templates/Application


	Copy each of the SDL/Xcode template directories into the correct location (e.g. "SDL OpenGL Application").
	Do not copy our enclosing folder into the location (e.g. TemplatesForXcodeSnowLeopard).
	So for example, in:
	/Library/Application Support/Developer/Shared/Xcode/Project Templates/Application
	you should have the 3 folders:
	SDL Application
	SDL Cocoa Application
	SDL OpenGL Application


	After doing this, when doing a File->New Project, you will see the 
	projects under the Application category.
	(Newer versions of Xcode have a separate section for User Templates and it will 
	appear in the Application category of the User Templates section.)



	How to create a new SDL project:

	1. Open Xcode (or Project Builder)
	2. Select File->New Project
	3. Select SDL Application
	4. Name, Save, and Finish
	5. Add your sources.
	*6. That's it!

	* If you installed the SDL.framework to $(HOME)/Library/Frameworks 
	instead of /Library/Frameworks, you will need to update the 
	location of the SDL.framework in the "Groups & Files" browser.
	

	The project templates we provide are:
	- SDL Application
		This is the barebones, most basic version. There is no 
		customized .Nib file. While still utilizing Cocoa under 
		the hood, this version may be best suited for fullscreen 
		applications.

	- SDL Cocoa Application
		This demonstrates the integration of using native 
		Cocoa Menus with an SDL Application. For applications
		designed to run in Windowed mode, Mac users may appreciate 
		having access to standard menus for things
		like Preferences and Quiting (among other things).
		
	- SDL OpenGL Application
		This reuses the same SDLMain from the "SDL Application" 
		temmplate, but also demonstrates how to 
		bring OpenGL into the mix.


Special Notes:
Only the 10.6 Snow Leopard templates (and later) will include 64-bit in the Universal Binary as 
prior versions of OS X lacked the API support SDL requires for 64-bit to work correctly.
To prevent 64-bit SDL executables from being launched on 10.5 Leopard, a special key has been set 
in the Info.plist in our Snow Leopard SDL/Xcode templates.


Xcode Tips and Tricks:

- Building from command line
	Use the command line tool: xcodebuild (see man page)
		 
- Running your app
    You can send command line args to your app by either 
	invoking it from the command line (in *.app/Contents/MacOS) 
	or by entering them in the "Executables" panel of the target 
	settings.
	
- Working directory
    As defined in the SDLMain.m file, the working directory of 
    your SDL app is by default set to its parent. You may wish to 
    change this to better suit your needs.



Additional References:

 - Screencast tutorials for getting started with OpenSceneGraph/Mac OS X are 
 	available at:
	http://www.openscenegraph.org/projects/osg/wiki/Support/Tutorials/MacOSXTips
	Though these are OpenSceneGraph centric, the same exact concepts apply to 
	SDL, thus the videos are recommended for everybody getting started with
	developing on Mac OS X. (You can skim over the PlugIns stuff since SDL
	doesn't have any PlugIns to worry about.)


Partial History:
2009-09-21 - CustomView template project was removed because it was broken by 
	the removal of legacy Quicktime support while moving to 64-bit.
	ProjectBuilder templates were removed.
	Tiger, Leopard, and Snow Leopard Xcode templates were introduced instead of 
	using a single common template due to the differences between the 3.
	(Tiger used a chevron marker for substitution while Leopard/Snow Leopard use ___
	and we need the 10.6 SDK for 64-bit.)

2007-12-30 - Updated documentation to reflect new template paths in Leopard
	Xcode. Added reference to OSG screencasts.

2006-03-17 - Changed the package format from a .pkg based 
	installer to a .dmg to avoid requiring administrator/root 
	to access contents, for better transparency, and to allow 
	users to more easily control which components 
	they actually want to install. 
	Introduced and updated documentation.
	Created brand new Xcode project templates for Xcode 2.1 
	based on the old Project Builder templates as they 
	required Xcode users to "Upgrade to Native Target". The new 
	templates try to leveage more default options and leverage 
	more Xcode conventions. The major change that may introduce 
	some breakage is that I now link to the SDL framework
	via the "Group & Files" browser instead of using build 
	options. The downside to this is that if the user 
	installs the SDL.framework to a place other than 
	/Library/Frameworks (e.g. $(HOME)/Library/Frameworks),
	the framework will not be found to link to and the user 
	has to manually fix this. But the upshot is (in addition to 
	being visually displayed in the forefront) is that it is 
	really easy to copy (embed) the framework automatically 
	into the .app bundle on build. So I have added this 
	feature, which makes the application potentially 
	drag-and-droppable ready. The Project Builder templates 
	are mostly unchanged due to the fact that I don't have 
	Project Builder. I did rename a file extension to .pbxproj 
	for the SDL Custom Cocoa Application template because 
	the .pbx extension would not load in my version of Xcode.
	For both Project Builder and Xcode templates, I resync'd
	the SDLMain.* files for the SDL App and OpenGL App 
	templates. I think people forget that we have 2 other 
	SDLMain's (and .Nib's) and somebody needs to go 
	through them and merge the new changes into those.
	I also wrote a fix for the SDL Custom Cocoa App 
	template in MyController.m. The sprite loading code 
	needed to be able to find the icon.bmp in the .app
	bundle's Resources folder. This change was needed to get 
	the app to run out of the box. This might change is untested 
	with Project Builder though and might break it.
	There also seemed to be some corruption in the .nib itself.
	Merely opening it and saving (allowing IB to correct the
	.nib) seemed to correct things.
	(Eric Wing)




