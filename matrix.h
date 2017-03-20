#ifndef MATRIX_H
#define MATRIX_H

#include "scene/main/node.h"
#include "core/io/http_client.h"

class MatrixRoom;

class MatrixClient : public Node {
  friend class MatrixRoom;
  GDCLASS(MatrixClient,Node);

  HTTPClient m_client;
  String hs_name;
  String auth_token;
  String sync_token;

  Map<String, MatrixRoom> rooms;

  HTTPClient::ResponseCode request(String endpoint, String body, HTTPClient::Method method, String &response_body);
  HTTPClient::ResponseCode request(String endpoint, String body, HTTPClient::Method method);

  Error sync(int timeout_ms=30000);

protected:
  static void _bind_methods();

public:
  String get_hs_name() const;
  void set_hs_name(String name);

  String get_auth_token() const;
  void set_auth_token(String token);

  Error login(String username, String password);
  Error logout();

  String get_versions();

  MatrixClient();
};

class MatrixRoom : public Object {
  friend class MatrixClient;
  friend class Map<String, MatrixRoom>;
  GDCLASS(MatrixRoom,Object);

  MatrixClient *client;

  String room_id;
  String prev_batch;

  String name;
  String topic;

  Array events;
  int event_history_limit = 20;

  void put_event(Dictionary event);

protected:
  static void _bind_methods();

public:
  int get_event_history_limit();
  void set_event_history_limit(int limit);

  Array get_events();

  MatrixRoom();

  void init(MatrixClient *client, String room_id);
};

#endif 
