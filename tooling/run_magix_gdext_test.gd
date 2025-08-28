extends Node

func _ready() -> void:
	var exit_code := MagixVirtualMachine.run_tests()
	# Printing to console is fine
	# The debugger however quits too early sometimes. This prevents this.
	await get_tree().create_timer(0.3).timeout
	get_tree().quit(exit_code)
