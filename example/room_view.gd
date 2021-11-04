extends PanelContainer

var preload_message = preload("res://message.tscn")

var scroll_to_bottom = true
var scrolled_to_bottom = true
var scrolled_to_top = false

var backfill_thread

var room: MatrixRoom setget set_room

onready var room_name_label: Label = find_node("ROOM_NAME")
onready var room_topic_label: Label = find_node("ROOM_TOPIC")
onready var typing_indicator: Label = find_node("TYPING_INDICATOR")
onready var message_box: LineEdit = find_node("MESSAGE_INPUT")
onready var message_list: VBoxContainer = find_node("MESSAGE_LIST")
onready var messages_scroller_box: ScrollContainer = find_node("MESSAGES")

func set_room(r, c=true):
	if (c):
		clear()
	room = r
	#backfill_thread = Thread.new()
	#backfill_thread.start(self, "fetch_backfill")
	room.connect("ephemeral_event", self, "ephemeral_event")
	room.connect("timeline_event", self, "timeline_event")
	room.connect("old_timeline_event", self, "old_timeline_event")
	room.connect("state_event", self, "state_event")
	room.state_sync().wait_to_finish()
	for event in room.get_events():
		process_timeline_event(event)
	find_node("room_meta_line").visible = true
	find_node("message_input_container").visible = true

func clear():
	for thing_to_clear in [room_name_label, room_topic_label, typing_indicator, message_box]:
		thing_to_clear.text = ""
	for c in message_list.get_children():
		message_list.remove_child(c)
	find_node("room_meta_line").visible = false
	find_node("message_input_container").visible = false
	if room:
		room.disconnect("ephemeral_event", self, "ephemeral_event")
		room.disconnect("timeline_event", self, "timeline_event")
		room.disconnect("old_timeline_event", self, "old_timeline_event")
		room.disconnect("state_event", self, "state_event")
	
func _ready():
	messages_scroller_box.get_v_scrollbar().connect("changed", self, "scrollbar_changed")
	messages_scroller_box.get_v_scrollbar().connect("value_changed", self, "scrollbar_value_changed")
	find_node("leave_button").connect("button_up", self, "leave_room")
	clear()

func leave_room():
	room.leave_room()

func scrollbar_changed():
	var scrollbar = messages_scroller_box.get_v_scrollbar()
	if scrollbar.page >= message_list.get_minimum_size().y:
		scrolled_to_bottom = true
	if scroll_to_bottom:
		scrollbar.set_value(scrollbar.max_value-scrollbar.page)
		scroll_to_bottom = false
	if scrolled_to_bottom:
		scrollbar.set_value(scrollbar.max_value-scrollbar.page)

func scrollbar_value_changed(v):
	var scrollbar = messages_scroller_box.get_v_scrollbar()
	scrolled_to_bottom = (scrollbar.max_value == 0 or scrollbar.value == scrollbar.max_value-scrollbar.page)
	scrolled_to_top = scrollbar.value == 0

func fetch_backfill(args=null):
	while true:
		var scrollbar = messages_scroller_box.get_v_scrollbar()
		if room:
			var dist_from_bottom = scrollbar.max_value-scrollbar.page-scrollbar.value
			while scrollbar.page >= message_list.get_minimum_size().y:
				room.get_old_events(1)
				yield(scrollbar, "changed")
				scrollbar.set_value(scrollbar.max_value-scrollbar.page)
			if scrollbar.value == 0:
				room.get_old_events(5)
				yield(scrollbar, "changed")
				scrollbar.set_value(dist_from_bottom)
		yield(scrollbar, "value_changed")

func has_room():
	if (room):
		return true
	return false

func process_timeline_event(e, old=false):
	if (e['type'] == "m.room.message" and e.has('content') and e['content'].has('body')):
		var message = preload_message.instance(true)
		add_event(message, old)
		message.set_sender_name(self.__get_name(e['sender']))
		print(e['content']['body'].length())
		message.set_message_text(e['content']['body'])
	elif (e['type'] == "m.room.name"):
		var message = preload_message.instance(true)
		add_event(message, old)
		message.set_sender_name(self.__get_name(e['sender']))
		message.set_message_text("changed the room name to "+e['content']['name'])
	elif (e['type'] == "m.room.topic"):
		var message = preload_message.instance(true)
		add_event(message, old)
		message.set_sender_name(self.__get_name(e['sender']))
		message.set_message_text("changed the room topic to "+e['content']['topic'])
	else:
		pass

func add_event(event, old):
	if not old:
		message_list.add_child(event)
		if (scrolled_to_bottom):
			scroll_to_bottom = true
	else:
		message_list.add_child(event)
		message_list.move_child(event, 0)
		yield(messages_scroller_box.get_v_scrollbar(), "changed")

func __get_name(user_id):
	return room.get_member_display_name(user_id, true)

func timeline_event(event):
	process_timeline_event(event, false)

func old_timeline_event(event):
	process_timeline_event(event, true)

func send_message(text=""):
	if (message_box.text != ""):
		room.send_text_message(message_box.text)
		message_box.clear()

func ephemeral_event(event):
	if (event['type'] == "m.typing"):
		if (event['content']['user_ids'].size() == 0):
			typing_indicator.text = ""
		elif (event['content']['user_ids'].size() == 1):
			typing_indicator.text = event['content']['user_ids'][0] + " is typing"
		else:
			var typing_text = String(event['content']['user_ids'])
			typing_text = typing_text.substr(1, typing_text.length()-2)
			
			typing_indicator.text = typing_text + " are typing"

func state_event(event):
	room_name_label.text = room.get_name(false)
	room_topic_label.text = room.get_topic(false)
