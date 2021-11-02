extends Panel

export(String) var sender_name = "" setget set_sender_name
export(String) var message_text = "" setget set_message_text

func set_sender_name(name):
	if (has_node("sender")):
		get_node("sender").set_text(name)

func set_message_text(text):
	if (has_node("message")):
		get_node("message").set_text(text)

func _ready():
	set_sender_name(sender_name)
	set_message_text(message_text)
	
	get_node("message").connect("minimum_size_changed", self, "resized")

func resized():
	set_custom_minimum_size(Vector2(0, 50+get_node("message").get_custom_minimum_size().y-get_node("message").get_line_height()))
