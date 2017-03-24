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

  Array events;
  int event_history_limit = 20;

  void _put_event(Dictionary event);
  void _put_ephemeral_event(Dictionary event);
  Error _process_state_event(Dictionary event);

protected:
  static void _bind_methods();

public:

  String name;
  String topic;

  int get_event_history_limit();
  void set_event_history_limit(int limit);

  String get_name() const;
  String get_topic() const;
  Array get_events() const;
  Dictionary get_aliases() const;

  Error send_text_message(String text);

  MatrixRoom();
  void init(MatrixClient *client, String room_id);
};

#endif
