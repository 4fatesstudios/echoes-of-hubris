extends CharacterBody2D


const SPEED = 300.0
const JUMP_VELOCITY = -500.0
var lockY = true
var lockX = true

@onready var cameraNode: Camera2D = get_node("/root/World/Camera2D")

@onready var anim = get_node("AnimationPlayer")
# Get the gravity from the project settings to be synced with RigidBody nodes.
var gravity = ProjectSettings.get_setting("physics/2d/default_gravity")


func _ready():
	cameraNode.position.x = position.x
	cameraNode.position.y = position.y - 100
	get_node("AnimatedSprite2D").play("idle")
	
func _process(delta):
	if not lockY:
		cameraNode.position.y = position.y
	else:
		cameraNode.position.y = position.y + 5000
	cameraNode.position.x = position.x
	

func _physics_process(delta):
	# Add the gravity.
	if not is_on_floor():
		velocity.y += gravity * delta

	# Handle jump.
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = JUMP_VELOCITY
		anim.play("jump")

	# Get the input direction and handle the movement/deceleration.
	# As good practice, you should replace UI actions with custom gameplay actions.
	var direction = Input.get_axis("ui_left", "ui_right")
	if (direction < 0):
		get_node("AnimatedSprite2D").flip_h = true
		velocity.x = direction * SPEED
		if (velocity.y == 0):
			anim.play("run")
	elif (direction > 0):
		get_node("AnimatedSprite2D").flip_h = false
		velocity.x = direction * SPEED
		if (velocity.y == 0):
			anim.play("run")
	else:
		if (velocity.y == 0):
			anim.play("idle")
		velocity.x = move_toward(velocity.x, 0, SPEED)
	
	if (velocity.y > 0):
		anim.play("fall")
		
	move_and_slide()
	
func _on_camera_lock_y_body_entered(body):
	lockY = true

func _on_camera_unlock_y_body_entered(body):
	lockY = false
	
func _on_camera_unlock_x_area_shape_entered(area_rid, area, area_shape_index, local_shape_index):
	lockX = false

func _on_camera_lock_x_body_entered(body):
	lockX = true
