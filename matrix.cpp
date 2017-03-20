#include "matrix.h"

#include "io/json.h"

HTTPClient::ResponseCode MatrixClient::request(String endpoint, String body, HTTPClient::Method method, String &response_body) {
  while (m_client.get_status()!=HTTPClient::Status::STATUS_CONNECTED) {
    m_client.poll();
  }

  Vector<String> headers = Vector<String>();
  headers.insert(0, "Content-Type: application/json");
  headers.insert(0, "Accept: application/json");
  
  m_client.request(method, endpoint, headers, body);
  while (m_client.get_status()==HTTPClient::Status::STATUS_REQUESTING) {
    m_client.poll();
  }
  PoolByteArray response_body_array = PoolByteArray();

  while (m_client.get_status()==HTTPClient::Status::STATUS_BODY) {
    m_client.poll();
    response_body_array.append_array(m_client.read_response_body_chunk());
  }

  if (response_body_array.size() >= 0) {
    PoolByteArray::Read r = response_body_array.read();
    response_body.parse_utf8((const char *)r.ptr(), response_body_array.size());
  }

  return (HTTPClient::ResponseCode)m_client.get_response_code();
}

HTTPClient::ResponseCode MatrixClient::request(String endpoint, String body, HTTPClient::Method method) {
  String response;
  return request(endpoint, body, method, response);
}

Error MatrixClient::sync(int timeout_ms) {
  String sync_filter = "filter=" + String("{ \"room\": { \"timeline\" : { \"limit\" : 10 } } }").http_escape();
  String since;
  if (sync_token.length() > 0) {
    since = "since=" + sync_token;
  }
  String timeout = "timeout=" + String::num_int64(timeout_ms);
  String response_json;
  HTTPClient::ResponseCode http_status = request("/_matrix/client/r0/sync?"+sync_filter+"&"+sync_token+"&"+timeout+"&access_token="+auth_token, String(), HTTPClient::Method::METHOD_GET, response_json);
  
  if (http_status == 200) {
    Variant response_variant = Variant();
    String r_err_str;
    int32_t r_err_line;
    Error parse_err = JSON::parse(response_json, response_variant, r_err_str, r_err_line);
    if (parse_err) { return parse_err; }
    Dictionary response = (Dictionary)response_variant;

    sync_token = response["next_batch"];

    print_line("leave rooms");
    Dictionary leave_rooms = ((Dictionary)response["rooms"])["leave"];
    Array leave_rooms_keys = leave_rooms.keys();
    for (int i=0; i<leave_rooms_keys.size(); i++) {
      print_line(leave_rooms_keys[i]);
    }

    print_line("join rooms");
    Dictionary join_rooms = ((Dictionary)response["rooms"])["join"];
    Array join_rooms_keys = join_rooms.keys();
    for (int i=0; i<join_rooms_keys.size(); i++) {
      if (!rooms.has(join_rooms_keys[i])) {
        rooms[join_rooms_keys[i]] = MatrixRoom();
        rooms[join_rooms_keys[i]].init(this, join_rooms_keys[i]);
      }

      MatrixRoom room = rooms[join_rooms_keys[i]];
      Dictionary sync_room = ((Dictionary)join_rooms[join_rooms_keys[i]]);

      room.prev_batch = ((Dictionary)sync_room["timeline"])["prev_batch"];

      Array timeline_events = ((Dictionary)sync_room["timeline"])["events"];
      for (int j=0; j<timeline_events.size(); j++) {
        Dictionary event = timeline_events[j];
        event["room_id"] = join_rooms_keys[i];
        room.put_event(event);
        //TODO: signal
      }

      //TODO: process state events
      //TODO: ephemeral events
      //TODO: event signals
    }
    
    /*
     * don't need this (yet)
    print_line("invite rooms");
    Dictionary invite_rooms = ((Dictionary)response["rooms"])["invite"];
    Array invite_rooms_keys = invite_rooms.keys();
    for (int i=0; i<invite_rooms_keys.size(); i++) {
    }
     */

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
  String::num_int64(m_client.connect_to_host(hs_name, 443, true, true));
  while (m_client.get_status()==HTTPClient::Status::STATUS_CONNECTING || m_client.get_status()==HTTPClient::Status::STATUS_RESOLVING) {
    m_client.poll();
  }
}

String MatrixClient::get_auth_token() const {
  return auth_token;
}

void MatrixClient::set_auth_token(String token) {
  auth_token = token;
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

    sync();

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

void MatrixClient::_bind_methods() {
  ClassDB::bind_method("get_hs_name", &MatrixClient::get_hs_name);
  ClassDB::bind_method("set_hs_name", &MatrixClient::set_hs_name);
  ClassDB::bind_method("set_auth_token", &MatrixClient::set_auth_token);
  ClassDB::bind_method("get_auth_token", &MatrixClient::get_auth_token);
  ADD_PROPERTY( PropertyInfo(Variant::STRING,"hs_name"), "set_hs_name", "get_hs_name" );
  ADD_PROPERTY( PropertyInfo(Variant::STRING,"auth_token"), "set_auth_token", "get_auth_token" );

  ClassDB::bind_method("login", &MatrixClient::login);
  ClassDB::bind_method("logout", &MatrixClient::logout);
}

MatrixClient::MatrixClient() {
}


void MatrixRoom::put_event(Dictionary event) {
  events.append(event);
  if (events.size() > event_history_limit) {
    events.pop_front();
  }

  //TODO: emit signals
}

int MatrixRoom::get_event_history_limit() {
  return event_history_limit;
}

void MatrixRoom::set_event_history_limit(int limit) {
  event_history_limit = limit;
}

Array MatrixRoom::get_events() {
  return events;
}

void MatrixRoom::_bind_methods() {
  ClassDB::bind_method("get_event_history_limit", &MatrixRoom::get_event_history_limit);
  ClassDB::bind_method("set_event_history_limit", &MatrixRoom::set_event_history_limit);
  ADD_PROPERTY( PropertyInfo(Variant::INT,"event_history_limit"), "set_event_history_limit", "get_event_history_limit");
  ClassDB::bind_method("get_events", &MatrixRoom::get_events);
}

MatrixRoom::MatrixRoom() {
}

void MatrixRoom::init(MatrixClient *c, String id) {
  client = c;
  room_id = id;

  String response_json;
  HTTPClient::ResponseCode status = client->request("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/state/m.room.name?access_token="+client->get_auth_token(), String(), HTTPClient::Method::METHOD_GET, response_json);
  
  if (status == 200) {
    Variant response = Variant();
    String r_err_str;
    int32_t r_err_line;
    Error parse_err = JSON::parse(response_json, response, r_err_str, r_err_line);
    name = ((Dictionary)response)["name"];
    print_line("DEBUG: room name is "+name);
  } else {
    print_line("DEBUG: room "+room_id+" has no display name");
  }
}
