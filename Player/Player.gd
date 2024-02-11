extends CharacterBody2D

const SPEED = 300.0
const JUMP_VELOCITY = -500.0
var camPositionY
var lockY = true
var lockX = true

@onready var cameraNode: Camera2D = get_node("/root/World/Camera2D")

@onready var anim = get_node("AnimationPlayer")
# Get the gravity from the project settings to be synced with RigidBody nodes.
var gravity = ProjectSettings.get_setting("physics/2d/default_gravity")

func _ready():
	cameraNode.position.x = position.x
	cameraNode.position.y = position.y - 100
	camPositionY = cameraNode.position.y
	get_node("AnimatedSprite2D").play("idle")
	
func _process(delta):
	cameraNode.position.x = position.x
	
	if lockY:
		cameraNode.position.y = position.y

	if Input.is_action_pressed("ui_up") && (velocity.x == 0) && (velocity.y == 0):
		cameraNode.position.y = position.y - 75
		lockY = false

	if Input.is_action_just_released("ui_up"):
		lockY = true
	
	if Input.is_action_pressed("ui_down") && (velocity.x == 0) && (velocity.y == 0):
		cameraNode.position.y = position.y + 75
		lockY = false
		
	if Input.is_action_just_released("ui_down"):
		lockY = true
	

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
