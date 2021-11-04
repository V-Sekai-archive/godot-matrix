extends HSplitContainer

export var initial_tree_area_ratio = 0.3

func _ready():
	split_offset = - get_viewport_rect().size.x * (1 - initial_tree_area_ratio)
