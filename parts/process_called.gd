extends Node

signal process_called(delta : float)
signal physics_process_called(delta : float)

func _process(delta: float) -> void:
	process_called.emit(delta)

func _physics_process(delta: float) -> void:
	physics_process_called.emit(delta)
