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

#define MATRIX_OK               Error::OK               //request completed successfully
#define MATRIX_CANT_CONNECT     Error::ERR_CANT_CONNECT //not able to connect to homeserver
#define MATRIX_UNAUTHENTICATED  Error::ERR_UNCONFIGURED //request requires authentication
#define MATRIX_UNAUTHORIZED     Error::ERR_UNAUTHORIZED //not allowed to do this
#define MATRIX_UNABLE           Error::FAILED           //request is not possible to fulfill
#define MATRIX_RATELIMITED      Error::ERR_BUSY         //too many requests, you are ratelimited
#define MATRIX_INVALID_REQUEST  Error::ERR_INVALID_DATA //request contained invalid data
#define MATRIX_INVALID_RESPONSE Error::ERR_PARSE_ERROR  //response contained invalid data
#define MATRIX_NOT_IMPLEMENTED  Error::ERR_BUG          //action is not yet implemented in this library

class MatrixClient : public Node {
  friend class MatrixRoom;
  friend class MatrixUser;
  GDCLASS(MatrixClient,Node);

  HTTPClient sync_client;
  String hs_name;
  String auth_token;
  String sync_token;
  String user_id;

  Thread* listener_thread;

  bool should_listen = true;

  Dictionary rooms;

  HTTPClient::ResponseCode request(HTTPClient &client, String endpoint, String body, HTTPClient::Method method, String &response_body);
  HTTPClient::ResponseCode request(HTTPClient &client, String endpoint, String body, HTTPClient::Method method);
  HTTPClient::ResponseCode request(String endpoint, String body, HTTPClient::Method method, String &response_body);
  HTTPClient::ResponseCode request(String endpoint, String body, HTTPClient::Method method);

  HTTPClient::ResponseCode request_json(String endpoint, Dictionary body, HTTPClient::Method method, Variant &response_body, bool auth=true);
  HTTPClient::ResponseCode request_json(String endpoint, Dictionary body, HTTPClient::Method method, bool auth=true);

  static void _listen_forever(void *m);

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

  Error register_account(Variant username, String password, bool guest=false);
  Error login(String username, String password);
  Error logout();

  Error start_listening();
  bool is_listening();
  Error stop_listening();

  Error _sync(int timeout_ms=30000);
  Error _listen_forever();

  Variant create_room(String alias=String(), bool is_public=false, Array invitees = Array());
  Variant join_room(String room_id_or_alias);
  Variant leave_room(String room_id);

  //Variant upload(PoolByteArray data, String content_type); TODO: :-D
 
  Variant get_user(String user_id);
  Variant get_me();

  MatrixClient();
};

#endif 
