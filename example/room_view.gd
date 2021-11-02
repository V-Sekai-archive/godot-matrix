extends Panel

var preload_message = preload("res://message.tscn")

var scroll_to_bottom = true
var scrolled_to_bottom = true
var scrolled_to_top = false

var backfill_thread

var room setget set_room
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

func clear():
	get_node("room_name").set_text("")
	get_node("room_topic").set_text("")
	get_node("typing").set_text("")
	get_node("messageboxcontainer/messagebox").set_text("")
	for c in get_node("messages/list").get_children():
		get_node("messages/list").remove_child(c)
	if room:
		room.disconnect("ephemeral_event", self, "ephemeral_event")
		room.disconnect("timeline_event", self, "timeline_event")
		room.disconnect("old_timeline_event", self, "old_timeline_event")
		room.disconnect("state_event", self, "state_event")
	
func _ready():
	get_node("messages/_v_scroll").connect("changed", self, "scrollbar_changed")
	get_node("messages/_v_scroll").connect("value_changed", self, "scrollbar_value_changed")
	get_node("leave_button").connect("button_up", self, "leave_room")

func leave_room():
	room.leave_room()

func scrollbar_changed(v):
	var scrollbar = get_node("messages/_v_scroll")
	if scrollbar.page >= get_node("messages/list").get_minimum_size().y:
		scrolled_to_bottom = true
	if scroll_to_bottom:
		scrollbar.set_value(scrollbar.max_value-scrollbar.page)
		scroll_to_bottom = false
	if scrolled_to_bottom:
		scrollbar.set_value(scrollbar.max_value-scrollbar.page)

func scrollbar_value_changed(v):
	var scrollbar = get_node("messages/_v_scroll")
	scrolled_to_bottom = (scrollbar.max_value == 0 or scrollbar.value == scrollbar.max_value-scrollbar.page)
	scrolled_to_top = scrollbar.value == 0

func fetch_backfill(args=null):
	while true:
		var scrollbar = get_node("messages/_v_scroll")
		if room:
			var dist_from_bottom = scrollbar.max_value-scrollbar.page-scrollbar.value
			while scrollbar.page >= get_node("messages/list").get_minimum_size().y:
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
		get_node("messages/list").add_child(event)
		if (scrolled_to_bottom):
			scroll_to_bottom = true
	else:
		get_node("messages/list").add_child(event)
		get_node("messages/list").move_child(event, 0)
		yield(get_node("messages/_v_scroll"), "changed")

func __get_name(user_id):
	return room.get_member_display_name(user_id, true)

func timeline_event(event):
	process_timeline_event(event, false)

func old_timeline_event(event):
	process_timeline_event(event, true)

func send_message(text=""):
	if (get_node("messageboxcontainer/messagebox").text != ""):
		room.send_text_message(get_node("messageboxcontainer/messagebox").text)
		get_node("messageboxcontainer/messagebox").clear()

func ephemeral_event(event):
	if (event['type'] == "m.typing"):
		if (event['content']['user_ids'].size() == 0):
			get_node("typing").text = ""
		elif (event['content']['user_ids'].size() == 1):
			get_node("typing").text = event['content']['user_ids'][0] + " is typing"
		else:
			var typing_text = String(event['content']['user_ids'])
			typing_text = typing_text.substr(1, typing_text.length()-2)
			
			get_node("typing").text = typing_text + " are typing"

func state_event(event):
	get_node("room_name").set_text(room.get_name(false))
	get_node("room_topic").set_text(room.get_topic(false))
