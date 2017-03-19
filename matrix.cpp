#include "matrix.h"

#include "io/json.h"

String MatrixClient::request(String endpoint, String body = String(), HTTPClient::Method method = HTTPClient::Method::METHOD_GET) {
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
  PoolByteArray response_body = PoolByteArray();

  while (m_client.get_status()==HTTPClient::Status::STATUS_BODY) {
    m_client.poll();
    response_body.append_array(m_client.read_response_body_chunk());
  }
  print_line(String::num_int64(m_client.get_response_code()));

  String s;
  if (response_body.size() >= 0) {
    PoolByteArray::Read r = response_body.read();
    s.parse_utf8((const char *)r.ptr(), response_body.size());
  }

  return s;
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

Error MatrixClient::login(String username, String password) {
  Dictionary request_body = Dictionary();
  request_body["type"] = "m.login.password";
  request_body["user"] = username;
  request_body["password"] = password;
  String body_json = JSON::print(request_body);

  String response_json = request("/_matrix/client/r0/login", body_json, HTTPClient::Method::METHOD_POST);
  
  Variant response = Variant();
  String r_err_str;
  int32_t r_err_line;
  Error parse_err = JSON::parse(response_json, response, r_err_str, r_err_line);
  if (parse_err) { return parse_err; }

  auth_token = ((Dictionary)response)["access_token"];

  return Error::OK;
}  

String MatrixClient::get_versions() {
  return request("/_matrix/client/versions");
}

void MatrixClient::_bind_methods() {
  ClassDB::bind_method("get_hs_name", &MatrixClient::get_hs_name);
  ClassDB::bind_method("set_hs_name", &MatrixClient::set_hs_name);
  ADD_PROPERTY( PropertyInfo(Variant::STRING,"hs_name"), "set_hs_name", "get_hs_name" );

  ClassDB::bind_method("login", &MatrixClient::login);
  ClassDB::bind_method("get_versions", &MatrixClient::get_versions);
}

MatrixClient::MatrixClient() {
}

