extends Node


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var exit := MagixVirtualMachine.run_tests()
	get_tree().quit(exit)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
