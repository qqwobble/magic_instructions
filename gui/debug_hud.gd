extends Control

func _process(_delta: float) -> void:
	($fps_label as Label).text = "FPS: {0}".format([Engine.get_frames_per_second()])
