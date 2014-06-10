
#pragma strict

var cameraLeft : Camera;
var cameraRight : Camera;

var startTime = System.DateTime.UtcNow;
var attachedObject : GameObject = null;
var lookedatObject : GameObject = null;

// Find the two cameras
function Awake() {
  var cameras;
  var ovrCameraController : GameObject;
  ovrCameraController = GameObject.Find("OVRCameraController");
  cameras = ovrCameraController.GetComponentsInChildren(Camera);
  
  for (var camera : Camera in cameras)
  {
    if(camera.name == "CameraLeft"){
      cameraLeft = camera;
      }
    if(camera.name == "CameraRight"){
      cameraRight = camera;
      }
  }

  if((cameraLeft == null) || (cameraRight == null)){
    Debug.Log("WARNING: Unity Cameras in OVRCameraController not found!");
    }
}

function Update (){

  var camRightTransform : Transform = cameraRight.transform;
  var camLeftTransform : Transform = cameraLeft.transform;
  // forward is same vector for both cameras... pick one
  var forward = camRightTransform.forward;
  // choose position halfway between two cameras
  var position = (camRightTransform.position + camLeftTransform.position)/2;
  
  var ray = new Ray(position, forward);
  var hit : RaycastHit;
  var currentTime = System.DateTime.UtcNow;

  if (attachedObject == null) { 
    // Check to see if the ray case from the camera hit anything
    if(Physics.Raycast (ray, hit, 100)) {
      // If the ray cast has hit something check to see if it has been for longer than 2 seconds
      if((currentTime - startTime).TotalSeconds > 2){
        attachedObject = lookedatObject;
        attachedObject.rigidbody.useGravity = false;
        attachedObject.rigidbody.isKinematic = false;
        attachedObject.transform.parent = cameraRight.transform;
        attachedObject.renderer.material.color = Color.red;

      } else {

        // If less than 2 seconds make it the looked at object and turn it blue
        if (lookedatObject == null){
          lookedatObject = hit.collider.gameObject;
          lookedatObject.renderer.material.color = Color.blue;
        }
      }
    } else {
      // If the ray isn't hitting anything
      if (lookedatObject != null) {
        lookedatObject.renderer.material.color = Color.white;
      }
      startTime = currentTime;
      lookedatObject = null;
    } 
  } else { 
    // If the attached object is not null, check to see if it has been put down.
    if (attachedObject.renderer.material.color == Color.white){	
      startTime = currentTime;
      attachedObject = null;
    }	
  }	

}

