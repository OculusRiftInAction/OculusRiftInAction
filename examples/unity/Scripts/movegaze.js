
#pragma strict

var startTime = System.DateTime.UtcNow;
var attachedObject : GameObject = null;
var lookedatObject : GameObject = null;



function Update (){

  var ray = new Ray(this.transform.position, this.transform.forward);
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
        attachedObject.transform.parent = this.transform;
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

