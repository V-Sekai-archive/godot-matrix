extends Node

#var room_view = preload("res://roomview.tscn")
#var room_views = {}

func _ready():
	print ("trying to connect to: ", $Control/MatrixClient.hs_name)
	get_node("Control/MatrixClient").login("coolperson", "cool")
	get_node("Control/MatrixClient").start_listening()
	#get_node("client view/room view").set_room(get_node("MatrixClient").get_rooms()["!RtOeFVSpNUJHGLYYTh:vurpo.fi"])
	
	var tree = get_node("Control/client view/room list/Tree")
	tree.set_select_mode(Tree.SELECT_SINGLE)
	var root = tree.create_item()
	tree.set_hide_root(true)
	var rooms = tree.create_item(root)
	rooms.set_text(0, "Rooms")
	
	for room_id in get_node("Control/MatrixClient").get_rooms():
		var room_ = tree.create_item(rooms)
		room_.set_text(0, get_node("Control/MatrixClient").get_rooms()[room_id].get_friendly_name(true))
		room_.set_metadata(0, room_id)
	
	tree.connect("cell_selected", self, "cell_selected")

func cell_selected():
	var tree = get_node("Control/client view/room list/Tree")
	if (tree.get_selected().get_parent().get_text(0) == "Rooms"):
		#var room = room_view.instance(true)
		#get_node("client view").get_children()[1].replace_by(room)#_views[tree.get_selected().get_metadata(0)])
		var room = get_node("Control/client view/Room view")
		room.set_room(get_node("Control/MatrixClient").get_rooms()[tree.get_selected().get_metadata(0)])
		room.room.state_sync()
