#include "matrixroom.h"

#include "io/json.h"

void MatrixRoom::_put_event(Dictionary event) {
  if (!event_ids.has(event["event_id"])) {
    event_ids.insert(event["event_id"]);
    events.push_back(event);

    emit_signal("timeline_event", event);
  }
}

void MatrixRoom::_put_old_event(Dictionary event) {
  if (!event_ids.has(event["event_id"])) {
    event_ids.insert(event["event_id"]);
    events.push_front(event);

    emit_signal("old_timeline_event", event);
  }
}

void MatrixRoom::_put_ephemeral_event(Dictionary event) {
  emit_signal("ephemeral_event", event);
}

Error MatrixRoom::_process_state_event(Dictionary event) {
  if (!event.has("type")) {
    return MATRIX_INVALID_RESPONSE;
  }

  String event_type = event["type"];

  if (event_type == "m.room.member") {
    String member = event["state_key"];

    if (((Dictionary)event["content"])["membership"] == "join") {
      members[member] = (Dictionary)event["content"];
    } else if (members.has(member)) {
      members.erase(member);
    }
  } else if (event_type == "m.room.name") {
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

  return MATRIX_OK;
}

String MatrixRoom::get_name(bool sync) {
  if (sync) {
    Variant response_v;
    HTTPClient::ResponseCode status = client->request_json("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/state/m.room.name", Dictionary(), HTTPClient::Method::METHOD_GET, response_v);
    if (status == 200) {
      Dictionary response = response_v;
      name = response["name"];
    } else {
      WARN_PRINT("Unable to get room name!");
    }
  }

  return name;
}

String MatrixRoom::get_friendly_name(bool sync) {
  get_name(sync);

  if (name.length() != 0) {
    return name;
  } else {
    return room_id;
  }
}

String MatrixRoom::get_topic(bool sync) {
  if (sync) {
    Variant response_v;
    HTTPClient::ResponseCode status = client->request_json("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/state/m.room.topic", Dictionary(), HTTPClient::Method::METHOD_GET, response_v);
    if (status == 200) {
      Dictionary response = response_v;
      topic = response["topic"];
    } else {
      WARN_PRINT("Unable to get room topic!");
    }
  }
  
  return topic;
}

Array MatrixRoom::get_events() const {
  return events;
}

Error MatrixRoom::get_old_events(int num_events) {
  String from;
  if (backfill_prev_batch.length() != 0) {
    from = backfill_prev_batch;
  } else {
    from = prev_batch;
  }
  Variant response_v;
  HTTPClient::ResponseCode status = client->request_json("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/messages?from="+from.http_escape()+"&dir=b&limit="+String::num_int64(num_events), Dictionary(), HTTPClient::Method::METHOD_GET, response_v);
  if (status == 200) {
    Dictionary response = response_v;
    backfill_prev_batch = response["end"]; //TODO: check if this is correct or if it should be "start" instead
    Array events = response["chunk"];
    for (int i=0; i<events.size(); i++) {
      _put_old_event(events[i]);
    }
    return MATRIX_OK;
  } else {
    WARN_PRINT("Unable to get old events!");
    return MATRIX_UNABLE;
  }
}

Dictionary MatrixRoom::get_aliases() const {
  return aliases;
}

Error MatrixRoom::send_text_message(String text) {
  Dictionary request_body = Dictionary();
  request_body["msgtype"] = "m.text";
  request_body["body"] = text;

  return send_event("m.text", request_body);
}

Error MatrixRoom::send_event(String msgtype, Dictionary event) {
  //this is just to generate a unique ID for every message sent from this client, not some kind of reliable timestamp or anything
  String txn_id = String::num_int64((OS::get_singleton()->get_unix_time()*1000)+(OS::get_singleton()->get_ticks_msec()%1000));

  Dictionary request_body = event;
  event["msgtype"] = msgtype;

  HTTPClient::ResponseCode status = client->request_json("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/send/m.room.message/"+txn_id, request_body, HTTPClient::Method::METHOD_PUT);
  
  if (status == 200) {
    return MATRIX_OK;
  } else if (status == 403) {
    ERR_PRINT("Not allowed to send event");
    return MATRIX_UNAUTHORIZED;
  } else {
    return MATRIX_NOT_IMPLEMENTED;
  }
}

Error MatrixRoom::set_typing(bool typing, int timeout_ms) {
  Dictionary request_body;
  request_body["typing"] = typing;
  request_body["timeout"] = timeout_ms;
  
  HTTPClient::ResponseCode status = client->request_json("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/typing/"+client->get_user_id().http_escape(), request_body, HTTPClient::Method::METHOD_PUT);

  if (status == 200) {
    return MATRIX_OK;
  } else if (status == 403) {
    ERR_PRINT("Not allowed to set typing status");
    return MATRIX_UNAUTHORIZED;
  } else if (status == 429) {
    ERR_PRINT("Ratelimited");
    return MATRIX_RATELIMITED;
  } else {
    return MATRIX_NOT_IMPLEMENTED;
  }
}

Dictionary MatrixRoom::get_members(bool sync) {
  if (sync) {
    Variant response_v;
    HTTPClient::ResponseCode status = client->request_json("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/members", Dictionary(), HTTPClient::Method::METHOD_GET, response_v);
    if (status == 200) {
      Dictionary response = response_v;
      if (response.has("chunk")) {
        Array chunk = response["chunk"];
        for (int i=0; i<chunk.size(); i++) {
          Dictionary event = chunk[i];
          if (((Dictionary)event["content"])["membership"] == "join") {
            members[event["state_key"]] = (Dictionary)event["content"];
          }
        }
      }
    }
  }

  return members;
}

String MatrixRoom::get_member_display_name(String id, bool sync) {
  if (sync) {
    Variant response_v;
    HTTPClient::ResponseCode status = client->request_json("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/state/m.room.member/"+id.http_escape(), Dictionary(), HTTPClient::Method::METHOD_GET, response_v);
    if (status == 200) {
      Dictionary response = response_v;
      print_line(JSON::print(response));
      members[id] = response;
    } else if (status == 404) {
      WARN_PRINT("Tried to look up non-existent room member!");
    }
  }
  if (members.has(id) && ((Dictionary)members[id]).has("displayname")) {
    return ((Dictionary)members[id])["displayname"]; //TODO: disambiguate display names
  } else {
    return id;
  }
}

void MatrixRoom::_state_sync(Variant userdata) {
  Variant response_v;
  HTTPClient::ResponseCode status = client->request_json("/_matrix/client/r0/rooms/"+room_id.http_escape()+"/state", Dictionary(), HTTPClient::Method::METHOD_GET, response_v);

  if (status == 200) {
    Array response = response_v;
    for (int i=0; i<response.size(); i++) {
      _process_state_event(response[i]);
    }
  }
}

Variant MatrixRoom::state_sync() {
  Ref<_Thread> state_thread = memnew(_Thread);
  state_thread->start(this, "_state_sync");
  return state_thread;
}

Variant MatrixRoom::leave_room() {
  return client->leave_room(room_id);
}

MatrixRoom::MatrixRoom() {
}

void MatrixRoom::init(MatrixClient *c, String id) {
  client = c;
  room_id = id;
}

void MatrixRoom::_bind_methods() {
  ClassDB::bind_method("get_name", &MatrixRoom::get_name);
  ClassDB::bind_method("get_friendly_name", &MatrixRoom::get_friendly_name);
  ClassDB::bind_method("get_topic", &MatrixRoom::get_topic);
  ClassDB::bind_method("get_events", &MatrixRoom::get_events);
  ClassDB::bind_method("get_aliases", &MatrixRoom::get_aliases);

  ClassDB::bind_method("get_old_events", &MatrixRoom::get_old_events);

  ClassDB::bind_method("get_members", &MatrixRoom::get_members);
  ClassDB::bind_method("get_member_display_name", &MatrixRoom::get_member_display_name);

  ClassDB::bind_method("send_text_message", &MatrixRoom::send_text_message);

  ClassDB::bind_method("_state_sync", &MatrixRoom::_state_sync);
  ClassDB::bind_method("state_sync", &MatrixRoom::state_sync);
  
  ClassDB::bind_method("leave_room", &MatrixRoom::leave_room);

  ADD_SIGNAL( MethodInfo("timeline_event") );     //new event inserted at most recent point in timeline
  ADD_SIGNAL( MethodInfo("old_timeline_event") ); //old event inserted at beginning of timeline
  ADD_SIGNAL( MethodInfo("ephemeral_event") );
  ADD_SIGNAL( MethodInfo("state_event") );
}
