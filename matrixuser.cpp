#include "matrixuser.h"
#include "core/io/json.h"

String MatrixUser::get_display_name(bool sync) {
  if (sync) {
    Variant response_v;
    client->request_json("/_matrix/client/r0/profile/"+user_id+"/displayname", Dictionary(), HTTPClient::Method::METHOD_GET, response_v);
    Dictionary response = response_v;
    display_name = response["displayname"];
  }

  return display_name;
}

String MatrixUser::get_friendly_name(bool sync) {
  get_display_name(sync);

  if (display_name.length() != 0) {
    return display_name;
  } else {
    return user_id;
  }
}

Error MatrixUser::set_display_name(String name) {
  Dictionary request_body;
  request_body["displayname"] = name;

  Variant response_v;
  HTTPClient::ResponseCode http_status = client->request_json("/_matrix/client/r0/profile/"+user_id.http_escape()+"/displayname", request_body, HTTPClient::Method::METHOD_PUT, response_v);

  if (http_status == 200) {
    Dictionary response = response_v;
    display_name = name;
    return MATRIX_OK;
  } else {
    return MATRIX_NOT_IMPLEMENTED;
  }
}

String MatrixUser::get_avatar_url(bool sync) {
  if (sync) {
    Variant response_v;
    HTTPClient::ResponseCode http_status = client->request_json("/_matrix/client/r0/profile/"+user_id.http_escape()+"/displayname", Dictionary(), HTTPClient::Method::METHOD_GET, response_v);
    Dictionary response = response_v;
    if (response.has("avatar_url")) {
      avatar_url = response["avatar_url"];
    } else {
      avatar_url = String();
    }
  }

  return "https://"+client->get_hs_name()+"/_matrix/media/r0/download/"+avatar_url.substr(6, avatar_url.length()-6);
}

Error MatrixUser::set_avatar_url(String mxcurl) {
  if (!mxcurl.begins_with("mxc://")) {
    WARN_PRINT("Avatar URL must begin with \"mxc://\"!");
    return MATRIX_INVALID_REQUEST;
  }

  Dictionary request_body;
  request_body["avatar_url"] = mxcurl;

  Variant response_v;
  HTTPClient::ResponseCode http_status = client->request_json("/_matrix/client/r0/profile/"+user_id+"/avatar_url", request_body, HTTPClient::Method::METHOD_PUT, response_v);
  Dictionary response = response_v;

  if (http_status == 200) {
    avatar_url = mxcurl;
    return MATRIX_OK;
  } else {
    WARN_PRINT(((String)response["error"]).utf8().get_data());
    return MATRIX_NOT_IMPLEMENTED;
  }
}

MatrixUser::MatrixUser() {
}

void MatrixUser::init(MatrixClient *c, String id) {
  client = c;
  user_id = id;

}
void MatrixUser::_bind_methods() {
  ClassDB::bind_method("get_display_name", &MatrixUser::get_display_name);
  ClassDB::bind_method("get_friendly_name", &MatrixUser::get_friendly_name);
  ClassDB::bind_method("set_display_name", &MatrixUser::set_display_name);
  ClassDB::bind_method("get_avatar_url", &MatrixUser::get_avatar_url);
  ClassDB::bind_method("set_avatar_url", &MatrixUser::set_avatar_url);
}
