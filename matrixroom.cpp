#include "matrixroom.h"

#include "io/json.h"

void MatrixRoom::_put_event(Dictionary event) {
  events.append(event);
  if (events.size() > event_history_limit) {
    events.pop_front();
  }

  emit_signal("timeline_event", event);
}

void MatrixRoom::_put_ephemeral_event(Dictionary event) {
  emit_signal("ephemeral_event", event);
}

Error MatrixRoom::_process_state_event(Dictionary event) {
  if (!event.has("type")) {
    return Error::ERR_INVALID_DATA;
  }
  print_line(JSON::print(event));
  
  String event_type = event["type"];

  if (event_type == "m.room.name") {
    if (((Dictionary)event["content"]).has("name")) {
      name = ((Dictionary)event["content"])["name"];
    } else {
      name = String();
    }
  } else if (event_type == "m.room.topic") {
    if (((Dictionary)event["content"]).has("topic")) {
      topic = ((Dictionary)event["content"])["topic"];
    } else {
      topic = String();
    }
  } else if (event_type == "m.room.aliases") {
    if (((Dictionary)event["content"]).has("aliases")) {
      aliases[((Dictionary)event["state_key"])] = ((Dictionary)event["content"])["aliases"];
    }
  }

  emit_signal("state_event", event);

  return Error::OK;
}

int MatrixRoom::get_event_history_limit() {
  return event_history_limit;
}

void MatrixRoom::set_event_history_limit(int limit) {
  event_history_limit = limit;
}

String MatrixRoom::get_name() const {
  return name;
}

String MatrixRoom::get_topic() const {
  return topic;
}

Array MatrixRoom::get_events() const {
  return events;
}

Dictionary MatrixRoom::get_aliases() const {
  return aliases;
}

Error MatrixRoom::send_text_message(String text) {
  //this is just to generate a unique ID for every message sent from this client
  String txn_id = String::num_int64((OS::get_singleton()->get_unix_time()*1000)+OS::get_singleton()->get_ticks_msec());

  Dictionary request_body = Dictionary();
  request_body["msgtype"] = "m.text";
  request_body["body"] = text;
  String body_json = JSON::print(request_body);
  print_line(txn_id);

  HTTPClient::ResponseCode status = client->request("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/send/m.room.message/"+txn_id+"?access_token="+client->get_auth_token(), body_json, HTTPClient::Method::METHOD_PUT);
  
  if (status == 200) {
    return Error::OK;
  } else {
    return Error::ERR_QUERY_FAILED;
  }
}

void MatrixRoom::_bind_methods() {
  ClassDB::bind_method("get_event_history_limit", &MatrixRoom::get_event_history_limit);
  ClassDB::bind_method("set_event_history_limit", &MatrixRoom::set_event_history_limit);
  ADD_PROPERTY( PropertyInfo(Variant::INT,"event_history_limit"), "set_event_history_limit", "get_event_history_limit");

  ClassDB::bind_method("get_name", &MatrixRoom::get_name);
  ClassDB::bind_method("get_topic", &MatrixRoom::get_topic);
  ClassDB::bind_method("get_events", &MatrixRoom::get_events);
  ClassDB::bind_method("get_aliases", &MatrixRoom::get_aliases);

  ClassDB::bind_method("send_text_message", &MatrixRoom::send_text_message);

  ADD_SIGNAL( MethodInfo("timeline_event") );
  ADD_SIGNAL( MethodInfo("ephemeral_event") );
  ADD_SIGNAL( MethodInfo("state_event") );
}

MatrixRoom::MatrixRoom() {
}

void MatrixRoom::init(MatrixClient *c, String id) {
  client = c;
  room_id = id;

  //TODO: replace the following with a dedicated "sync room state" method
  String response_json;
  HTTPClient::ResponseCode status = client->request("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/state/m.room.name?access_token="+client->get_auth_token(), String(), HTTPClient::Method::METHOD_GET, response_json);
  
  if (status == 200) {
    Variant response = Variant();
    String r_err_str;
    int32_t r_err_line;
    Error parse_err = JSON::parse(response_json, response, r_err_str, r_err_line);
    name = ((Dictionary)response)["name"];
  }

  response_json = String();
  status = client->request("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/state/m.room.topic?access_token="+client->get_auth_token(), String(), HTTPClient::Method::METHOD_GET, response_json);
  
  if (status == 200) {
    Variant response = Variant();
    String r_err_str;
    int32_t r_err_line;
    Error parse_err = JSON::parse(response_json, response, r_err_str, r_err_line);
    topic = ((Dictionary)response)["topic"];
  }
}
