UNITY PRO 4 EXAMPLES - README

The scripts and Unity package in this directory are for use with Chapter 7 and 8 of “Oculus Rift in Action.”  Chapter 7  provides all of the instructions you need to build a simple Rift-compatible scene using Unity Pro 4.6 or later and the Oculus Integration package version 0.4.4. Chapter 8 builds on those examples by adding a simple GUI.

The scripts and a base scene are available in the OculusRiftinAction.unitypackage. 


———————
SCRIPTS
———————
The Scripts directory contains all of the scripts used to create the examples in chapter 7  and 8 of Oculus Rift in Action excluding those available in the Oculus Rift Integration Package from Oculus VR.

Character Controller Scripts  
———————————————————————————
These are the first person controller scripts from the Unity 4.6  Standard Assets. The MouseLook Script has been modified to work with the Oculus OVRCameraRig Prefab.

- CharacterMotor.js
- FPSInputController.js
- MouseLook.cs

Getting user profile information
————————————————————————————————
This script is used to print the current profile name to the console log.

report.cs

GUI Scripts
——————————
These scripts are used to create a simple GUI for Rift applications.

ToggleMenu.cs
UpdateTimer.cs


Moving Objects Using Rift Head Tracking
———————————————————————————————————————
These are the required scripts for creating the example in chapter 8 that allows you to create a Unity scene where head tracking is used to move objects.

Movegaze.cs
Cubecollision.cs


————————————————
OculusRiftinAction.unitypackage
———————————————

The OculusRiftinAction.unitypackage package the base scene and the completed scenes:

Beach - The base scene used for all examples
Beach_OVRPlayerController - The base scene with the OVRPlayerController added
Beach_OVRCameraRig - The base scene with a player  using the OVRCameraRig with the Unity character controller, a simple GUI, and crates




