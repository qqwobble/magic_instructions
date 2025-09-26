extends RigidBody3D

@export var move_state : MoveState = MoveState.EXTERNAL
@export var mouse_sensitivity : Vector2 = Vector2(0.0013,0.0013)

enum MoveState {
	EXTERNAL,
	DEACTIVATED,
	RAGDOLL,
	FREE,
	SLIDE,
}

@onready var floor_detect : ShapeCast3D = $floor_detect
@onready var head_attach : Node3D = $head_attachment
@onready var animplayer : AnimationPlayer = $animation_player
@onready var approach_floor : RayCast3D = $approach_floor

func _enter_tree() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

var mouse_delta : Vector2
func _input(event: InputEvent) -> void:
	var watch_mouse := false
	match move_state:
		MoveState.FREE, MoveState.SLIDE:
			watch_mouse = true

	if watch_mouse and event is InputEventMouseMotion:
		var mouse := event as InputEventMouseMotion
		mouse_delta += mouse.relative

var accum_look_angle : float
const MAX_LOOK_ANGLE := PI * 0.5
func correct_look(state: PhysicsDirectBodyState3D) -> void:
	var tf := state.transform
	var move := (mouse_delta) * 0.75 * mouse_sensitivity
	mouse_delta *= 0.25
	tf = tf.rotated_local(Vector3.UP, -move.x)
	accum_look_angle = clampf(accum_look_angle - move.y, -MAX_LOOK_ANGLE, MAX_LOOK_ANGLE)
	head_attach.rotation.x = accum_look_angle

	state.transform = tf

func enter_free(_state: PhysicsDirectBodyState3D) -> void:
	move_state = MoveState.FREE
	animplayer.play(&"ExitSlide")

func clip_towards_floor(state: PhysicsDirectBodyState3D) -> void:
	if approach_floor.is_colliding():
		var col_rid := approach_floor.get_collider_rid()
		var col_state := PhysicsServer3D.body_get_direct_state(col_rid)
		var global_col := approach_floor.get_collision_point()
		var local_col := col_state.transform.inverse() * global_col
		var local_speed := col_state.get_velocity_at_local_position(local_col)
		var n := approach_floor.get_collision_normal()
		var rel_speed := (state.linear_velocity - local_speed).project(n)
		var rel_distance := (global_col - state.transform.origin).project(n) * 0.9
		if rel_speed.length_squared() > 0.01 and rel_speed.dot(rel_distance) > 0.0:
			if (rel_speed * state.step).length_squared() > rel_distance.length_squared():
				state.linear_velocity += -rel_speed + rel_distance / state.step

@export var FREE_SPEED := 5.2
@export var FREE_JUMP := 9.2
func move_free(state: PhysicsDirectBodyState3D) -> void:
	var grounded := floor_detect.is_colliding()

	if Input.is_action_pressed(&"slide") and grounded:
		enter_slide(state)
		return move_slide(state)

	correct_look(state)

	var tf := state.transform
	var bs := tf.basis
	var right := bs.x
	var up := bs.y
	var forward := -bs.z

	var input_dir := Input.get_vector(&"left", &"right", &"back", &"forward")
	var input_motion := right * input_dir.x + forward * input_dir.y
	var speed_updown := state.linear_velocity.project(up)
	var speed_planar := state.linear_velocity - speed_updown
	var target_vel := input_motion * FREE_SPEED

	if grounded:
		var approach := target_vel.lerp(speed_planar, exp(-state.step * 10))
		state.linear_velocity = approach + speed_updown
	else:
		if target_vel.dot(speed_planar) < 0.0 or target_vel.length_squared() > speed_planar.length_squared():
			var approach := target_vel.lerp(speed_planar, exp(-state.step * 3.1))
			state.linear_velocity = approach + speed_updown

	if grounded and Input.is_action_just_pressed(&"jump"):
		state.apply_central_impulse(up * FREE_JUMP)
		($caster/cast_lmb as MagixCastSlot).cast_spell(MagixVm as MagixVirtualMachine, "entry")

	clip_towards_floor(state)

	state.integrate_forces()

func enter_slide(_state: PhysicsDirectBodyState3D) -> void:
	move_state = MoveState.SLIDE
	animplayer.play(&"ToSlide")


@export var SLIDE_JUMP : float = 6.5
@export var SLIDE_JACCEL : float = 1.5
func move_slide(state: PhysicsDirectBodyState3D) -> void:
	if not Input.is_action_pressed(&"slide"):
		enter_free(state)
		return move_free(state)

	correct_look(state)

	var bs := state.transform.basis
	var right := bs.x
	var up := bs.y
	var forward := -bs.z

	var grounded := floor_detect.is_colliding()
	var input_dir := Input.get_vector(&"left", &"right", &"back", &"forward")
	var input_motion := right * input_dir.x + forward * input_dir.y

	if grounded and Input.is_action_just_pressed(&"jump"):
		var impulse := up * SLIDE_JUMP + input_motion * SLIDE_JACCEL
		state.apply_central_impulse(impulse)
		enter_free(state)
	else:
		# normal slide just does not slow down
		# this is just to avoid penetrating floors
		clip_towards_floor(state)

	state.integrate_forces()

func move_ragdoll(state: PhysicsDirectBodyState3D) -> void:
	state.linear_velocity *= exp(-0.8 * state.step)
	state.angular_velocity *= exp(-0.8 * state.step)
	state.integrate_forces()

func _integrate_forces(state: PhysicsDirectBodyState3D) -> void:
	match move_state:
		MoveState.FREE:
			move_free(state)
		MoveState.SLIDE:
			move_slide(state)
		MoveState.RAGDOLL:
			move_ragdoll(state)
		MoveState.EXTERNAL, MoveState.DEACTIVATED:
			pass
		var unknown:
			assert(false, "Unknown movement state: {}".format(unknown))
