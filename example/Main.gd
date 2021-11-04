extends Node

onready var m_client = $MatrixClient
#onready var room_tree =  $"client view/room list/Tree"
onready var room_tree =  find_node("ROOM_TREE")
onready var room_view =  find_node("ROOM_VIEW")

func _ready():
	print ("trying to connect to: ", m_client.hs_name)
	m_client.login("coolperson", "cool")
	m_client.start_listening()
	
	room_tree.set_select_mode(Tree.SELECT_SINGLE)
	var root = room_tree.create_item()
	room_tree.set_hide_root(true)
	var rooms = room_tree.create_item(root)
	rooms.set_text(0, "Rooms")
	
	for room_id in m_client.get_rooms():
		var room_ = room_tree.create_item(rooms)
		room_.set_text(0, m_client.get_rooms()[room_id].get_friendly_name(true))
		room_.set_metadata(0, room_id)
	
	room_tree.connect("cell_selected", self, "roomlist_room_selected")

func roomlist_room_selected():
	print("roomlist_room_selected called")
	var selected_item = room_tree.get_selected()
	if (selected_item.get_parent().get_text(0) == "Rooms"):
		room_view.set_room(m_client.get_rooms()[selected_item.get_metadata(0)])
		room_view.room.state_sync()
