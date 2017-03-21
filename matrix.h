#ifndef MATRIX_H
#define MATRIX_H

#include "scene/main/node.h"
#include "core/io/http_client.h"
#include "core/os/thread.h"
#include "core/bind/core_bind.h"

class MatrixRoom;

class MatrixClient : public Node {
  friend class MatrixRoom;
  GDCLASS(MatrixClient,Node);

  HTTPClient sync_client;
  String hs_name;
  String auth_token;
  String sync_token;

  _Thread listener_thread;

  bool should_listen = true;

  Dictionary rooms;

  HTTPClient::ResponseCode request(HTTPClient &client, String endpoint, String body, HTTPClient::Method method, String &response_body);
  HTTPClient::ResponseCode request(HTTPClient &client, String endpoint, String body, HTTPClient::Method method);
  HTTPClient::ResponseCode request(String endpoint, String body, HTTPClient::Method method, String &response_body);
  HTTPClient::ResponseCode request(String endpoint, String body, HTTPClient::Method method);


protected:
  static void _bind_methods();

public:
  String get_hs_name() const;
  void set_hs_name(String name);

  String get_auth_token() const;
  void set_auth_token(String token);

  String get_sync_token() const;
  void set_sync_token(String token);

  Dictionary get_rooms() const;

  Error login(String username, String password);
  Error logout();

  Error start_listening();
  bool is_listening();
  Error stop_listening();

  Error _sync(int timeout_ms=30000);
  Error _listen_forever(Variant userdata);

  MatrixClient();
};

class MatrixRoom : public Reference {
  friend class MatrixClient;
  friend class Map<String, MatrixRoom>;
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
