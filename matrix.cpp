#include "matrix.h"

#include "io/json.h"

HTTPClient::ResponseCode MatrixClient::request(HTTPClient &client, String endpoint, String body, HTTPClient::Method method, String &response_body) {
  client.poll();

  if (
      client.get_status() == HTTPClient::Status::STATUS_DISCONNECTED ||
      client.get_status() == HTTPClient::Status::STATUS_CONNECTION_ERROR ||
      client.get_status() == HTTPClient::Status::STATUS_SSL_HANDSHAKE_ERROR
      ) {
    client.connect_to_host(hs_name, 443, true, true);
  }

  while (client.get_status()!=HTTPClient::Status::STATUS_CONNECTED) {
    client.poll();
  }

  Vector<String> headers = Vector<String>();
  headers.insert(0, "Content-Type: application/json");
  headers.insert(0, "Accept: application/json");
  
  String::num_int64((int)client.request(method, endpoint, headers, body));
  while (client.get_status()==HTTPClient::Status::STATUS_REQUESTING) {
    client.poll();
  }
  PoolByteArray response_body_array = PoolByteArray();

  while (client.get_status()==HTTPClient::Status::STATUS_BODY) {
    client.poll();
    response_body_array.append_array(client.read_response_body_chunk());
  }

  if (response_body_array.size() >= 0) {
    PoolByteArray::Read r = response_body_array.read();
    response_body.parse_utf8((const char *)r.ptr(), response_body_array.size());
  }
  
  return (HTTPClient::ResponseCode)client.get_response_code();
}

HTTPClient::ResponseCode MatrixClient::request(HTTPClient &client, String endpoint, String body, HTTPClient::Method method) {
  String response;
  return request(client, endpoint, body, method, response);
}

HTTPClient::ResponseCode MatrixClient::request(String endpoint, String body, HTTPClient::Method method, String &response) {
  HTTPClient client;
  return request(client, endpoint, body, method, response);
}

HTTPClient::ResponseCode MatrixClient::request(String endpoint, String body, HTTPClient::Method method) {
  HTTPClient client;
  String response;
  return request(client, endpoint, body, method, response);
}

Error MatrixClient::_sync(int timeout_ms) {
  String sync_filter = "filter=" + String("{ \"room\": { \"timeline\" : { \"limit\" : 10 } } }").http_escape();
  String since;
  if (sync_token.length() > 0) {
    since = "since=" + sync_token;
  }
  String timeout = "timeout=" + String::num_int64(timeout_ms);
  String response_json;
  HTTPClient::ResponseCode http_status = request(sync_client, "/_matrix/client/r0/sync?"+sync_filter+"&"+since+"&"+timeout+"&access_token="+auth_token, String(), HTTPClient::Method::METHOD_GET, response_json);
  
  if (http_status == 200) {
    Variant response_variant = Variant();
    String r_err_str;
    int32_t r_err_line;
    Error parse_err = JSON::parse(response_json, response_variant, r_err_str, r_err_line);
    if (parse_err) { return parse_err; }
    Dictionary response = (Dictionary)response_variant;

    sync_token = response["next_batch"];

    Dictionary leave_rooms = ((Dictionary)response["rooms"])["leave"];
    Array leave_rooms_keys = leave_rooms.keys();
    for (int i=0; i<leave_rooms_keys.size(); i++) {
      emit_signal("left_room", (Dictionary)leave_rooms[leave_rooms_keys[i]]);

      rooms.erase(leave_rooms_keys[i]);
    }

    Dictionary join_rooms = ((Dictionary)response["rooms"])["join"];
    Array join_rooms_keys = join_rooms.keys();
    for (int i=0; i<join_rooms_keys.size(); i++) {
      if (!rooms.has(join_rooms_keys[i])) {
        Ref<MatrixRoom> new_room = memnew(MatrixRoom);
        new_room->init(this, join_rooms_keys[i]);
        rooms[join_rooms_keys[i]] = new_room;
      }

      Ref<MatrixRoom> room = rooms[join_rooms_keys[i]];
      Dictionary sync_room = (Dictionary)join_rooms[join_rooms_keys[i]];

      Array state_events = ((Dictionary)sync_room["state"])["events"];
      for (int j=0; j<state_events.size(); j++) {
        Dictionary event = state_events[j];
        event["room_id"] = join_rooms_keys[i];
        room->_process_state_event(event);

        emit_signal("state_event", event);
      }

      room->prev_batch = ((Dictionary)sync_room["timeline"])["prev_batch"];
      Array timeline_events = ((Dictionary)sync_room["timeline"])["events"];
      for (int j=0; j<timeline_events.size(); j++) {
        Dictionary event = timeline_events[j];
        event["room_id"] = join_rooms_keys[i];
        room->_put_event(event);

        if (event["type"] == "m.room.name" || event["type"] == "m.room.topic") { //TODO: is this a hack or how it's supposed to be?
          room->_process_state_event(event);
          emit_signal("state_event", event);
        }
        
        emit_signal("timeline_event", event);
      }

      Array ephemeral_events = ((Dictionary)sync_room["ephemeral"])["events"];
      for (int j=0; j<ephemeral_events.size(); j++) {
        Dictionary event = ephemeral_events[j];
        event["room_id"] = join_rooms_keys[i];
        room->_put_ephemeral_event(event);
        
        emit_signal("ephemeral_event", event);
      }
    }
    
    Dictionary invite_rooms = ((Dictionary)response["rooms"])["invite"];
    Array invite_rooms_keys = invite_rooms.keys();
    for (int i=0; i<invite_rooms_keys.size(); i++) {
      emit_signal("invited_to_room", (Dictionary)invite_rooms[invite_rooms_keys[i]]);
    }

    return Error::OK;
  } else {
    return Error::ERR_QUERY_FAILED;
  }
}

String MatrixClient::get_hs_name() const {
  return hs_name;
}

void MatrixClient::set_hs_name(String name) {
  hs_name = name;
}

String MatrixClient::get_auth_token() const {
  return auth_token;
}

void MatrixClient::set_auth_token(String token) {
  auth_token = token;
}

String MatrixClient::get_sync_token() const {
  return sync_token;
}

void MatrixClient::set_sync_token(String token) {
  sync_token = token;
}

Dictionary MatrixClient::get_rooms() const {
  return rooms;
}

Error MatrixClient::login(String username, String password) {
  Dictionary request_body = Dictionary();
  request_body["type"] = "m.login.password";
  request_body["user"] = username;
  request_body["password"] = password;
  String body_json = JSON::print(request_body);

  String response_json;
  HTTPClient::ResponseCode http_status = request("/_matrix/client/r0/login", body_json, HTTPClient::Method::METHOD_POST, response_json);
  
  if (http_status == 200) {
    Variant response = Variant();
    String r_err_str;
    int32_t r_err_line;
    Error parse_err = JSON::parse(response_json, response, r_err_str, r_err_line);
    if (parse_err) { return parse_err; }

    auth_token = ((Dictionary)response)["access_token"];

    return Error::OK;
  } else {
    return Error::ERR_QUERY_FAILED;
  }
}  

Error MatrixClient::logout() {
  if (auth_token.length() == 0) {
    return Error::ERR_UNCONFIGURED;
  }
  request("/_matrix/client/r0/logout?access_token"+auth_token, String(), HTTPClient::Method::METHOD_POST);
  return Error::OK;
}

Error MatrixClient::start_listening() {
  if (!listener_thread.is_active()) {
    listener_thread.start(this, "_listen_forever");
    return Error::OK;
  } else {
    return Error::ERR_ALREADY_IN_USE;
  }
}

bool MatrixClient::is_listening() {
  return listener_thread.is_active();
}

Error MatrixClient::stop_listening() {
  if (listener_thread.is_active()) {
    should_listen = false;
    listener_thread.wait_to_finish();
    return Error::OK;
  } else {
    return Error::ERR_ALREADY_IN_USE;
  }
}

Error MatrixClient::_listen_forever(Variant userdata=NULL) {
  const int timeout_ms = 30000;
  int bad_sync_timeout = 5000;
  should_listen = true;
  while(should_listen) {
    Error sync_status = _sync(timeout_ms);
    if (sync_status == Error::OK) {
      bad_sync_timeout = 5;
    } else {
      print_line("error while syncing");
      OS::get_singleton()->delay_usec(bad_sync_timeout*1000);
      if (bad_sync_timeout < 3600) { bad_sync_timeout *= 2; }
      print_line("retrying");
    }
    //TODO: either retry or return an error depending on the kind of sync error
  }
  return Error::OK;
}

Variant MatrixClient::create_room(String alias, bool is_public, Array invitees) {
  if (hs_name.length() == 0 || auth_token.length() == 0) {
    WARN_PRINT("Not connected and authenticated to a homeserver!");
    return false;
  }

  Dictionary content;
  if (is_public) {
    content["visibility"] = "public";
  } else {
    content["visibility"] = "private";
  }
  if (alias.length() != 0) {
    content["room_alias_name"] = alias;
  }
  if (!invitees.empty()) {
    content["invitees"] = invitees;
  }
  String body_json = JSON::print(content);

  String response_json;
  HTTPClient::ResponseCode http_status = request("/_matrix/client/r0/createRoom?access_token="+auth_token, body_json, HTTPClient::Method::METHOD_POST, response_json);
  
  if (http_status == 200) {
    Variant response = Variant();
    String r_err_str;
    int32_t r_err_line;
    Error parse_err = JSON::parse(response_json, response, r_err_str, r_err_line);
    if (parse_err) { return false; }

    Ref<MatrixRoom> room = memnew(MatrixRoom);
    room->init(this, ((Dictionary)response)["room_id"]);
    rooms[((Dictionary)response)["room_id"]] = room;
    return room;
  } else {
    WARN_PRINT("Unable to create new room!");
    return false;
  }
}

Variant MatrixClient::join_room(String room_id_or_alias) {
  if (hs_name.length() == 0 || auth_token.length() == 0) {
    WARN_PRINT("Not connected and authenticated to a homeserver!");
    return false;
  }

  String response_json;
  HTTPClient::ResponseCode http_status = request("/_matrix/client/r0/joinRoom/"+room_id_or_alias+"?access_token="+auth_token, String(), HTTPClient::Method::METHOD_POST, response_json);
  
  if (http_status == 200) {
    Variant response = Variant();
    String r_err_str;
    int32_t r_err_line;
    Error parse_err = JSON::parse(response_json, response, r_err_str, r_err_line);
    if (parse_err) { return false; }

    Ref<MatrixRoom> room = memnew(MatrixRoom);
    room->init(this, ((Dictionary)response)["room_id"]);
    rooms[((Dictionary)response)["room_id"]] = room;
    return room;
  } else if (http_status == 403) {
    WARN_PRINT("Not allowed to join room!");
    return false;
  } else {
    WARN_PRINT("Unable to join room!");
    return false;
  }
}

void MatrixClient::_bind_methods() {
  ClassDB::bind_method("set_hs_name", &MatrixClient::set_hs_name);
  ClassDB::bind_method("get_hs_name", &MatrixClient::get_hs_name);
  ClassDB::bind_method("set_auth_token", &MatrixClient::set_auth_token);
  ClassDB::bind_method("get_auth_token", &MatrixClient::get_auth_token);
  ClassDB::bind_method("set_sync_token", &MatrixClient::set_sync_token);
  ClassDB::bind_method("get_sync_token", &MatrixClient::get_sync_token);
  ADD_PROPERTY( PropertyInfo(Variant::STRING,"hs_name"), "set_hs_name", "get_hs_name" );
  ADD_PROPERTY( PropertyInfo(Variant::STRING,"auth_token"), "set_auth_token", "get_auth_token" );
  ADD_PROPERTY( PropertyInfo(Variant::STRING,"sync_token"), "set_sync_token", "get_sync_token" );

  ClassDB::bind_method("login", &MatrixClient::login);
  ClassDB::bind_method("logout", &MatrixClient::logout);
  ClassDB::bind_method("_sync", &MatrixClient::_sync);
  ClassDB::bind_method("start_listening", &MatrixClient::start_listening);
  ClassDB::bind_method("is_listening", &MatrixClient::is_listening);
  ClassDB::bind_method("stop_listening", &MatrixClient::stop_listening);
  ClassDB::bind_method("_listen_forever", &MatrixClient::_listen_forever);
  ClassDB::bind_method("get_rooms", &MatrixClient::get_rooms);

  ClassDB::bind_method("create_room", &MatrixClient::create_room);
  ClassDB::bind_method("join_room", &MatrixClient::join_room);

  ADD_SIGNAL( MethodInfo("timeline_event") );
  ADD_SIGNAL( MethodInfo("ephemeral_event") );
  ADD_SIGNAL( MethodInfo("state_event" ) );
  ADD_SIGNAL( MethodInfo("invited_to_room") );
  ADD_SIGNAL( MethodInfo("left_room") );
}

MatrixClient::MatrixClient() {
}
