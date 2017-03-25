#ifndef MATRIX_H
#define MATRIX_H

#include "scene/main/node.h"
#include "core/io/http_client.h"
#include "core/os/thread.h"
#include "core/bind/core_bind.h"

#include "matrixroom.h"
#include "matrixuser.h"

class MatrixRoom;
class MatrixUser;

class MatrixClient : public Node {
  friend class MatrixRoom;
  friend class MatrixUser;
  GDCLASS(MatrixClient,Node);

  HTTPClient sync_client;
  String hs_name;
  String auth_token;
  String sync_token;
  String user_id;

  _Thread listener_thread;

  bool should_listen = true;

  Dictionary rooms;

  HTTPClient::ResponseCode request(HTTPClient &client, String endpoint, String body, HTTPClient::Method method, String &response_body);
  HTTPClient::ResponseCode request(HTTPClient &client, String endpoint, String body, HTTPClient::Method method);
  HTTPClient::ResponseCode request(String endpoint, String body, HTTPClient::Method method, String &response_body);
  HTTPClient::ResponseCode request(String endpoint, String body, HTTPClient::Method method);

  HTTPClient::ResponseCode request_json(String endpoint, Dictionary body, HTTPClient::Method method, Dictionary &response_body, bool auth=true);
  HTTPClient::ResponseCode request_json(String endpoint, Dictionary body, HTTPClient::Method method, bool auth=true);

protected:
  static void _bind_methods();

public:
  String get_hs_name() const;
  void set_hs_name(String name);

  String get_auth_token() const;
  void set_auth_token(String token);

  String get_sync_token() const;
  void set_sync_token(String token);

  String get_user_id() const;
  void set_user_id(String id);

  Dictionary get_rooms() const;

  Error login(String username, String password);
  Error logout();

  Error start_listening();
  bool is_listening();
  Error stop_listening();

  Error _sync(int timeout_ms=30000);
  Error _listen_forever(Variant userdata);

  Variant create_room(String alias=String(), bool is_public=false, Array invitees = Array());
  Variant join_room(String room_id_or_alias);

  //Variant upload(PoolByteArray data, String content_type); TODO: :-D
 
  Variant get_user(String user_id);
  Variant get_me();

  MatrixClient();
};

#endif 
