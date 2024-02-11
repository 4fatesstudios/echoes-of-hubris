extends CharacterBody2D

const SPEED = 300.0
const JUMP_VELOCITY = -500.0
var lockY = true
var lockX = true
var keyPress = 0

@onready var cameraNode: Camera2D = get_node("/root/World/Camera2D")

@onready var anim = get_node("AnimationPlayer")
# Get the gravity from the project settings to be synced with RigidBody nodes.
var gravity = ProjectSettings.get_setting("physics/2d/default_gravity")

func _ready():
	cameraNode.position.x = position.x
	cameraNode.position.y = position.y - 100
	get_node("AnimatedSprite2D").play("idle")
	
func _process(delta):
	cameraNode.position.x = position.x
	
	if lockY:
		cameraNode.position.y = position.y
	if (velocity.x == 0) && (velocity.y == 0):
		if Input.is_action_pressed("ui_up") && is_on_floor():
			keyPress += delta
			print(keyPress)
			if keyPress > 2:
				cameraNode.position.y = position.y - 75
				lockY = false
		
		if Input.is_action_just_released("ui_up") && not Input.is_action_pressed("ui_down"):
			lockY = true
			keyPress = 0
			print("here")
		
		if Input.is_action_pressed("ui_down") && is_on_floor():
			keyPress += delta
			print(keyPress)
			if keyPress > 2:
				cameraNode.position.y = position.y + 75
				lockY = false
			
		if Input.is_action_just_released("ui_down") && not Input.is_action_pressed("ui_up"):
			lockY = true
			keyPress = 0
			print("here")
	else:
		keyPress = 0
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
