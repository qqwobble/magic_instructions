@tool
extends EditorPlugin

func wait_pipe(file : FileAccess) -> void:
	while true:
		var err = file.get_error()
		if err != OK:
			if err != ERR_FILE_CANT_READ:
				# seems to be ok
				print("pipe printer failed with:", error_string(err))
			return
		if file.eof_reached():
			return
		else:
			var line := file.get_line()
			if line.length() > 0:
				print(line)

func run_and_report(app : String, args : PackedStringArray) -> int:
	var exec := OS.execute_with_pipe(app, args, true)
	var stdio : FileAccess = exec["stdio"]
	var stderr : FileAccess = exec["stderr"]
	var pid : int = exec["pid"]

	var t1 := Thread.new()
	t1.start(wait_pipe.bind(stdio))
	var t2 := Thread.new()
	t2.start(wait_pipe.bind(stderr))
	while OS.is_process_running(pid):
		await get_tree().create_timer(0.1).timeout
	t1.wait_to_finish()
	t2.wait_to_finish()
	return OS.get_process_exit_code(pid)

func gdextensions_default_template(platform : String, invoc : PackedStringArray):
	if platform == "windows":
		invoc.push_back("use_mingw=yes")
		invoc.push_back("use_llvm=no")

func gdextensions_build(configs : Array[Dictionary], targets : PackedStringArray = []):
	var exec := ""
	var args : PackedStringArray
	if OS.get_name() == "Windows":
		# This calls into wsl
		# The proper thing to do would be to use the proper godot-build-containers
		# BUT... I can't be bothered to learn docker right now.
		exec = "wsl"
		var gdext_cpp_dir := ProjectSettings.globalize_path("res://gdextension/cpp")
		args = PackedStringArray(["--cd", gdext_cpp_dir ,"--", "scons"])
	else:
		printerr("Unknown platform. Unable to build")
		return false

	for conf in configs:
		var invoc := args.duplicate()
		gdextensions_default_template(conf["platform"], invoc)
		for key in conf:
			invoc.push_back("%s=%s" % [key, conf[key]])
		invoc.append_array(targets)
		if await run_and_report(exec, invoc) != 0:
			return false

	return true

func gdextensions_build_all() -> bool:
	var configs : Array[Dictionary] = [
		{
			"platform" : "windows",
			"arch" : "x86_64",
			"target" : "template_debug",
		},
		{
			"platform" : "windows",
			"arch" : "x86_64",
			"target" : "template_release",
		},
		{
			"platform" : "linux",
			"arch" : "x86_64",
			"target" : "template_debug",
		},
		{
			"platform" : "linux",
			"arch" : "x86_64",
			"target" : "template_release",
		}
	]
	return await gdextensions_build(configs)

func gdextensions_compile_commands() -> bool:
	var configs : Array[Dictionary] = [
		{
			"platform" : "linux",
			"arch" : "x86_64",
			"target" : "template_debug",
		},
	]
	return await gdextensions_build(configs, ["compile_commands.json"])

func gdextensions_build_host() -> bool:
	var arch : String
	for a in ["x86_64"]:
		if OS.has_feature(a):
			arch = a
	if arch == "":
		printerr("Failed to find host architecture...")
		return false
	var configs : Array[Dictionary] = [
		{
			"platform" : OS.get_name().to_lower(),
			"arch" : arch,
			"target" : "template_debug",
		},
	]
	return await gdextensions_build(configs)

func gdextensions_regen_docs():
	var godot := OS.get_executable_path()
	var proj_dir := ProjectSettings.globalize_path("res://")
	var out = []
	var exit_code := OS.execute(godot, ["--project", proj_dir, "--doctool", "gdextension", "--gdextension-docs"], out, true, true)
	for x in out:
		print(x)


func _enter_tree() -> void:
	add_tool_menu_item("Rebuild all GDExtensions", gdextensions_build_all)
	add_tool_menu_item("Rebuild host GDExtensions", gdextensions_build_host)
	add_tool_menu_item("Rebuild compile_commands.json", gdextensions_compile_commands)
	add_tool_menu_item("Regen GDExtensions docs", gdextensions_regen_docs)

func _exit_tree() -> void:
	remove_tool_menu_item("Rebuild all GDExtensions")
	remove_tool_menu_item("Rebuild host GDExtensions")
	remove_tool_menu_item("Rebuild compile_commands.json")
	remove_tool_menu_item("Regen GDExtensions docs")

func _build() -> bool:
	# too slow for regular use
	#if not await gdextensions_build_host():
		#return false
	return true
