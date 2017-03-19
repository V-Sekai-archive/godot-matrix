#include "matrix.h"

#include "core/io/http_client.h"

String MatrixClient::get_hs_name() const {
  return hs_name;
}

String MatrixClient::get_versions() const {
  HTTPClient client = HTTPClient();

  print_line(String::num_int64(client.connect_to_host(hs_name, 443, true, true)));
  while (client.get_status()==HTTPClient::Status::STATUS_CONNECTING || client.get_status()==HTTPClient::Status::STATUS_RESOLVING) {
    client.poll();
  }

  Vector<String> headers = Vector<String>();
  
  client.request(HTTPClient::Method::METHOD_GET, "/_matrix/client/versions", headers);
  while (client.get_status()==HTTPClient::Status::STATUS_REQUESTING) {
    client.poll();
  }
  PoolByteArray body = PoolByteArray();

  while (client.get_status()==HTTPClient::Status::STATUS_BODY) {
    client.poll();
    body.append_array(client.read_response_body_chunk());
  }

  String s;
  if (body.size() >= 0) {
    PoolByteArray::Read r = body.read();
    s.parse_utf8((const char *)r.ptr(), body.size());
  }

  return s;
}

void MatrixClient::_bind_methods() {
  ClassDB::bind_method("get_hs_name", &MatrixClient::get_hs_name);
  ClassDB::bind_method("get_versions", &MatrixClient::get_versions);
}

MatrixClient::MatrixClient() {
  hs_name = "vurpo.fi";
}

