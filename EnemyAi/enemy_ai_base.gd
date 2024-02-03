class_name enemyAiBase

extends CharacterBody2D

# Get the gravity from the project settings to be synced with RigidBody nodes.
var gravity = ProjectSettings.get_setting("physics/2d/default_gravity")

const SPEED = 80.0
const JUMP_VELOCITY = -400.0
var direction = 1
var chase = false
var player

# A signal to notify when the character takes damage
signal took_damage(damage_amount)

func attack():
	pass

func take_damage(amount):
	pass
	

func _physics_process(delta):
	velocity.y += gravity * delta
	var playerNode = get_node("/root/World/PlayerNode/Player")
	if chase == true && playerNode != null:
		get_node("AnimatedSprite2D").play("jump")
		var directionToPlayer = (playerNode.position - self.position).normalized()
		if (directionToPlayer.x > 0):
			get_node("AnimatedSprite2D").flip_h = true
		else:
			get_node("AnimatedSprite2D").flip_h = false
		velocity.x = directionToPlayer.x * SPEED
	else:
		get_node("AnimatedSprite2D").play("idle")
		velocity.x = 0
	
	move_and_slide()
