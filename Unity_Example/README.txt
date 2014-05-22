UNITY PRO 4 EXAMPLES - README

The scripts and Unity package in this directory are for use with Chapter 12 of “Oculus Rift in Action.”  Chapter 12 provides all of the instructions you need to build a simple Rift-compatible scene using Unity Pro 4.2 or later and the Oculus Integration package. 

The completed examples are available in the OculusRiftinAction.unitypackage, however,  we recommend creating the scenes on your own using the scripts available here and the instructions in chapter 12 to fully understand the process. 


———————
SCRIPTS
———————
The Scripts directory contains all of the scripts used to create the examples in chapter 12 of Oculus Rift in Action excluding those available in the Oculus Rift Integration Package from Oculus VR.

Character Controller Scripts  
———————————————————————————
These are the first person controller scripts from the Unity Standard Assets that have been modified to work with the Oculus OVRCameraController Prefab.

- CharacterMotor.js- FPSInputController.js
- MouseLook.cs


Skybox Mesh Script
——————————————————
This script can be used to create a Rift-friendly skybox. Special thanks to Nora of http://stereoarts.jp for the SkyboxMesh.cs script.

- SkyboxMesh.cs



Moving Objects Using Rift Head Tracking
———————————————————————————————————————
These are the required scripts for creating the final example in chapter 12 that allows you to create a Unity scene where head tracking is used to move objects.
movegaze.js
cubecollision.js



————————————————
COMPLETE EXAMPLE - OculusRiftinAction.unitypackage
———————————————

The OculusRiftinAction.unitypackage package contains the completed Unity example scenes used in Chapter 12 of “Oculus Rift in Action.” This package requires Unity Pro 4.2 or later AND the Oculus Pro 4 Integration package, OculusUnityIntegration.unitypackage, from Oculus VR.

To use this package:

1. Create a new Unity project using Unity Pro 4.2 or later. (You do not need to add any of the standard assets.)

2. Download and unzip the “Oculus Pro 4 Integration” package from  the “Downloads” page  “0.2” panel at https://developer.oculusvr.com/.

3 Select Assets > Import Package > Custom Package and then select OculusUnityIntegration.unitypackage from the downloaded files and import all packages.

4. Select Assets > Import Package > Custom Package and then select OculusRiftinAction.unitypackage. Import all packages.

There are three scenes in this package. All scenes are in Project View: Assets > OculusRiftinAction > Scenes.

Beach_OvrPlayerController -  A simple beach scene with the Oculus OVRPlayerController prefab used to add Rift compatibility and navigation.

Beach_OvrCameraController - A simple beach scene with the OVRCameraController prefab used to add Rift compatibility. The navigation is a custom controller based on the Unity standard assets character controller.

Beach_movecrates - The same scene as Beach_OverCameraController but with “crates” added  that the user can select and move by looking at them. When looked at, a crate will turn blue and after 2 seconds the crate will turn red. The crate can then be moved using the Rift. To put the crate down, the crate needs to collide with the Beach.


