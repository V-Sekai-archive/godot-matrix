#ifndef MATRIXROOM_H
#define MATRIXROOM_H

#include "core/reference.h"
#include "core/ustring.h"

#include "matrix.h"

class MatrixClient;

class MatrixRoom : public Reference {
  friend class MatrixClient;
  GDCLASS(MatrixRoom,Reference);

  MatrixClient *client;

  String room_id;
  String prev_batch;

  Dictionary aliases; // HS name -> array of aliases

  Dictionary members;

  Dictionary tags;

  Set<String> event_ids; //all the events that have been received
  Array events;
  String backfill_prev_batch;

  void _put_event(Dictionary event); //insert new event
  void _put_old_event(Dictionary event); //insert old event (backwards in time)

  void _put_ephemeral_event(Dictionary event);
  Error _process_state_event(Dictionary event);

  void _state_sync(Variant userdata);

protected:
  static void _bind_methods();

public:

  String name;
  String topic;

  Error get_old_events(int amount);

  String get_name(bool sync=false);
  String get_friendly_name(bool sync=false);
  String get_topic(bool sync=false);
  Array get_events() const;
  Dictionary get_aliases() const;
  Dictionary get_members(bool sync=false);

  String get_member_display_name(String id, bool sync=false);

  Variant state_sync();

  Error send_text_message(String text);
  Error send_event(String event_type, Dictionary event);

  Error set_typing(bool typing, int timeout_ms);

  Variant leave_room();
  
  MatrixRoom();
  void init(MatrixClient *client, String room_id);
};

#endif
